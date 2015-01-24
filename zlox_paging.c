/* zlox_paging.c Defines the interface for and structures relating to paging.*/

#include "zlox_paging.h"
#include "zlox_kheap.h"
#include "zlox_monitor.h"
#include "zlox_isr.h"
#include "zlox_descriptor_tables.h"
#include "zlox_vga.h"

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
// Defined in zlox_task.c
extern ZLOX_TASK * current_task;

// Defined in zlox_process.s
extern ZLOX_VOID _zlox_copy_page_physical(ZLOX_UINT32 src,ZLOX_UINT32 dest);

// Defined in zlox_descriptor_tables.c
extern ZLOX_TSS_ENTRY tss_entry;
extern ZLOX_TSS_ENTRY tss_entry_double_fault;

// zlox_vga.c
extern ZLOX_UINT32 vga_current_mode;

// Macros used in the bitset algorithms.
#define ZLOX_INDEX_FROM_BIT(a) (a/(8*4))
#define ZLOX_OFFSET_FROM_BIT(a) (a%(8*4))

ZLOX_VOID zlox_page_fault(ZLOX_ISR_REGISTERS * regs);
ZLOX_VOID zlox_double_fault(ZLOX_ISR_REGISTERS * regs);
static ZLOX_PAGE_TABLE * zlox_clone_table(ZLOX_PAGE_TABLE * src, ZLOX_UINT32 * physAddr, ZLOX_UINT32 needCopy);

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

// Function real to allocate a frame.
ZLOX_VOID zlox_alloc_frame_do(ZLOX_PAGE *page, ZLOX_SINT32 is_kernel, ZLOX_SINT32 is_writeable)
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
	page->rw = (is_writeable==1)?1:0;
	page->user = (is_kernel==1)?0:1;
	page->frame = tmp_page;
}

ZLOX_VOID zlox_alloc_frame_do_ext(ZLOX_PAGE *page, ZLOX_UINT32 frame_addr, ZLOX_SINT32 is_kernel, ZLOX_SINT32 is_writeable)
{
	page->present = 1;
	page->rw = (is_writeable==1)?1:0;
	page->user = (is_kernel==1)?0:1;
	page->frame = frame_addr / 0x1000;
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
		zlox_alloc_frame_do(page, is_kernel, is_writeable);
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
		zlox_clear_frame(frame * 0x1000);
		zlox_memset((ZLOX_UINT8 *)page,0,4);
		//page->frame = 0x0;
	}
}

ZLOX_VOID zlox_init_paging_start(ZLOX_UINT32 total_phymem)
{
	ZLOX_UINT32 i;
	// The size of physical memory. For the moment we 
	// assume it is 32MB big.
	//ZLOX_UINT32 mem_end_page = 0x1000000;
	ZLOX_UINT32 mem_end_page = 0x8000000;

	if(total_phymem > 0 && total_phymem < (0x8000000 / 1024))
		mem_end_page = total_phymem * 1024;

	nframes = mem_end_page / 0x1000;
	frames = (ZLOX_UINT32 *)zlox_kmalloc(nframes/8);
	zlox_memset((ZLOX_UINT8 *)frames, 0, nframes/8);

	// Let's make a page directory.
	kernel_directory = (ZLOX_PAGE_DIRECTORY *)zlox_kmalloc_a(sizeof(ZLOX_PAGE_DIRECTORY));
	// we must clean the memory of kernel_directory!
	zlox_memset((ZLOX_UINT8 *)kernel_directory, 0, sizeof(ZLOX_PAGE_DIRECTORY));
	kernel_directory->physicalAddr = (ZLOX_UINT32)kernel_directory->tablesPhysical;

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
	for (i = ZLOX_KHEAP_START; i < ZLOX_KHEAP_START + mem_end_page /*ZLOX_KHEAP_INITIAL_SIZE*/; i += 0x1000)
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
	zlox_register_interrupt_callback(8,zlox_double_fault);

	// Now, enable paging!
	zlox_switch_page_directory(kernel_directory);

	// 在0xc0000000线性地址处创建一个新的堆,用于后面的zlox_kmalloc,zlox_kfree之类的分配及释放操作
	kheap = zlox_create_heap(ZLOX_KHEAP_START,ZLOX_KHEAP_START+ZLOX_KHEAP_INITIAL_SIZE,0xCFFFF000, 0, 1);
}

ZLOX_VOID zlox_init_paging_end()
{
	current_directory = zlox_clone_directory(kernel_directory,1);
	zlox_switch_page_directory(current_directory);
}

