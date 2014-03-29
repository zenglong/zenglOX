/*zlox_common.c Define some global functions*/

#include "zlox_common.h"

// Outputs an integer to the monitor.
extern ZLOX_VOID zlox_monitor_write_dec(ZLOX_UINT32 n);
extern ZLOX_VOID zlox_monitor_write(const ZLOX_CHAR * c);

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

// Copy len bytes from src to dest.
ZLOX_VOID zlox_memcpy(ZLOX_UINT8 *dest, const ZLOX_UINT8 *src, ZLOX_UINT32 len)
{
	const ZLOX_UINT8 *sp = (const ZLOX_UINT8 *)src;
	ZLOX_UINT8 *dp = (ZLOX_UINT8 *)dest;
	if(len == 0)
		return;
	for(; len != 0; len--) 
		*dp++ = *sp++;
}

// Write len copies of val into dest.
ZLOX_VOID zlox_memset(ZLOX_UINT8 *dest, ZLOX_UINT8 val, ZLOX_UINT32 len)
{
	ZLOX_UINT8 *temp = (ZLOX_UINT8 *)dest;
	for ( ; len != 0; len--) 
		*temp++ = val;
}

// Compare two strings. return 0 if they are equal or 1 otherwise.
ZLOX_SINT32 zlox_strcmp(ZLOX_CHAR * str1, ZLOX_CHAR * str2)
{
	ZLOX_SINT32 i = 0;
	ZLOX_SINT32 failed = 0;
	while(str1[i] != '\0' && str2[i] != '\0')
	{
		if(str1[i] != str2[i])
		{
			failed = 1;
			break;
		}
		i++;
	}
	// why did the loop exit?
	if( (str1[i] == '\0' && str2[i] != '\0') || (str1[i] != '\0' && str2[i] == '\0') )
		failed = 1;
  
	return failed;
}

// Copy the NULL-terminated string src into dest, and
// return dest.
ZLOX_CHAR * zlox_strcpy(ZLOX_CHAR * dest, const ZLOX_CHAR * src)
{
	do
	{
		*dest++ = *src++;
	}
	while (*src != 0);
	return dest;
}

ZLOX_SINT32 zlox_strlen(ZLOX_CHAR *src)
{
	ZLOX_SINT32 i = 0;
	while (*src++)
		i++;
	return i;
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

ZLOX_VOID zlox_panic_assert(const ZLOX_CHAR *file, ZLOX_UINT32 line, const ZLOX_CHAR * desc)
{
	// We encountered a massive problem and have to stop.
	asm volatile("cli"); // Disable interrupts.

	zlox_monitor_write("ASSERTION-FAILED(");
	zlox_monitor_write((const ZLOX_CHAR *)desc);
	zlox_monitor_write(") at ");
	zlox_monitor_write((const ZLOX_CHAR *)file);
	zlox_monitor_write(":");
	zlox_monitor_write_dec(line);
	zlox_monitor_write("\n");
	// Halt by going into an infinite loop.
	for(;;)
		;
}

