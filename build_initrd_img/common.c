// common.c -- 定义一些通用的函数

#include "common.h"

UINT32 _commontest_gl = 3; // debug

// Copy len bytes from src to dest.
VOID memcpy(UINT8 *dest, const UINT8 *src, UINT32 len)
{
	_commontest_gl = 3; // debug

	const UINT8 *sp = (const UINT8 *)src;
	UINT8 *dp = (UINT8 *)dest;
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
VOID reverse_memcpy(UINT8 *dest, const UINT8 *src, UINT32 len)
{
	const UINT8 *sp = (const UINT8 *)src;
	UINT8 *dp = (UINT8 *)dest;
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
VOID memset(UINT8 *dest, UINT8 val, UINT32 len)
{
	//UINT8 *temp = (UINT8 *)dest;
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
SINT32 strcmp(CHAR * str1, CHAR * str2)
{
	/*SINT32 i = 0;
	SINT32 failed = 0;
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

	SINT32 retval = 0;

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

// Copy the NULL-terminated string src into dest, and
// return dest.
CHAR * strcpy(CHAR * dest, const CHAR * src)
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

SINT32 strlen(CHAR *src)
{
	SINT32 i = -1;
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

// convert string to unsigned int
UINT32 strToUInt(CHAR *c)
{
	UINT32 result = 0; 
	while(*c != 0)
	{
		result *= 10;           
		result += (*c - 48);
		c++;
	}
	return result;
}