ZLOX_VOID zlox_switch_page_directory(ZLOX_PAGE_DIRECTORY *dir)
{
	ZLOX_UINT32 cr0_val;
	current_directory = dir;
	asm volatile("mov %0, %%cr3":: "r"(dir->physicalAddr));
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

ZLOX_VOID zlox_page_copy(ZLOX_UINT32 copy_address)
{
	ZLOX_UINT32 phys;
	ZLOX_BOOL need_flushTLB = ZLOX_FALSE;
	ZLOX_BOOL need_copy_page_physical = ZLOX_FALSE;
	ZLOX_PAGE_TABLE * newTable;
	copy_address /= 0x1000;
	ZLOX_UINT32 table_idx = copy_address / 1024;
	ZLOX_UINT32 page_idx = copy_address % 1024;
	if( (current_directory->tablesPhysical[table_idx] & 0x2) == 0)
	{
		newTable = zlox_clone_table(current_directory->tables[table_idx],&phys,0);
		need_flushTLB = ZLOX_TRUE;
	}
	else
		newTable = current_directory->tables[table_idx];

	ZLOX_PAGE oldPage;
	if(newTable->pages[page_idx].frame == 0 || newTable->pages[page_idx].rw == 0)
	{
		oldPage = current_directory->tables[table_idx]->pages[page_idx];
		zlox_alloc_frame_do(&newTable->pages[page_idx], 0, 1);
		if (oldPage.present) newTable->pages[page_idx].present = 1;
		if (oldPage.user) newTable->pages[page_idx].user = 1;
		if (oldPage.accessed) newTable->pages[page_idx].accessed = 1;
		if (oldPage.dirty) newTable->pages[page_idx].dirty = 1;
		newTable->pages[page_idx].rw = 1;
		need_flushTLB = ZLOX_TRUE;
		need_copy_page_physical = ZLOX_TRUE;
	}

	if( (current_directory->tablesPhysical[table_idx] & 0x2) == 0)
	{
		current_directory->tables[table_idx] = newTable;
		current_directory->tablesPhysical[table_idx] = phys | 0x07;
	}
	
	if(need_flushTLB && (current_task->page_directory == current_directory))
		zlox_page_Flush_TLB();

	if(need_copy_page_physical)
		_zlox_copy_page_physical(oldPage.frame*0x1000, 
					newTable->pages[page_idx].frame*0x1000);
}

ZLOX_VOID zlox_double_fault(ZLOX_ISR_REGISTERS * regs)
{
	ZLOX_UNUSED(regs);

	zlox_monitor_write("double fault! following is some info dumps: \n");
	if(tss_entry.esp > 0xE0000000 && tss_entry.esp < tss_entry.esp0)
	{
		zlox_monitor_write("*** kernel stack overflow! *** \n");
	}
	zlox_monitor_write("cs : ");
	zlox_monitor_write_hex(tss_entry.cs);
	zlox_monitor_write("  eip: ");
	zlox_monitor_write_hex(tss_entry.eip);
	zlox_monitor_write("\nss : ");
	zlox_monitor_write_hex(tss_entry.ss);
	zlox_monitor_write("  esp: ");
	zlox_monitor_write_hex(tss_entry.esp);
	zlox_monitor_write("  ebp: ");
	zlox_monitor_write_hex(tss_entry.ebp);
	zlox_monitor_write("\nss0: ");
	zlox_monitor_write_hex(tss_entry.ss0);
	zlox_monitor_write("  esp0: ");
	zlox_monitor_write_hex(tss_entry.esp0);
	zlox_monitor_write("\nds : ");
	zlox_monitor_write_hex(tss_entry.ds);
	zlox_monitor_write("  fs : ");
	zlox_monitor_write_hex(tss_entry.fs);
	zlox_monitor_write("\ngs : ");
	zlox_monitor_write_hex(tss_entry.gs);
	zlox_monitor_write("  es : ");
	zlox_monitor_write_hex(tss_entry.es);
	zlox_monitor_write("\neflags: ");
	zlox_monitor_write_hex(tss_entry.eflags);
	zlox_monitor_write("  cr3: ");
	zlox_monitor_write_hex(tss_entry.cr3);
	zlox_monitor_write("\neax: ");
	zlox_monitor_write_hex(tss_entry.eax);
	zlox_monitor_write("  ebx: ");
	zlox_monitor_write_hex(tss_entry.ebx);
	zlox_monitor_write("\necx: ");
	zlox_monitor_write_hex(tss_entry.ecx);
	zlox_monitor_write("  edx: ");
	zlox_monitor_write_hex(tss_entry.edx);
	zlox_monitor_write("\nesi: ");
	zlox_monitor_write_hex(tss_entry.esi);
	zlox_monitor_write("  edi: ");
	zlox_monitor_write_hex(tss_entry.edi);
	zlox_monitor_write("\n\n");
	ZLOX_PANIC("system halt");
}

ZLOX_VOID zlox_page_fault(ZLOX_ISR_REGISTERS * regs)
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
	present   = !(regs->err_code & 0x1); // Page not present
	rw = regs->err_code & 0x2; // Write operation?
	us = regs->err_code & 0x4; // Processor was in user-mode?
	reserved = regs->err_code & 0x8;	// Overwritten CPU-reserved bits of page entry?
	id = regs->err_code & 0x10; // Caused by an instruction fetch?

	// 当用户权限的程式对只读内存进行写操作时,如果该段内存位于可以复制的内存区段,则进行写时复制
	if (!present && rw && (faulting_address >= 0x8048000 && faulting_address < 0xc0000000))
	{
		zlox_page_copy(faulting_address);
		return ;
	}

print_page_error_message:

	// Output an error message.
	zlox_monitor_write("Page fault! ( ");

	if (present)  
	{
		zlox_monitor_write("present ");
		if(faulting_address > 0xc0000000 && faulting_address < 0xE0000000)
		{
			zlox_monitor_write("and user stack overflow... ");
		}
	}
	if (rw)
		zlox_monitor_write("read-only ");
	if (us) 
		zlox_monitor_write("user-mode ");
	if (reserved) 
		zlox_monitor_write("reserved ");
	if(id)
		zlox_monitor_write("instruction fetch ");

	zlox_monitor_write(") at ");

	zlox_monitor_write_hex(faulting_address);
	zlox_monitor_write(" - EIP: ");
	zlox_monitor_write_hex(regs->eip);
	zlox_monitor_write("\n");

	// 如果只是用户态程式出错，则只需结束掉当前任务，而无需 ZLOX_PANIC 挂掉整个系统
	if(regs->eip >= 0x8048000 && regs->eip < 0xc0000000)
	{
		if(vga_current_mode != ZLOX_VGA_MODE_80X25_TEXT)
		{
			zlox_vga_exit_graphic_mode();
			goto print_page_error_message;
		}
		zlox_exit(-1);
	}

	ZLOX_PANIC("Page fault");
}

