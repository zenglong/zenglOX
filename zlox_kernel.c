/*zlox_kernel.c Defines the C-code kernel entry point, calls initialisation routines.*/

#include "zlox_monitor.h"

//zenglOX kernel main entry
ZLOX_SINT32 zlox_kernel_main(ZLOX_VOID * mboot_ptr)
{
	// Initialise the screen (by clearing it)
	zlox_monitor_clear();
	 // Write out a sample string
	zlox_monitor_write("hello world!\nwelcome to zenglOX!");
	return 0;
}

