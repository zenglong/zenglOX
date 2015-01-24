/*common.h the common header*/

#ifndef _COMMON_H_
#define _COMMON_H_

// Some nice typedefs, to standardise sizes across platforms.
// These typedefs are written for 32-bit X86.
typedef unsigned int UINT32;
typedef	int SINT32;
typedef unsigned short UINT16;
typedef	short SINT16;
typedef unsigned char UINT8;
typedef char SINT8;
typedef char CHAR;
typedef void VOID;
typedef UINT8 BOOL;
typedef float FLOAT;

#define FALSE 0
#define TRUE 1
#define NULL 0

#define UNUSED(x) (void)(x)

// Copy len bytes from src to dest.
VOID * memcpy(VOID * dest, const VOID * src, UINT32 len);
// 以反向，向低地址方向进行拷贝
VOID * reverse_memcpy(VOID * dest, const VOID * src, UINT32 len);
// Write len copies of val into dest.
VOID * memset(VOID * dest, UINT8 val, UINT32 len);
// Compare two strings. return 0 if they are equal or 1 otherwise.
SINT32 strcmp(CHAR * str1, CHAR * str2);
SINT32 strcmpn(CHAR * s1, CHAR * s2, SINT32 n);
// Copy the NULL-terminated string src into dest, and
// return dest.
CHAR * strcpy(CHAR * dest, const CHAR * src);
SINT32 strlen(const CHAR *src);
// convert string to unsigned int
UINT32 strToUInt(CHAR *c);
BOOL strIsNum(CHAR *c);

#endif // _COMMON_H_

