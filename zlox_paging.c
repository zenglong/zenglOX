/* zlox_paging.c Defines the interface for and structures relating to paging.*/

#include "zlox_paging.h"
#include "zlox_kheap.h"
#include "zlox_monitor.h"
#include "zlox_isr.h"

// The kernel's page directory
ZLOX_PAGE_DIRECTORY *kernel_directory=0;

// The current page directory;
ZLOX_PAGE_DIRECTORY *current_directory=0;

// A bitset of frames - used or free.
ZLOX_UINT32 *frames;
ZLOX_UINT32 nframes;

// Defined in zlox_kheap.c
extern ZLOX_UINT32 placement_address;
extern ZLOX_HEAP *kheap;

// Macros used in the bitset algorithms.
#define ZLOX_INDEX_FROM_BIT(a) (a/(8*4))
#define ZLOX_OFFSET_FROM_BIT(a) (a%(8*4))

ZLOX_VOID zlox_switch_page_directory(ZLOX_PAGE_DIRECTORY *dir);
ZLOX_PAGE *zlox_get_page(ZLOX_UINT32 address, ZLOX_SINT32 make, ZLOX_PAGE_DIRECTORY *dir);
ZLOX_VOID zlox_page_fault(ZLOX_ISR_REGISTERS regs);

// Static function to set a bit in the frames bitset
static ZLOX_VOID zlox_set_frame(ZLOX_UINT32 frame_addr)
{
	ZLOX_UINT32 frame = frame_addr/0x1000;
	ZLOX_UINT32 idx = ZLOX_INDEX_FROM_BIT(frame);
	ZLOX_UINT32 off = ZLOX_OFFSET_FROM_BIT(frame);
	frames[idx] |= (0x1 << off);
}

// Static function to clear a bit in the frames bitset
static ZLOX_VOID zlox_clear_frame(ZLOX_UINT32 frame_addr)
{
	ZLOX_UINT32 frame = frame_addr/0x1000;
	ZLOX_UINT32 idx = ZLOX_INDEX_FROM_BIT(frame);
	ZLOX_UINT32 off = ZLOX_OFFSET_FROM_BIT(frame);
	frames[idx] &= ~(0x1 << off);
}

// Static function to test if a bit is set.
/*static ZLOX_UINT32 zlox_test_frame(ZLOX_UINT32 frame_addr)
{
	ZLOX_UINT32 frame = frame_addr/0x1000;
	ZLOX_UINT32 idx = ZLOX_INDEX_FROM_BIT(frame);
	ZLOX_UINT32 off = ZLOX_OFFSET_FROM_BIT(frame);
	return (frames[idx] & (0x1 << off));
}*/

// Static function to find the first free frame.
static ZLOX_UINT32 zlox_first_frame()
{
	ZLOX_UINT32 i, j;
	for (i = 0; i < ZLOX_INDEX_FROM_BIT(nframes); i++)
	{
		if (frames[i] != 0xFFFFFFFF) // nothing free, exit early.
		{
			// at least one bit is free here.
			for (j = 0; j < 32; j++)
			{
				ZLOX_UINT32 toTest = 0x1 << j;
				if ( !(frames[i]&toTest) )
				{
					return i*4*8+j;
				}
			}
		}
	}
	return 0xFFFFFFFF;
}

// Function to allocate a frame.
ZLOX_VOID zlox_alloc_frame(ZLOX_PAGE *page, ZLOX_SINT32 is_kernel, ZLOX_SINT32 is_writeable)
{
	if (page->frame != 0)
	{
		return;
	}
	else
	{
		ZLOX_UINT32 tmp_page = zlox_first_frame();
		if (tmp_page == ((ZLOX_UINT32)(-1)))
		{
			// PANIC! no free frames!!
			// 在frames位图里,没有足够的物理内存可供分配了
			ZLOX_PANIC("no free frames!!");
		}
		zlox_set_frame(tmp_page*0x1000);
		page->present = 1;
		page->rw = (is_writeable)?1:0;
		page->user = (is_kernel)?0:1;
		page->frame = tmp_page;
	}
}

// Function to deallocate a frame.
ZLOX_VOID zlox_free_frame(ZLOX_PAGE *page)
{
	ZLOX_UINT32 frame;
	if (!(frame=page->frame))
	{
		return;
	}
	else
	{
		zlox_clear_frame(frame);
		zlox_memset((ZLOX_UINT8 *)page,0,4);
		//page->frame = 0x0;
	}
}

