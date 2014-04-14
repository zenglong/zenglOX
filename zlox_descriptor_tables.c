/*zlox_descriptor_tables.c Initialises the GDT and IDT, 
  and defines the default ISR and IRQ handler.*/

#include "zlox_common.h"
#include "zlox_isr.h"
#include "zlox_descriptor_tables.h"

// Lets us access our ASM functions from our C code.
extern ZLOX_VOID _zlox_gdt_flush(ZLOX_UINT32);
extern ZLOX_VOID _zlox_idt_flush(ZLOX_UINT32);
extern ZLOX_VOID _zlox_tss_flush();

static ZLOX_VOID zlox_init_gdt();
static ZLOX_VOID zlox_init_idt();
static ZLOX_VOID zlox_gdt_set_gate(ZLOX_SINT32 num, ZLOX_UINT32 base, ZLOX_UINT32 limit, 
									ZLOX_UINT8 access, ZLOX_UINT8 gran);
static ZLOX_VOID zlox_idt_set_gate(ZLOX_UINT8 num, ZLOX_UINT32 base, ZLOX_UINT16 sel,
									ZLOX_UINT8 flags);
static ZLOX_VOID zlox_write_tss(ZLOX_SINT32 num, ZLOX_UINT16 ss0, ZLOX_UINT32 esp0);

ZLOX_GDT_ENTRY gdt_entries[ZLOX_GDT_ENTRY_NUMBER];
ZLOX_GDT_PTR gdt_ptr;
ZLOX_IDT_ENTRY idt_entries[256];
ZLOX_IDT_PTR idt_ptr;
ZLOX_TSS_ENTRY tss_entry;

// Extern the ISR handler array so we can nullify them on startup.
extern ZLOX_ISR_CALLBACK interrupt_callbacks[];

// initialises the GDT and IDT.
ZLOX_VOID zlox_init_descriptor_tables()
{
	// Initialise the global descriptor table.
	zlox_init_gdt();

	// Initialise the interrupt descriptor table.
	zlox_init_idt();

	// Nullify all the interrupt callbacks.
	zlox_memset((ZLOX_UINT8 *)interrupt_callbacks, 0, sizeof(ZLOX_ISR_CALLBACK)*256);
}

// Initialises the GDT (global descriptor table)
static ZLOX_VOID zlox_init_gdt()
{
	gdt_ptr.limit = (sizeof(ZLOX_GDT_ENTRY) * ZLOX_GDT_ENTRY_NUMBER) - 1;
	gdt_ptr.base  = (ZLOX_UINT32)gdt_entries;
	
	zlox_gdt_set_gate(0, 0, 0, 0, 0);					// Null segment
	zlox_gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);	// Code segment
	zlox_gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);	// Data segment
	zlox_gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);	// User mode code segment
	zlox_gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);	// User mode data segment
	zlox_write_tss(5, 0x10, 0x0);

	_zlox_gdt_flush((ZLOX_UINT32)&gdt_ptr);
	_zlox_tss_flush();
}

// Set the value of one GDT entry.
static ZLOX_VOID zlox_gdt_set_gate(ZLOX_SINT32 num, ZLOX_UINT32 base, ZLOX_UINT32 limit, 
									ZLOX_UINT8 access, ZLOX_UINT8 gran)
{
	gdt_entries[num].base_low = (base & 0xFFFF);
	gdt_entries[num].base_middle = (base >> 16) & 0xFF;
	gdt_entries[num].base_high = (base >> 24) & 0xFF;

	gdt_entries[num].limit_low = (limit & 0xFFFF);
	gdt_entries[num].granularity = (limit >> 16) & 0x0F;
	
	gdt_entries[num].granularity |= gran & 0xF0;
	gdt_entries[num].access = access;
}

// Initialise our task state segment structure.
static ZLOX_VOID zlox_write_tss(ZLOX_SINT32 num, ZLOX_UINT16 ss0, ZLOX_UINT32 esp0)
{
    // Firstly, let's compute the base and limit of our entry into the GDT.
    ZLOX_UINT32 base = (ZLOX_UINT32) &tss_entry;
    ZLOX_UINT32 limit = sizeof(tss_entry) - 1;
    
    // Now, add our TSS descriptor's address to the GDT.
    zlox_gdt_set_gate(num, base, limit, 0xE9, 0x00);

    // Ensure the descriptor is initially zero.
    zlox_memset((ZLOX_UINT8 *)&tss_entry, 0, sizeof(tss_entry));

    tss_entry.ss0  = ss0;  // Set the kernel stack segment.
    tss_entry.esp0 = esp0; // Set the kernel stack pointer.
    
    // tss_entry.cs   = 0x0b;     
    // tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x13;
}