static ZLOX_PAGE_TABLE * zlox_clone_table(ZLOX_PAGE_TABLE * src, ZLOX_UINT32 * physAddr, ZLOX_UINT32 needCopy)
{
	// Make a new page table, which is page aligned.
	ZLOX_PAGE_TABLE * table = (ZLOX_PAGE_TABLE *)zlox_kmalloc_ap(sizeof(ZLOX_PAGE_TABLE), physAddr);
	// Ensure that the new table is blank.
	zlox_memset((ZLOX_UINT8 *)table, 0, sizeof(ZLOX_PAGE_TABLE));

	if(src == ZLOX_NULL)
		return table;

	// For every entry in the table...
	ZLOX_SINT32 i;
	for (i = 0; i < 1024; i++)
	{
		// If the source entry has a frame associated with it...
		if (src->pages[i].frame)
		{
			if(needCopy == 1)
			{
				// Get a new frame.
				zlox_alloc_frame(&table->pages[i], 0, 0);
				// Clone the flags from source to destination.
				if (src->pages[i].present) table->pages[i].present = 1;
				if (src->pages[i].rw) table->pages[i].rw = 1;
				if (src->pages[i].user) table->pages[i].user = 1;
				if (src->pages[i].accessed) table->pages[i].accessed = 1;
				if (src->pages[i].dirty) table->pages[i].dirty = 1;
				// Physically copy the data across. This function is in process.s.
				_zlox_copy_page_physical(src->pages[i].frame*0x1000, table->pages[i].frame*0x1000);
			}
			else
			{
				table->pages[i] = src->pages[i];
				table->pages[i].rw = 0; // 读写位清零,用于写时复制
			}
		}
	}
	return table;
}

