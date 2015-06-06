/* zlox_timer.c Initialises the PIT, and handles clock updates. */

#include "zlox_time.h"
#include "zlox_isr.h"
#include "zlox_monitor.h"
#include "zlox_task.h"
#include "zlox_usb.h"

ZLOX_UINT32 tick = 0;
ZLOX_UINT32 timer_frequency = 0;
ZLOX_BOOL timer_switch_flag = ZLOX_TRUE;

ZLOX_VOID zlox_test_ps2_keyboard();

static ZLOX_VOID zlox_timer_callback(/*ZLOX_ISR_REGISTERS * regs*/)
{
	tick++;
	if(timer_switch_flag)
	{
		zlox_usb_poll();
		zlox_switch_task();
	}
}

ZLOX_VOID zlox_timer_sleep(ZLOX_UINT32 Ticks, ZLOX_BOOL need_switch)
{
	ZLOX_UINT32 ETicks = 0;
	ETicks = Ticks + tick;
	ZLOX_BOOL orig_timer_switch_flag = timer_switch_flag;
	if(need_switch == ZLOX_FALSE)
		timer_switch_flag = ZLOX_FALSE;
	else
		timer_switch_flag = ZLOX_TRUE;
	asm("pushf; sti");
	do
	{
		asm("pause");
	}while(ETicks > tick);
	asm("popf");
	timer_switch_flag = orig_timer_switch_flag;
}

ZLOX_UINT32 zlox_timer_get_tick()
{
	return tick;
}

ZLOX_UINT32 zlox_timer_get_frequency()
{
	return timer_frequency;
}

ZLOX_VOID zlox_init_timer(ZLOX_UINT32 frequency)
{
	ZLOX_UINT32 divisor;
	ZLOX_UINT8 l;
	ZLOX_UINT8 h;
	// Firstly, register our timer callback.
	zlox_register_interrupt_callback(ZLOX_IRQ0,&zlox_timer_callback);

	// The value we send to the PIT is the value to divide it's input clock
	// (1193180 Hz) by, to get our required frequency. Important to note is
	// that the divisor must be small enough to fit into 16-bits.
	divisor = 1193180 / frequency;

	// Send the command byte.
	zlox_outb(0x43, 0x36);

	// Divisor has to be sent byte-wise, so split here into upper/lower bytes.
	l = (ZLOX_UINT8)(divisor & 0xFF);
	h = (ZLOX_UINT8)( (divisor>>8) & 0xFF);

	// Send the frequency divisor.
	zlox_outb(0x40, l);
	zlox_outb(0x40, h);

	timer_frequency = frequency;
}