ZLOX_VOID zlox_set_kernel_stack(ZLOX_UINT32 stack)
{
    tss_entry.esp0 = stack;
}

// Initialise the interrupt descriptor table.
static ZLOX_VOID zlox_init_idt()
{
	idt_ptr.limit = sizeof(ZLOX_IDT_ENTRY) * 256 - 1;
	idt_ptr.base  = (ZLOX_UINT32)idt_entries;

	zlox_memset((ZLOX_UINT8 *)idt_entries, 0, sizeof(ZLOX_IDT_ENTRY)*256);

	// Remap the irq table.
	// ICW1: starts the initialization sequence (in cascade mode)
	zlox_outb(0x20, 0x11); 
	zlox_outb(0xA0, 0x11);
	// ICW2: Master PIC vector offset
	zlox_outb(0x21, 0x20); 
	// ICW2: Slave PIC vector offset
	zlox_outb(0xA1, 0x28);
	// ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	zlox_outb(0x21, 0x04); 
	// ICW3: tell Slave PIC its cascade identity (0000 0010)
	zlox_outb(0xA1, 0x02);
	// ICW4: 8086/88 (MCS-80/85) mode
	zlox_outb(0x21, 0x01);
	// ICW4: 8086/88 (MCS-80/85) mode
	zlox_outb(0xA1, 0x01);
	// masks.
	zlox_outb(0x21, 0x0);
	// masks.
	zlox_outb(0xA1, 0x0);
	
	zlox_idt_set_gate( 0, (ZLOX_UINT32)_zlox_isr_0 , 0x08, 0x8E);
	zlox_idt_set_gate( 1, (ZLOX_UINT32)_zlox_isr_1 , 0x08, 0x8E);
	zlox_idt_set_gate( 2, (ZLOX_UINT32)_zlox_isr_2 , 0x08, 0x8E);
	zlox_idt_set_gate( 3, (ZLOX_UINT32)_zlox_isr_3 , 0x08, 0x8E);
	zlox_idt_set_gate( 4, (ZLOX_UINT32)_zlox_isr_4 , 0x08, 0x8E);
	zlox_idt_set_gate( 5, (ZLOX_UINT32)_zlox_isr_5 , 0x08, 0x8E);
	zlox_idt_set_gate( 6, (ZLOX_UINT32)_zlox_isr_6 , 0x08, 0x8E);
	zlox_idt_set_gate( 7, (ZLOX_UINT32)_zlox_isr_7 , 0x08, 0x8E);
	zlox_idt_set_gate( 8, (ZLOX_UINT32)_zlox_isr_8 , 0x08, 0x8E);
	zlox_idt_set_gate( 9, (ZLOX_UINT32)_zlox_isr_9 , 0x08, 0x8E);
	zlox_idt_set_gate(10, (ZLOX_UINT32)_zlox_isr_10, 0x08, 0x8E);
	zlox_idt_set_gate(11, (ZLOX_UINT32)_zlox_isr_11, 0x08, 0x8E);
	zlox_idt_set_gate(12, (ZLOX_UINT32)_zlox_isr_12, 0x08, 0x8E);
	zlox_idt_set_gate(13, (ZLOX_UINT32)_zlox_isr_13, 0x08, 0x8E);
	zlox_idt_set_gate(14, (ZLOX_UINT32)_zlox_isr_14, 0x08, 0x8E);
	zlox_idt_set_gate(15, (ZLOX_UINT32)_zlox_isr_15, 0x08, 0x8E);
	zlox_idt_set_gate(16, (ZLOX_UINT32)_zlox_isr_16, 0x08, 0x8E);
	zlox_idt_set_gate(17, (ZLOX_UINT32)_zlox_isr_17, 0x08, 0x8E);
	zlox_idt_set_gate(18, (ZLOX_UINT32)_zlox_isr_18, 0x08, 0x8E);
	zlox_idt_set_gate(19, (ZLOX_UINT32)_zlox_isr_19, 0x08, 0x8E);
	zlox_idt_set_gate(20, (ZLOX_UINT32)_zlox_isr_20, 0x08, 0x8E);
	zlox_idt_set_gate(21, (ZLOX_UINT32)_zlox_isr_21, 0x08, 0x8E);
	zlox_idt_set_gate(22, (ZLOX_UINT32)_zlox_isr_22, 0x08, 0x8E);
	zlox_idt_set_gate(23, (ZLOX_UINT32)_zlox_isr_23, 0x08, 0x8E);
	zlox_idt_set_gate(24, (ZLOX_UINT32)_zlox_isr_24, 0x08, 0x8E);
	zlox_idt_set_gate(25, (ZLOX_UINT32)_zlox_isr_25, 0x08, 0x8E);
	zlox_idt_set_gate(26, (ZLOX_UINT32)_zlox_isr_26, 0x08, 0x8E);
	zlox_idt_set_gate(27, (ZLOX_UINT32)_zlox_isr_27, 0x08, 0x8E);
	zlox_idt_set_gate(28, (ZLOX_UINT32)_zlox_isr_28, 0x08, 0x8E);
	zlox_idt_set_gate(29, (ZLOX_UINT32)_zlox_isr_29, 0x08, 0x8E);
	zlox_idt_set_gate(30, (ZLOX_UINT32)_zlox_isr_30, 0x08, 0x8E);
	zlox_idt_set_gate(31, (ZLOX_UINT32)_zlox_isr_31, 0x08, 0x8E);
	zlox_idt_set_gate(31, (ZLOX_UINT32)_zlox_isr_31, 0x08, 0x8E);
	zlox_idt_set_gate(128, (ZLOX_UINT32)_zlox_isr_128, 0x08, 0x8E); // syscall
	zlox_idt_set_gate(ZLOX_IRQ0, (ZLOX_UINT32)_zlox_irq_0, 0x08, 0x8E);
	zlox_idt_set_gate(ZLOX_IRQ1, (ZLOX_UINT32)_zlox_irq_1, 0x08, 0x8E);
	zlox_idt_set_gate(ZLOX_IRQ2, (ZLOX_UINT32)_zlox_irq_2, 0x08, 0x8E);
	zlox_idt_set_gate(ZLOX_IRQ3, (ZLOX_UINT32)_zlox_irq_3, 0x08, 0x8E);
	zlox_idt_set_gate(ZLOX_IRQ4, (ZLOX_UINT32)_zlox_irq_4, 0x08, 0x8E);
	zlox_idt_set_gate(ZLOX_IRQ5, (ZLOX_UINT32)_zlox_irq_5, 0x08, 0x8E);
	zlox_idt_set_gate(ZLOX_IRQ6, (ZLOX_UINT32)_zlox_irq_6, 0x08, 0x8E);
	zlox_idt_set_gate(ZLOX_IRQ7, (ZLOX_UINT32)_zlox_irq_7, 0x08, 0x8E);
	zlox_idt_set_gate(ZLOX_IRQ8, (ZLOX_UINT32)_zlox_irq_8, 0x08, 0x8E);
	zlox_idt_set_gate(ZLOX_IRQ9, (ZLOX_UINT32)_zlox_irq_9, 0x08, 0x8E);
	zlox_idt_set_gate(ZLOX_IRQ10, (ZLOX_UINT32)_zlox_irq_10, 0x08, 0x8E);
	zlox_idt_set_gate(ZLOX_IRQ11, (ZLOX_UINT32)_zlox_irq_11, 0x08, 0x8E);
	zlox_idt_set_gate(ZLOX_IRQ12, (ZLOX_UINT32)_zlox_irq_12, 0x08, 0x8E);
	zlox_idt_set_gate(ZLOX_IRQ13, (ZLOX_UINT32)_zlox_irq_13, 0x08, 0x8E);
	zlox_idt_set_gate(ZLOX_IRQ14, (ZLOX_UINT32)_zlox_irq_14, 0x08, 0x8E);
	zlox_idt_set_gate(ZLOX_IRQ15, (ZLOX_UINT32)_zlox_irq_15, 0x08, 0x8E);

	_zlox_idt_flush((ZLOX_UINT32)&idt_ptr);
}

static ZLOX_VOID zlox_idt_set_gate(ZLOX_UINT8 num, ZLOX_UINT32 base, ZLOX_UINT16 sel,
									ZLOX_UINT8 flags)
{
	idt_entries[num].base_lo = base & 0xFFFF;
	idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

	idt_entries[num].sel     = sel;
	idt_entries[num].always0 = 0;
	idt_entries[num].flags   = flags  | 0x60;
}