ZLOX_PAGE_DIRECTORY * zlox_clone_directory(ZLOX_PAGE_DIRECTORY * src , ZLOX_UINT32 needCopy)
{
	ZLOX_UINT32 phys;
	// Make a new page directory and obtain its physical address.
	ZLOX_PAGE_DIRECTORY * dir = (ZLOX_PAGE_DIRECTORY *)zlox_kmalloc_ap(sizeof(ZLOX_PAGE_DIRECTORY), &phys);
	// Ensure that it is blank.
	zlox_memset((ZLOX_UINT8 *)dir, 0, sizeof(ZLOX_PAGE_DIRECTORY));

	// Get the offset of tablesPhysical from the start of the page_directory_t structure.
	//ZLOX_UINT32 offset = (ZLOX_UINT32)dir->tablesPhysical - (ZLOX_UINT32)dir;

	// Then the physical address of dir->tablesPhysical is:
	//dir->physicalAddr = phys + offset;

	// 因为一个8K的虚拟内存空间，可以对应两个非连续的物理内存页面，因此简单的用phys + offset得到的结果不一定正确, 
	// 尤其是在多任务环境下，必须用下面的方法来得出实际的正确的物理内存地址，否则，错误的cr3页表地址会让整个系统直接崩溃掉!
	ZLOX_UINT32 * phy_page = (ZLOX_UINT32 *)zlox_get_page((ZLOX_UINT32)dir->tablesPhysical, 0,
								 current_directory);
	dir->physicalAddr = ((*phy_page) & 0xfffff000);

	// Go through each page table. If the page table is in the kernel directory, do not make a new copy.
	ZLOX_SINT32 i;
	for (i = 0; i < 1024; i++)
	{
		if (!src->tables[i])
			continue;

		if (kernel_directory->tables[i] == src->tables[i])
		{
			// It's in the kernel, so just use the same pointer.
			dir->tables[i] = src->tables[i];
			dir->tablesPhysical[i] = src->tablesPhysical[i];
		}
		else
		{
			// Copy the table.
			ZLOX_UINT32 phys;
			if(needCopy == 1)
			{
				dir->tables[i] = zlox_clone_table(src->tables[i], &phys , 1);
				dir->tablesPhysical[i] = phys | 0x07;
			}
			else
			{
				dir->tables[i] = src->tables[i];
				// 读写位清零,用于写时复制
				dir->tablesPhysical[i] = src->tablesPhysical[i] & (~0x2);
			}
		}
	}
	return dir;
}

ZLOX_VOID zlox_free_directory(ZLOX_PAGE_DIRECTORY * src)
{
	ZLOX_SINT32 i,j;
	ZLOX_PAGE_TABLE * src_table;
	for (i = 0; i < 1024; i++)
	{
		if (!src->tables[i])
			continue;
		if (kernel_directory->tables[i] == src->tables[i])
			continue;
		if((src->tablesPhysical[i] & 0x2) == 0)
			continue;

		src_table = src->tables[i];
		for(j = 0; j < 1024; j++)
		{			
			if (src_table->pages[j].frame && (src_table->pages[j].rw == 1))
			{
				zlox_free_frame(&src_table->pages[j]);
			}
		}
		zlox_kfree(src_table);
	}
	zlox_kfree(src);
}

// 获取内存的位图信息，主要用于系统调用
ZLOX_SINT32 zlox_get_frame_info(ZLOX_UINT32 ** hold_frames,ZLOX_UINT32 * hold_nframes)
{
	(*hold_frames) = frames;
	(*hold_nframes) = nframes;
	return 0;
}

ZLOX_SINT32 zlox_page_Flush_TLB()
{
	// Flush the TLB(translation lookaside buffer) by reading and writing the page directory address again.
	ZLOX_UINT32 pd_addr;
	asm volatile("mov %%cr3, %0" : "=r" (pd_addr));
	asm volatile("mov %0, %%cr3" : : "r" (pd_addr));
	return 0;
}

ZLOX_BOOL zlox_page_alloc(ZLOX_UINT32 alloc_address, ZLOX_BOOL need_newpage)
{
	ZLOX_UINT32 phys;
	ZLOX_PAGE_TABLE * newTable;
	ZLOX_BOOL need_FlushTLB = ZLOX_FALSE;
	alloc_address /= 0x1000;
	ZLOX_UINT32 table_idx = alloc_address / 1024;
	ZLOX_UINT32 page_idx = alloc_address % 1024;
	if( (current_directory->tablesPhysical[table_idx] & 0x2) == 0)
	{
		newTable = zlox_clone_table(current_directory->tables[table_idx],&phys,0);
		need_FlushTLB = ZLOX_TRUE;
	}
	else
		newTable = current_directory->tables[table_idx];

	if(need_newpage)
	{
		if(newTable->pages[page_idx].frame == 0 || newTable->pages[page_idx].rw == 0)
		{
			zlox_alloc_frame_do(&newTable->pages[page_idx], 0, 1);
			newTable->pages[page_idx].rw = newTable->pages[page_idx].present = newTable->pages[page_idx].user = 1;
			newTable->pages[page_idx].accessed = newTable->pages[page_idx].dirty = 0;
			need_FlushTLB = ZLOX_TRUE;
		}
	}

	if( (current_directory->tablesPhysical[table_idx] & 0x2) == 0)
	{
		current_directory->tables[table_idx] = newTable;
		current_directory->tablesPhysical[table_idx] = phys | 0x07;
	}
	return need_FlushTLB;
}

