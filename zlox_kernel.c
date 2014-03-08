/*zlox_kernel.c Defines the C-code kernel entry point, calls initialisation routines.*/

#include "zlox_monitor.h"
#include "zlox_descriptor_tables.h"
#include "zlox_time.h"

//zenglOX kernel main entry
ZLOX_SINT32 zlox_kernel_main(ZLOX_VOID * mboot_ptr)
{
	// init gdt and idt
	zlox_init_descriptor_tables();	

	// Initialise the screen (by clearing it)
	zlox_monitor_clear();
	 // Write out a sample string
	zlox_monitor_write("hello world!\nwelcome to zenglOX v0.0.4!\n");

	asm volatile("int $0x3");
    asm volatile("int $0x4");
	
	asm volatile("sti");
	zlox_init_timer(50);

	return (ZLOX_SINT32)mboot_ptr;
}

