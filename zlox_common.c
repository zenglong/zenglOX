/*zlox_common.c Define some global functions*/

#include "zlox_common.h"

// Outputs an integer to the monitor.
extern ZLOX_VOID zlox_monitor_write_dec(ZLOX_UINT32 n);

// Write a byte out to the specified port.
ZLOX_VOID zlox_outb(ZLOX_UINT16 port,ZLOX_UINT8 value)
{
	asm volatile("outb %1,%0"::"dN" (port),"a" (value));
	//asm volatile("outb %1,%0"::"d" (port),"a" (value));
	//asm volatile("outb %%al,%%dx"::"d" (port),"a" (value));
	//asm volatile("outb %%al,(%%dx)"::"d" (port),"a" (value));
}

//read a byte from the specified port.
ZLOX_UINT8 zlox_inb(ZLOX_UINT16 port)
{
	ZLOX_UINT8 ret;
	asm volatile("inb %1,%0":"=a" (ret):"dN" (port));
	return ret;
}

//read a word(2 byte) from the specified port.
ZLOX_UINT16 zlox_inw(ZLOX_UINT16 port)
{
	ZLOX_UINT16 ret;
	asm volatile("inw %1,%0":"=a" (ret):"dN" (port));
	return ret;
}

// Write len copies of val into dest.
ZLOX_VOID zlox_memset(ZLOX_UINT8 *dest, ZLOX_UINT8 val, ZLOX_UINT32 len)
{
    ZLOX_UINT8 *temp = (ZLOX_UINT8 *)dest;
    for ( ; len != 0; len--) 
		*temp++ = val;
}

ZLOX_VOID zlox_panic(const ZLOX_CHAR *message, const ZLOX_CHAR *file, ZLOX_UINT32 line)
{
    // We encountered a massive problem and have to stop.
    asm volatile("cli"); // Disable interrupts.

    zlox_monitor_write("PANIC(");
    zlox_monitor_write(message);
    zlox_monitor_write(") at ");
    zlox_monitor_write(file);
    zlox_monitor_write(":");
    zlox_monitor_write_dec(line);
    zlox_monitor_write("\n");
    // Halt by going into an infinite loop.
    for(;;)
		;
}

