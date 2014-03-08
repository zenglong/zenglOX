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
static ZLOX_UINT32 zlox_test_frame(ZLOX_UINT32 frame_addr)
{
    ZLOX_UINT32 frame = frame_addr/0x1000;
    ZLOX_UINT32 idx = ZLOX_INDEX_FROM_BIT(frame);
    ZLOX_UINT32 off = ZLOX_OFFSET_FROM_BIT(frame);
    return (frames[idx] & (0x1 << off));
}

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
        page->frame = 0x0;
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

	// We need to identity map (phys addr = virt addr) from
    // 0x0 to the end of used memory, so we can access this
    // transparently, as if paging wasn't enabled.
    // NOTE that we use a while loop here deliberately.
    // inside the loop body we actually change placement_address
    // by calling kmalloc(). A while loop causes this to be
    // computed on-the-fly rather than once at the start.
    i = 0;
    while (i < placement_address)
    {
        // Kernel code is readable but not writeable from userspace.
        zlox_alloc_frame( zlox_get_page(i, 1, kernel_directory), 0, 0);
        i += 0x1000;
    }
	// Before we enable paging, we must register our page fault handler.
	zlox_register_interrupt_callback(14,zlox_page_fault);

	// Now, enable paging!
	zlox_switch_page_directory(kernel_directory);
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
    rw = regs.err_code & 0x2;           // Write operation?
    us = regs.err_code & 0x4;           // Processor was in user-mode?
    reserved = regs.err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
    id = regs.err_code & 0x10;          // Caused by an instruction fetch?

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