ZLOX_SINT32 zlox_pages_alloc(ZLOX_UINT32 addr, ZLOX_UINT32 size)
{
	ZLOX_BOOL need_FlushTLB = ZLOX_FALSE;
	ZLOX_UINT32 j;
	for( j = addr; j < (addr + size); (j = ZLOX_NEXT_PAGE_START(j)) )
	{
		if((zlox_page_alloc(j, ZLOX_TRUE) == ZLOX_TRUE))
			need_FlushTLB = ZLOX_TRUE;
	}
	if(need_FlushTLB == ZLOX_TRUE && (current_task->page_directory == current_directory))
		zlox_page_Flush_TLB();
	return 0;
}

ZLOX_SINT32 zlox_pages_map(ZLOX_UINT32 dvaddr, ZLOX_PAGE * heap, ZLOX_UINT32 npage)
{
	ZLOX_BOOL need_FlushTLB = ZLOX_FALSE;
	ZLOX_UINT32 d, dtable_idx, dpage_idx, i;
	for( d = dvaddr, i = 0 ; i < npage; (d = ZLOX_NEXT_PAGE_START(d)) , i++)
	{
		dtable_idx = (d / 0x1000) / 1024;
		if((current_directory->tables[dtable_idx] == 0) || 
			((current_directory->tablesPhysical[dtable_idx] & 0x2) == 0))
		{
			zlox_page_alloc(d, ZLOX_FALSE);
		}
		dpage_idx = (d / 0x1000) % 1024;
		current_directory->tables[dtable_idx]->pages[dpage_idx] = heap[i];
		current_directory->tables[dtable_idx]->pages[dpage_idx].rw = 0;
		need_FlushTLB = ZLOX_TRUE;
	}
	if(need_FlushTLB == ZLOX_TRUE && (current_task->page_directory == current_directory))
		zlox_page_Flush_TLB();
	return 0;
}

ZLOX_VOID * zlox_pages_map_to_heap(ZLOX_TASK * task, ZLOX_UINT32 svaddr, ZLOX_UINT32 size, ZLOX_BOOL clear_me_rw,
					ZLOX_UINT32 * ret_npage)
{
	ZLOX_PAGE_DIRECTORY * src_dir = task->page_directory;
	ZLOX_UINT32 s, stable_idx, spage_idx;
	ZLOX_UINT32 npage = 0,i;
	for(s = svaddr ; s < (svaddr + size); (s = ZLOX_NEXT_PAGE_START(s)) )
	{
		npage++;
	}
	ZLOX_PAGE * heap = (ZLOX_PAGE *)zlox_kmalloc(npage * sizeof(ZLOX_PAGE));
	for(i=0, s = svaddr; i < npage ; i++, (s = ZLOX_NEXT_PAGE_START(s)))
	{
		stable_idx = (s / 0x1000) / 1024;
		spage_idx = (s / 0x1000) % 1024;
		heap[i] = src_dir->tables[stable_idx]->pages[spage_idx];
		if(clear_me_rw)
			src_dir->tables[stable_idx]->pages[spage_idx].rw = 0;
	}
	if(clear_me_rw && (current_task->page_directory == src_dir))
		zlox_page_Flush_TLB();
	(*ret_npage) = npage;
	return (ZLOX_VOID *)heap;
}

ZLOX_UINT32 zlox_page_get_dev_map_start(ZLOX_UINT32 need_page_count)
{
	ZLOX_UINT32 count = 0;
	ZLOX_UINT32 start = PAGE_DEV_MAP_START;
	ZLOX_UINT32 i = 0;
	while(1)
	{
		for(i = start;;i += 0x1000)
		{
			ZLOX_PAGE * page = zlox_get_page(i, 0, kernel_directory);
			if(page == ZLOX_NULL || 
				(page->present==0 && page->frame == 0)
				)
			{
				count++;
				if(count >= need_page_count)
					return start;
				else
					continue;
			}
			else
			{
				start = i;
				break;
			}
		}
		start += 0x1000;
		if(start >= PAGE_DEV_MAP_MAX_ADDR)
			break;
	}
	return 0;
}

