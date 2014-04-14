# zlox_gdt.s contains global descriptor table and interrupt descriptor table setup code.

.global _zlox_gdt_flush	# Allows the C code to call _zlox_gdt_flush().
_zlox_gdt_flush:
	movl 4(%esp),%eax	# Get the pointer to the GDT, passed as a parameter.
	lgdt (%eax)			# Load the new GDT pointer
	
	movw $0x10,%ax		# 0x10 is the offset in the GDT to our data segment
	movw %ax,%ds		# Load all data segment selectors
	movw %ax,%es
	movw %ax,%fs
	movw %ax,%gs
	movw %ax,%ss
	ljmp $0x08,$_gdt_ljmp_flush	# 0x08 is the offset to our code segment: Far jump!
_gdt_ljmp_flush:
	ret

.global _zlox_idt_flush	# Allows the C code to call _zlox_idt_flush().
_zlox_idt_flush:
	movl 4(%esp),%eax	# Get the pointer to the IDT, passed as a parameter.
	lidt (%eax)			# Load the IDT pointer.
	ret

.global _zlox_tss_flush
_zlox_tss_flush:
	movl $0x2B,%eax		# The index is 5th in GDT and RPL is 3
	ltr %ax			# Load 0x2B into the task state register.
	ret

