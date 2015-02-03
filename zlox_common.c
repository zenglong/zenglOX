/*zlox_common.c Define some global functions*/

#include "zlox_common.h"

// Outputs an integer to the monitor.
extern ZLOX_VOID zlox_monitor_write_dec(ZLOX_UINT32 n);
extern ZLOX_VOID zlox_monitor_write(const ZLOX_CHAR * c);
extern ZLOX_VOID zlox_monitor_set_color_space(ZLOX_BOOL flag, ZLOX_UINT32 color, ZLOX_UINT32 backColour,
			ZLOX_SINT32 w_space, ZLOX_SINT32 h_space);

// Write a byte out to the specified port.
ZLOX_VOID zlox_outb(ZLOX_UINT16 port,ZLOX_UINT8 value)
{
	asm volatile("outb %1,%0"::"dN" (port),"a" (value));
	//asm volatile("outb %1,%0"::"d" (port),"a" (value));
	//asm volatile("outb %%al,%%dx"::"d" (port),"a" (value));
	//asm volatile("outb %%al,(%%dx)"::"d" (port),"a" (value));
}

// Write a word(2 byte) out to the specified port.
ZLOX_VOID zlox_outw(ZLOX_UINT16 port,ZLOX_UINT16 value)
{
	asm volatile("outw %1,%0"::"dN" (port),"a" (value));
}

// Write a dword(4 byte) out to the specified port.
ZLOX_VOID zlox_outl(ZLOX_UINT16 port, ZLOX_UINT32 value)
{
	asm volatile ("outl %1, %0" : : "dN" (port), "a" (value));
}

