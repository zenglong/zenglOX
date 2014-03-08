/*zlox_kernel.c Defines the C-code kernel entry point, calls initialisation routines.*/

#include "zlox_monitor.h"
#include "zlox_descriptor_tables.h"
#include "zlox_time.h"
#include "zlox_paging.h"

//zenglOX kernel main entry
ZLOX_SINT32 zlox_kernel_main(ZLOX_VOID * mboot_ptr)
{
	// init gdt and idt
	zlox_init_descriptor_tables();	

	// Initialise the screen (by clearing it)
	zlox_monitor_clear();

	zlox_init_paging();

	// Write out a sample string
	zlox_monitor_write("hello world!\nwelcome to zenglOX v0.0.5!\n");

	ZLOX_UINT32 *ptr = (ZLOX_UINT32*)0xA0000000;
    ZLOX_UINT32 do_page_fault = *ptr;
	*ptr = do_page_fault;

	/*asm volatile("int $0x3");
    asm volatile("int $0x4");
	
	asm volatile("sti");
	zlox_init_timer(50);*/

	return (ZLOX_SINT32)mboot_ptr;
}

