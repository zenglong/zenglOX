/*zlox_isr.c High level interrupt service routines and interrupt request handlers.*/

#include "zlox_common.h"
#include "zlox_isr.h"
#include "zlox_monitor.h"

ZLOX_ISR_CALLBACK interrupt_callbacks[256];

ZLOX_VOID zlox_register_interrupt_callback(ZLOX_UINT8 n, ZLOX_ISR_CALLBACK callback)
{
    interrupt_callbacks[n] = callback;
}

// This gets called from our ASM interrupt handler stub.
ZLOX_VOID zlox_isr_handler(ZLOX_ISR_REGISTERS regs)
{
	zlox_monitor_write("zenglox recieved interrupt: ");
	zlox_monitor_write_dec(regs.int_no);
	zlox_monitor_put('\n');
}

// This gets called from our ASM interrupt handler stub.
ZLOX_VOID zlox_irq_handler(ZLOX_ISR_REGISTERS regs)
{
	// Send an EOI (end of interrupt) signal to the PICs.
    // If this interrupt involved the slave.
	if (regs.int_no >= ZLOX_IRQ8)
    {
        // Send reset signal to slave.
        zlox_outb(0xA0, 0x20);
    }
	// Send reset signal to master. (As well as slave, if necessary).
    zlox_outb(0x20, 0x20);
	
	//zlox_monitor_write("zenglox recieved irq: ");
	//zlox_monitor_write_dec(regs.int_no);
	//zlox_monitor_put('\n');

	if (interrupt_callbacks[regs.int_no] != 0)
    {
        ZLOX_ISR_CALLBACK callback = interrupt_callbacks[regs.int_no];
        callback(regs);
    }
}

