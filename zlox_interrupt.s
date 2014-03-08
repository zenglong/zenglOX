# zlox_interrupt.s Contains interrupt service routine wrappers.

# This macro creates a stub for an ISR which does NOT pass it's own 
# error code (adds a dummy errcode byte).
.macro _ZLOX_INT_ISR_NOERRCODE argNum
.global _zlox_isr_\argNum
_zlox_isr_\argNum:
	cli				# Disable interrupts firstly.
	pushl $0		# Push a dummy error code.
	pushl $\argNum	# Push the interrupt number.
	jmp _zlox_isr_common_stub
.endm

# This macro creates a stub for an ISR which passes it's own error code.
.macro _ZLOX_INT_ISR_ERRCODE argNum
.global _zlox_isr_\argNum
_zlox_isr_\argNum:
	cli				# Disable interrupts firstly.
	pushl $\argNum	# Push the interrupt number.
	jmp _zlox_isr_common_stub
.endm

.macro _ZLOX_INT_IRQ irqNum intNum
.global _zlox_irq_\irqNum
_zlox_irq_\irqNum:
	cli
	pushl $0
	pushl $\intNum
	jmp _zlox_irq_common_stub
.endm

_ZLOX_INT_ISR_NOERRCODE	0
_ZLOX_INT_ISR_NOERRCODE	1
_ZLOX_INT_ISR_NOERRCODE	2
_ZLOX_INT_ISR_NOERRCODE	3
_ZLOX_INT_ISR_NOERRCODE	4
_ZLOX_INT_ISR_NOERRCODE	5
_ZLOX_INT_ISR_NOERRCODE	6
_ZLOX_INT_ISR_NOERRCODE	7
_ZLOX_INT_ISR_ERRCODE	8
_ZLOX_INT_ISR_NOERRCODE	9
_ZLOX_INT_ISR_ERRCODE	10
_ZLOX_INT_ISR_ERRCODE	11
_ZLOX_INT_ISR_ERRCODE	12
_ZLOX_INT_ISR_ERRCODE	13
_ZLOX_INT_ISR_ERRCODE	14
_ZLOX_INT_ISR_NOERRCODE	15
_ZLOX_INT_ISR_NOERRCODE	16
_ZLOX_INT_ISR_NOERRCODE	17
_ZLOX_INT_ISR_NOERRCODE	18
_ZLOX_INT_ISR_NOERRCODE	19
_ZLOX_INT_ISR_NOERRCODE	20
_ZLOX_INT_ISR_NOERRCODE	21
_ZLOX_INT_ISR_NOERRCODE	22
_ZLOX_INT_ISR_NOERRCODE	23
_ZLOX_INT_ISR_NOERRCODE	24
_ZLOX_INT_ISR_NOERRCODE	25
_ZLOX_INT_ISR_NOERRCODE	26
_ZLOX_INT_ISR_NOERRCODE	27
_ZLOX_INT_ISR_NOERRCODE	28
_ZLOX_INT_ISR_NOERRCODE	29
_ZLOX_INT_ISR_NOERRCODE	30
_ZLOX_INT_ISR_NOERRCODE	31
_ZLOX_INT_IRQ	0,	32
_ZLOX_INT_IRQ	1,	33
_ZLOX_INT_IRQ	2,	34
_ZLOX_INT_IRQ	3,	35
_ZLOX_INT_IRQ	4,	36
_ZLOX_INT_IRQ	5,	37
_ZLOX_INT_IRQ	6,	38
_ZLOX_INT_IRQ	7,	39
_ZLOX_INT_IRQ	8,	40
_ZLOX_INT_IRQ	9,	41
_ZLOX_INT_IRQ	10,	42
_ZLOX_INT_IRQ	11,	43
_ZLOX_INT_IRQ	12,	44
_ZLOX_INT_IRQ	13,	45
_ZLOX_INT_IRQ	14,	46
_ZLOX_INT_IRQ	15,	47

_zlox_isr_common_stub:
	pusha			# Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

	movw %ds,%ax	# Lower 16-bits of eax = ds.
	pushl %eax		# save the data segment descriptor

	movw $0x10,%ax	# load the kernel data segment descriptor
	movw %ax,%ds
	movw %ax,%es
	movw %ax,%fs
	movw %ax,%gs

	call zlox_isr_handler	# in zlox_isr.c

	pop %ebx		# reload the original data segment descriptor
	movw %bx,%ds
	movw %bx,%es
	movw %bx,%fs
	movw %bx,%gs

	popa			# Pops edi,esi,ebp...
	addl $8,%esp	# Cleans up the pushed error code and pushed ISR number
	sti
	iret			# pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP

# This is our common IRQ stub. It saves the processor state, sets
# up for kernel mode segments, calls the C-level fault handler,
# and finally restores the stack frame.
_zlox_irq_common_stub:
	pusha			# Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

	movw %ds,%ax	# Lower 16-bits of eax = ds.
	pushl %eax		# save the data segment descriptor

	movw $0x10,%ax	# load the kernel data segment descriptor
	movw %ax,%ds
	movw %ax,%es
	movw %ax,%fs
	movw %ax,%gs

	call zlox_irq_handler	# in zlox_isr.c

	pop %ebx		# reload the original data segment descriptor
	movw %bx,%ds
	movw %bx,%es
	movw %bx,%fs
	movw %bx,%gs

	popa			# Pops edi,esi,ebp...
	addl $8,%esp	# Cleans up the pushed error code and pushed ISR number
	sti
	iret			# pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP

