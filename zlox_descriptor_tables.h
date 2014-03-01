/*zlox_descriptor_tables.h Defines the interface for initialising the GDT and IDT.
  Also defines needed structures.*/

#ifndef _ZLOX_DESCRIPTOR_TABLES_H_
#define _ZLOX_DESCRIPTOR_TABLES_H_

#include "zlox_common.h"

// Initialisation function is publicly accessible.
ZLOX_VOID zlox_init_descriptor_tables();

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

#endif //_ZLOX_DESCRIPTOR_TABLES_H_