ZLOX_VOID zlox_init_paging()
{
	ZLOX_UINT32 i;
	// The size of physical memory. For the moment we 
	// assume it is 16MB big.
	ZLOX_UINT32 mem_end_page = 0x1000000;

	nframes = mem_end_page / 0x1000;
	frames = (ZLOX_UINT32 *)zlox_kmalloc(nframes/8);
	zlox_memset((ZLOX_UINT8 *)frames, 0, nframes/8);

	// Let's make a page directory.
	kernel_directory = (ZLOX_PAGE_DIRECTORY *)zlox_kmalloc_a(sizeof(ZLOX_PAGE_DIRECTORY));
	// we must clean the memory of kernel_directory!
	zlox_memset((ZLOX_UINT8 *)kernel_directory, 0, sizeof(ZLOX_PAGE_DIRECTORY));
	current_directory = kernel_directory;

	// Map some pages in the kernel heap area.
	// Here we call get_page but not alloc_frame. This causes page_table_t's 
	// to be created where necessary. We can't allocate frames yet because they
	// they need to be identity mapped first below, and yet we can't increase
	// placement_address between identity mapping and enabling the heap!
	// 先为堆(当前起始线性地址为0xc0000000)分配页表项,但是暂时先不设置具体的物理内存,
	//	因为必须确保堆之前的线性地址能够先从frames位图里分配到物理内存,否则内核代码部分的
	//	线性地址就会找不到正确的物理地址(内核代码的线性地址等于物理地址),堆必须等内核部分的映射完,
	//	然后才能用它们后面的物理地址进行映射 
	i = 0;
	for (i = ZLOX_KHEAP_START; i < ZLOX_KHEAP_START+ZLOX_KHEAP_INITIAL_SIZE; i += 0x1000)
		zlox_get_page(i, 1, kernel_directory);

	// We need to identity map (phys addr = virt addr) from
	// 0x0 to the end of used memory, so we can access this
	// transparently, as if paging wasn't enabled.
	// NOTE that we use a while loop here deliberately.
	// inside the loop body we actually change placement_address
	// by calling kmalloc(). A while loop causes this to be
	// computed on-the-fly rather than once at the start.
	// 为内核代码部分(包括内核页目录,页表部分)进行zlox_get_page(分配页表)和进行zlox_alloc_frame(映射具体的物理地址)
	i = 0;
	while (i < placement_address + 0x1000) //加0x1000表示多分配一页,该页是为初始化堆准备的
	{
		// Kernel code is readable but not writeable from userspace.
		zlox_alloc_frame( zlox_get_page(i, 1, kernel_directory), 0, 0);
		i += 0x1000;
	}

	// Now allocate those pages we mapped earlier.
	// 为之前给堆(0xc0000000起始的线性地址)准备的页表设置具体的物理地址
	for (i = ZLOX_KHEAP_START; i < ZLOX_KHEAP_START+ZLOX_KHEAP_INITIAL_SIZE; i += 0x1000)
		zlox_alloc_frame(zlox_get_page(i, 1, kernel_directory), 0, 0);

	// Before we enable paging, we must register our page fault handler.
	zlox_register_interrupt_callback(14,zlox_page_fault);

	// Now, enable paging!
	zlox_switch_page_directory(kernel_directory);

	// 在0xc0000000线性地址处创建一个新的堆,用于后面的zlox_kmalloc,zlox_kfree之类的分配及释放操作
	kheap = zlox_create_heap(ZLOX_KHEAP_START,ZLOX_KHEAP_START+ZLOX_KHEAP_INITIAL_SIZE,0xCFFFF000, 0, 0);
}

ZLOX_VOID zlox_switch_page_directory(ZLOX_PAGE_DIRECTORY *dir)
{
	ZLOX_UINT32 cr0_val;
	current_directory = dir;
	asm volatile("mov %0, %%cr3":: "r"(dir->tablesPhysical));
	asm volatile("mov %%cr0, %0": "=r"(cr0_val));
	cr0_val |= 0x80000000; // Enable paging!
	asm volatile("mov %0, %%cr0":: "r"(cr0_val));
}

ZLOX_PAGE *zlox_get_page(ZLOX_UINT32 address, ZLOX_SINT32 make, ZLOX_PAGE_DIRECTORY *dir)
{
	ZLOX_UINT32 table_idx;
	// Turn the address into an index.
	address /= 0x1000;
	// Find the page table containing this address.
	table_idx = address / 1024;
	if (dir->tables[table_idx]) // If this table is already assigned
	{
		return &dir->tables[table_idx]->pages[address%1024];
	}
	else if(make)
	{
		ZLOX_UINT32 tmp;
		dir->tables[table_idx] = (ZLOX_PAGE_TABLE *)zlox_kmalloc_ap(sizeof(ZLOX_PAGE_TABLE), &tmp);
		// we must clean the new allocated memory!
		zlox_memset((ZLOX_UINT8 *)dir->tables[table_idx], 0, sizeof(ZLOX_PAGE_TABLE));
		dir->tablesPhysical[table_idx] = tmp | 0x7; // PRESENT, RW, US.
		return &dir->tables[table_idx]->pages[address%1024];
	}
	else
	{
		return 0;
	}
}

ZLOX_VOID zlox_page_fault(ZLOX_ISR_REGISTERS regs)
{
	// A page fault has occurred.
	// The faulting address is stored in the CR2 register.
	ZLOX_UINT32 faulting_address;
	ZLOX_SINT32 present;
	ZLOX_SINT32 rw;
	ZLOX_SINT32 us;
	ZLOX_SINT32 reserved;
	ZLOX_SINT32 id;

	asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
	
	// The error code gives us details of what happened.
	present   = !(regs.err_code & 0x1); // Page not present
	rw = regs.err_code & 0x2;		   // Write operation?
	us = regs.err_code & 0x4;		   // Processor was in user-mode?
	reserved = regs.err_code & 0x8;	 // Overwritten CPU-reserved bits of page entry?
	id = regs.err_code & 0x10;		  // Caused by an instruction fetch?

	// Output an error message.
	zlox_monitor_write("Page fault! ( ");

	if (present)  
		zlox_monitor_write("present ");
	if (rw)
		zlox_monitor_write("read-only ");
	if (us) 
		zlox_monitor_write("user-mode ");
	if (reserved) 
		zlox_monitor_write("reserved ");
	if(id)
		zlox_monitor_write("instruction fetch ");

	zlox_monitor_write(") at 0x");

	zlox_monitor_write_hex(faulting_address);
	zlox_monitor_write("\n");

	ZLOX_PANIC("Page fault");
}

