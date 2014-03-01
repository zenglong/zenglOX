/*zlox_common.c Define some global functions*/

#include "zlox_common.h"

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

