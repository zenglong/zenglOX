/*zlox_isr.h Interface and structures for high level interrupt service routines.*/

#ifndef _ZLOX_ISR_H_
#define _ZLOX_ISR_H_

#include "zlox_common.h"

struct _ZLOX_ISR_REGISTERS
{
	ZLOX_UINT32 ds;                  // Data segment selector
    ZLOX_UINT32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
    ZLOX_UINT32 int_no, err_code;    // Interrupt number and error code (if applicable)
    ZLOX_UINT32 eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
}__attribute__((packed));

typedef struct _ZLOX_ISR_REGISTERS ZLOX_ISR_REGISTERS;

#endif //_ZLOX_ISR_H_

