/*zlox_isr.c High level interrupt service routines and interrupt request handlers.*/

#include "zlox_common.h"
#include "zlox_isr.h"
#include "zlox_monitor.h"

// This gets called from our ASM interrupt handler stub.
ZLOX_VOID zlox_isr_handler(ZLOX_ISR_REGISTERS regs)
{
	zlox_monitor_write("zenglox recieved interrupt: ");
	zlox_monitor_write_dec(regs.int_no);
	zlox_monitor_put('\n');
}

