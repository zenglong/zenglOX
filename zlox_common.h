/*zlox_common.h the common header*/

#ifndef _ZLOX_COMMON_H_
#define _ZLOX_COMMON_H_

// Some nice typedefs, to standardise sizes across platforms.
// These typedefs are written for 32-bit X86.
typedef unsigned int ZLOX_UINT32;
typedef	int ZLOX_SINT32;
typedef unsigned short ZLOX_UINT16;
typedef	short ZLOX_SINT16;
typedef unsigned char ZLOX_UINT8;
typedef char ZLOX_SINT8;
typedef char ZLOX_CHAR;
typedef void ZLOX_VOID;

// Write a byte out to the specified port.
ZLOX_VOID zlox_outb(ZLOX_UINT16 port,ZLOX_UINT8 value);
//read a byte from the specified port.
ZLOX_UINT8 zlox_inb(ZLOX_UINT16 port);
//read a word(2 byte) from the specified port.
ZLOX_UINT16 zlox_inw(ZLOX_UINT16 port);
// Write len copies of val into dest.
ZLOX_VOID zlox_memset(ZLOX_UINT8 *dest, ZLOX_UINT8 val, ZLOX_UINT32 len);

#endif // _ZLOX_COMMON_H_

