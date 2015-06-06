/*zlox_common.h the common header*/

#ifndef _ZLOX_COMMON_H_
#define _ZLOX_COMMON_H_

// Some nice typedefs, to standardise sizes across platforms.
// These typedefs are written for 32-bit X86.
typedef unsigned long long int ZLOX_UINT64;
typedef long long int ZLOX_SINT64;
typedef unsigned int ZLOX_UINT32;
typedef	int ZLOX_SINT32;
typedef unsigned short ZLOX_UINT16;
typedef	short ZLOX_SINT16;
typedef unsigned char ZLOX_UINT8;
typedef char ZLOX_SINT8;
typedef char ZLOX_CHAR;
typedef void ZLOX_VOID;
typedef ZLOX_UINT8 ZLOX_BOOL;
typedef float ZLOX_FLOAT;

#define ZLOX_FALSE 0
#define ZLOX_TRUE 1
#define ZLOX_NULL 0
#define ZLOX_PANIC(msg) zlox_panic(msg, __FILE__, __LINE__);
#define ZLOX_ASSERT(b) ((b) ? (void)0 : zlox_panic_assert(__FILE__, __LINE__, #b))

#define ZLOX_UNUSED(x) (void)(x)

#define ZLOX_PACKED __attribute__((packed))

// Write a byte out to the specified port.
ZLOX_VOID zlox_outb(ZLOX_UINT16 port,ZLOX_UINT8 value);
// Write a word(2 byte) out to the specified port.
ZLOX_VOID zlox_outw(ZLOX_UINT16 port,ZLOX_UINT16 value);
// Write a dword(4 byte) out to the specified port.
ZLOX_VOID zlox_outl(ZLOX_UINT16 port, ZLOX_UINT32 value);
// writes Count words (16-bit) from memory location Data to I/O port
ZLOX_VOID zlox_outsw(ZLOX_UINT16 port, ZLOX_UINT16 * Data, ZLOX_UINT32 Count);

//read a byte from the specified port.
ZLOX_UINT8 zlox_inb(ZLOX_UINT16 port);
//read a word(2 byte) from the specified port.
ZLOX_UINT16 zlox_inw(ZLOX_UINT16 port);
//read a dword(4 byte) from the specified port.
ZLOX_UINT32 zlox_inl(ZLOX_UINT16 port);
//reads Count words (16-bit) from I/O port to memory location Data
ZLOX_VOID zlox_insw(ZLOX_UINT16 port, ZLOX_UINT16 * Data, ZLOX_UINT32 Count);

// Copy len bytes from src to dest.
ZLOX_VOID zlox_memcpy(ZLOX_UINT8 *dest, const ZLOX_UINT8 *src, ZLOX_UINT32 len);
// 以反向，向低地址方向进行拷贝
ZLOX_VOID zlox_reverse_memcpy(ZLOX_UINT8 *dest, const ZLOX_UINT8 *src, ZLOX_UINT32 len);
// Write len copies of val into dest.
ZLOX_VOID zlox_memset(ZLOX_UINT8 *dest, ZLOX_UINT8 val, ZLOX_UINT32 len);
// Compare two strings. return 0 if they are equal or 1 otherwise.
ZLOX_SINT32 zlox_strcmp(ZLOX_CHAR * str1, ZLOX_CHAR * str2);
ZLOX_SINT32 zlox_strcmpn(ZLOX_CHAR * s1, ZLOX_CHAR * s2, ZLOX_SINT32 n);
ZLOX_SINT32 zlox_memcmp(ZLOX_UINT8 * s1, ZLOX_UINT8 * s2, ZLOX_UINT32 n);
// Copy the NULL-terminated string src into dest, and
// return dest.
ZLOX_CHAR * zlox_strcpy(ZLOX_CHAR * dest, const ZLOX_CHAR * src);
ZLOX_SINT32 zlox_strlen(ZLOX_CHAR *src);

extern ZLOX_VOID zlox_panic(const ZLOX_CHAR *message, const ZLOX_CHAR *file, ZLOX_UINT32 line);
extern ZLOX_VOID zlox_panic_assert(const ZLOX_CHAR *file, ZLOX_UINT32 line, const ZLOX_CHAR * desc);

#endif // _ZLOX_COMMON_H_

