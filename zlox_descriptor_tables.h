/*zlox_descriptor_tables.h Defines the interface for initialising the GDT and IDT.
  Also defines needed structures.*/

#ifndef _ZLOX_DESCRIPTOR_TABLES_H_
#define _ZLOX_DESCRIPTOR_TABLES_H_

#include "zlox_common.h"

#define ZLOX_GDT_ENTRY_NUMBER 7

// Initialisation function is publicly accessible.
ZLOX_VOID zlox_init_descriptor_tables();

// Allows the kernel stack in the TSS to be changed.
ZLOX_VOID zlox_set_kernel_stack(ZLOX_UINT32 stack);

// This structure contains the value of one GDT entry.
// We use the attribute 'packed' to tell GCC not to change
// any of the alignment in the structure.
struct _ZLOX_GDT_ENTRY
{
	ZLOX_UINT16 limit_low;	// The lower 16 bits of the limit.
	ZLOX_UINT16 base_low;	// The lower 16 bits of the base.
	ZLOX_UINT8 base_middle;	// The next 8 bits of the base.
	ZLOX_UINT8 access;		// Access flags, determine what ring this segment can be used in.
	ZLOX_UINT8 granularity;
	ZLOX_UINT8 base_high;	// The last 8 bits of the base.
}__attribute__((packed));

typedef struct _ZLOX_GDT_ENTRY ZLOX_GDT_ENTRY;

// This struct describes a GDT pointer. It points to the start of
// our array of GDT entries, and is in the format required by the
// lgdt instruction.
struct _ZLOX_GDT_PTR
{
	ZLOX_UINT16 limit;		// The upper 16 bits of all selector limits.
	ZLOX_UINT32 base;		// The address of the first ZLOX_GDT_ENTRY struct.
}__attribute__((packed));

typedef struct _ZLOX_GDT_PTR ZLOX_GDT_PTR;

struct _ZLOX_IDT_ENTRY
{
	ZLOX_UINT16 base_lo;	// The lower 16 bits of the address to jump to when this interrupt fires.
	ZLOX_UINT16 sel;		// Kernel segment selector.
	ZLOX_UINT8 always0;		// This must always be zero.
	ZLOX_UINT8 flags;		// More flags.
	ZLOX_UINT16 base_hi;	// The upper 16 bits of the address to jump to.
}__attribute__((packed));

typedef struct _ZLOX_IDT_ENTRY ZLOX_IDT_ENTRY;

struct _ZLOX_IDT_PTR
{
	ZLOX_UINT16 limit;
	ZLOX_UINT32 base;		// The address of the first element in our ZLOX_IDT_ENTRY array.
}__attribute__((packed));

typedef struct _ZLOX_IDT_PTR ZLOX_IDT_PTR;

// A struct describing a Task State Segment.
struct _ZLOX_TSS_ENTRY
{
    ZLOX_UINT32 prev_tss;   // The previous TSS - if we used hardware task switching this would form a linked list.
    ZLOX_UINT32 esp0;       // The stack pointer to load when we change to kernel mode.
    ZLOX_UINT32 ss0;        // The stack segment to load when we change to kernel mode.
    ZLOX_UINT32 esp1;       // Unused...
    ZLOX_UINT32 ss1;
    ZLOX_UINT32 esp2;  
    ZLOX_UINT32 ss2;   
    ZLOX_UINT32 cr3;   
    ZLOX_UINT32 eip;   
    ZLOX_UINT32 eflags;
    ZLOX_UINT32 eax;
    ZLOX_UINT32 ecx;
    ZLOX_UINT32 edx;
    ZLOX_UINT32 ebx;
    ZLOX_UINT32 esp;
    ZLOX_UINT32 ebp;
    ZLOX_UINT32 esi;
    ZLOX_UINT32 edi;
    ZLOX_UINT32 es;         // The value to load into ES when we change to kernel mode.
    ZLOX_UINT32 cs;         // The value to load into CS when we change to kernel mode.
    ZLOX_UINT32 ss;         // The value to load into SS when we change to kernel mode.
    ZLOX_UINT32 ds;         // The value to load into DS when we change to kernel mode.
    ZLOX_UINT32 fs;         // The value to load into FS when we change to kernel mode.
    ZLOX_UINT32 gs;         // The value to load into GS when we change to kernel mode.
    ZLOX_UINT32 ldt;        // Unused...
    ZLOX_UINT16 trap;
    ZLOX_UINT16 iomap_base;

} __attribute__((packed));

typedef struct _ZLOX_TSS_ENTRY ZLOX_TSS_ENTRY;

// These extern directives let us access the addresses of our ASM ISR handlers.
extern void _zlox_isr_0 ();
extern void _zlox_isr_1 ();
extern void _zlox_isr_2 ();
extern void _zlox_isr_3 ();
extern void _zlox_isr_4 ();
extern void _zlox_isr_5 ();
extern void _zlox_isr_6 ();
extern void _zlox_isr_7 ();
extern void _zlox_isr_8 ();
extern void _zlox_isr_9 ();
extern void _zlox_isr_10();
extern void _zlox_isr_11();
extern void _zlox_isr_12();
extern void _zlox_isr_13();
extern void _zlox_isr_14();
extern void _zlox_isr_15();
extern void _zlox_isr_16();
extern void _zlox_isr_17();
extern void _zlox_isr_18();
extern void _zlox_isr_19();
extern void _zlox_isr_20();
extern void _zlox_isr_21();
extern void _zlox_isr_22();
extern void _zlox_isr_23();
extern void _zlox_isr_24();
extern void _zlox_isr_25();
extern void _zlox_isr_26();
extern void _zlox_isr_27();
extern void _zlox_isr_28();
extern void _zlox_isr_29();
extern void _zlox_isr_30();
extern void _zlox_isr_31();
extern void _zlox_isr_128();
extern void _zlox_irq_0	();
extern void _zlox_irq_1	();
extern void _zlox_irq_2	();
extern void _zlox_irq_3	();
extern void _zlox_irq_4	();
extern void _zlox_irq_5	();
extern void _zlox_irq_6	();
extern void _zlox_irq_7	();
extern void _zlox_irq_8	();
extern void _zlox_irq_9	();
extern void _zlox_irq_10();
extern void _zlox_irq_11();
extern void _zlox_irq_12();
extern void _zlox_irq_13();
extern void _zlox_irq_14();
extern void _zlox_irq_15();

#endif //_ZLOX_DESCRIPTOR_TABLES_H_

