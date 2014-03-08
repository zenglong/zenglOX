/*zlox_isr.h Interface and structures for high level interrupt service routines.*/

#ifndef _ZLOX_ISR_H_
#define _ZLOX_ISR_H_

#include "zlox_common.h"

// A few defines to make life a little easier
#define ZLOX_IRQ0 32
#define ZLOX_IRQ1 33
#define ZLOX_IRQ2 34
#define ZLOX_IRQ3 35
#define ZLOX_IRQ4 36
#define ZLOX_IRQ5 37
#define ZLOX_IRQ6 38
#define ZLOX_IRQ7 39
#define ZLOX_IRQ8 40
#define ZLOX_IRQ9 41
#define ZLOX_IRQ10 42
#define ZLOX_IRQ11 43
#define ZLOX_IRQ12 44
#define ZLOX_IRQ13 45
#define ZLOX_IRQ14 46
#define ZLOX_IRQ15 47

struct _ZLOX_ISR_REGISTERS
{
	ZLOX_UINT32 ds;                  // Data segment selector
    ZLOX_UINT32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
    ZLOX_UINT32 int_no, err_code;    // Interrupt number and error code (if applicable)
    ZLOX_UINT32 eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
}__attribute__((packed));

typedef struct _ZLOX_ISR_REGISTERS ZLOX_ISR_REGISTERS;

// Enables registration of callbacks for interrupts or IRQs.
// For IRQs, to ease confusion, use the #defines above as the
// first parameter.
typedef void (*ZLOX_ISR_CALLBACK)(ZLOX_ISR_REGISTERS);

ZLOX_VOID zlox_register_interrupt_callback(ZLOX_UINT8 n, ZLOX_ISR_CALLBACK callback);

#endif //_ZLOX_ISR_H_