// writes Count words (16-bit) from memory location Data to I/O port
ZLOX_VOID zlox_outsw(ZLOX_UINT16 port, ZLOX_UINT16 * Data, ZLOX_UINT32 Count)
{
	for(; Count; Count--,Data++)
		asm volatile("outw %1,%0"::"dN" (port),"a" (*Data));
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

//read a dword(4 byte) from the specified port.
ZLOX_UINT32 zlox_inl(ZLOX_UINT16 port)
{
	ZLOX_UINT32 ret;
	asm volatile ("inl %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

//reads Count words (16-bit) from I/O port to memory location Data
ZLOX_VOID zlox_insw(ZLOX_UINT16 port, ZLOX_UINT16 * Data, ZLOX_UINT32 Count)
{
	for(; Count; Count--,Data++)
		asm volatile("inw %1,%0":"=a" (*Data):"dN" (port));
}

// Copy len bytes from src to dest.
ZLOX_VOID zlox_memcpy(ZLOX_UINT8 *dest, const ZLOX_UINT8 *src, ZLOX_UINT32 len)
{
	const ZLOX_UINT8 *sp = (const ZLOX_UINT8 *)src;
	ZLOX_UINT8 *dp = (ZLOX_UINT8 *)dest;
	if(len == 0)
		return;
	//for(; len != 0; len--) 
	//	*dp++ = *sp++;
	
	// 使用movsl和movsb来提高内存拷贝的效率
	asm volatile (
	"pushf\n\t"
	"movl %%edx,%%ecx\n\t"
	"shrl $2, %%ecx\n\t"	// 长度除以4,得到的商使用movsl进行拷贝,因为每次movsl可以拷贝4个字节
	"cld\n\t"
	"rep movsl\n\t"
	"movl %%edx, %%ecx\n\t"
	"andl $3, %%ecx\n\t"	// and运算得到除以4的余数,余数使用movsb每次拷贝一个字节
	"rep movsb\n\t"
	"popf"			// 恢复eflags里的标志,主要是将上面cld清零的DF标志恢复为原始值
	::"S"(sp),"D"(dp),"d"(len):"%ecx");
}

// 以反向，向低地址方向进行拷贝
ZLOX_VOID zlox_reverse_memcpy(ZLOX_UINT8 *dest, const ZLOX_UINT8 *src, ZLOX_UINT32 len)
{
	const ZLOX_UINT8 *sp = (const ZLOX_UINT8 *)src;
	ZLOX_UINT8 *dp = (ZLOX_UINT8 *)dest;
	if(len == 0)
		return;
	
	// 使用movsl和movsb来提高内存拷贝的效率
	asm volatile (
	"pushf\n\t"
	"movl %%edx,%%ecx\n\t"
	"andl $3, %%ecx\n\t"	// and运算得到除以4的余数,余数使用movsb每次拷贝一个字节
	"std\n\t"
	"rep movsb\n\t"
	"movl %%edx, %%ecx\n\t"
	"shrl $2, %%ecx\n\t"	// 长度除以4,得到的商使用movsl进行拷贝,因为每次movsl可以拷贝4个字节
	"subl $3,%%edi\n\t"	// 将源和目标地址减去3，为movsl做准备，之前的movsb已经自动减了1个字节
	"subl $3,%%esi\n\t"
	"rep movsl\n\t"
	"popf"			// 恢复eflags里的标志,主要是将上面std设置的DF标志恢复为原始值
	::"S"(sp),"D"(dp),"d"(len):"%ecx");
}

// Write len copies of val into dest.
ZLOX_VOID zlox_memset(ZLOX_UINT8 *dest, ZLOX_UINT8 val, ZLOX_UINT32 len)
{
	//ZLOX_UINT8 *temp = (ZLOX_UINT8 *)dest;
	//for ( ; len != 0; len--) 
	//	*temp++ = val;
	
	asm volatile (
	"pushf\n\t"
	"movb %1,%%al\n\t"
	"cld\n\t"
	"rep stosb\n\t"	// 使用rep stosb根据ecx里的长度信息,将al里的值填充到edi指向的一片内存里
	"popf"
	::"D"(dest),"m"(val),"c"(len):"%eax");
}

// Compare two strings. return 0 if they are equal or 1 otherwise.
ZLOX_SINT32 zlox_strcmp(ZLOX_CHAR * str1, ZLOX_CHAR * str2)
{
	/*ZLOX_SINT32 i = 0;
	ZLOX_SINT32 failed = 0;
	while(str1[i] != '\0' && str2[i] != '\0')
	{
		if(str1[i] != str2[i])
		{
			failed = 1;
			break;
		}
		i++;
	}*/
	// why did the loop exit?
	/*if( (str1[i] == '\0' && str2[i] != '\0') || (str1[i] != '\0' && str2[i] == '\0') )
		failed = 1;
  	*/

	ZLOX_SINT32 retval = 0;

	asm volatile (
		"pushf\n\t"
		"cld\n"
	"start:\n\t"		
		"lodsb\n\t"		// 将esi的第一个字节装入寄存器AL，同时esi的值加1
		"scasb\n\t"		// 将edi的第一个字节和AL相减，同时edi的值加1
		"jne not_eq\n\t"	// 不相等则跳转到not_eq
		"testb %b0,%b0\n\t"	// 判断是否到达字符串的最后一个NULL字符
		"jne start\n\t"		// 如果到了最后一个NULL字符,则说明两个字符串完全相等
		"xorl %0,%0\n\t"	// 字符串相等则将结果设置为0
		"jmp end\n"		
	"not_eq:\n\t"
		"movl $1,%0\n"		// 两字符串不相等,则将结果设置为1
	"end:\n\t"
		"popf"
		:"=a"(retval):"S"(str1),"D"(str2));
	return retval;
}

ZLOX_SINT32 zlox_strcmpn(ZLOX_CHAR * s1, ZLOX_CHAR * s2, ZLOX_SINT32 n)
{
	while (--n >= 0 && *s1 == *s2++)
		if (*s1++ == '\0')
			return 0;
	return(n<0 ? 0 : *s1 - *--s2);
}

// Copy the NULL-terminated string src into dest, and
// return dest.
ZLOX_CHAR * zlox_strcpy(ZLOX_CHAR * dest, const ZLOX_CHAR * src)
{
	/*do
	{
		*dest++ = *src++;
	}
	while (*src != 0);*/

	asm volatile (
	"pushf\n\t"
	"movl %1,%%edi\n\t"		// get source string
	"movl %%edi,%%esi\n\t"		// copy to esi and edi
	"movl $-1,%%ecx\n\t"
	"xorb %b0,%b0\n\t"		// search for null at end of source string
	"cld\n\t"
	"repne scasb\n\t"		// scan one character past null
	"notl %%ecx\n\t"		// ecx = number of characters including null
	"movl %2,%%edi\n\t"		// get destination buffer
	"movl %%edi,%0\n\t"		// copy to eax for return value
	"movl %%ecx,%%edx\n\t"		// save count
	"shrl $2, %%ecx\n\t"		// calculate longword count
	"cld\n\t"
	"rep movsl\n\t"			// copy longwords
	"movl %%edx,%%ecx\n\t"		// get back count
	"andl $3, %%ecx\n\t"		// calculate remainder byte count (0-3)
	"rep movsb\n\t"			// copy remaining bytes
	"popf"
	:"=a"(dest):"m"(src),"m"(dest):"%esi","%edi","%ecx","%edx");
	return dest;
}

ZLOX_SINT32 zlox_strlen(ZLOX_CHAR *src)
{
	ZLOX_SINT32 i = -1;
	/*while (*src++)
		i++;*/

	asm volatile (
	"pushf\n\t"
	"xorb %%al,%%al\n\t"		// search for null at end of source string
	"cld\n\t"
	"repne scasb\n\t"		// scan one character past null
	"notl %%ecx\n\t"		// ecx = number of characters including null
	"decl %%ecx\n\t"		// ecx - 1 = string length (not including null)
	"popf"
	:"+c"(i):"D"(src):"%eax");
	return i;
}

ZLOX_VOID zlox_panic_shutdown()
{
	zlox_monitor_set_color_space(ZLOX_TRUE, 0xFFFFFFFF, 0xFFFF0000, 0, 0);
	zlox_monitor_write("zenglOX is shutdown now , you can power off!\n");
	return;
}

ZLOX_VOID zlox_panic(const ZLOX_CHAR *message, const ZLOX_CHAR *file, ZLOX_UINT32 line)
{
	// We encountered a massive problem and have to stop.
	asm volatile("cli"); // Disable interrupts.

	zlox_monitor_set_color_space(ZLOX_TRUE, 0xFFFFFFFF, 0xFFFF0000, 0, 0);

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

	zlox_monitor_set_color_space(ZLOX_TRUE, 0xFFFFFFFF, 0xFFFF0000, 0, 0);

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

