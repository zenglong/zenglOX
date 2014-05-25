# zlox_process.s some functions relate to process
.global _zlox_read_eip
_zlox_read_eip:
	pop %eax	# Get the return address
	jmp *%eax	# Return. Can't use RET because return address popped off the stack. 

.global _zlox_switch_task_do
_zlox_switch_task_do:
	cli
	movl 4(%esp),%ecx	# eip
	movl 8(%esp),%edx	# esp tmp
	movl 12(%esp),%ebp	# ebp
	movl 16(%esp),%eax	# cr3 tmp
	movl %eax,%cr3		# cr3
	movl $0x12345, %eax	# eax
	movl %edx,%esp		# esp
	jmp *%ecx

.global _zlox_copy_page_physical
_zlox_copy_page_physical:
	pushf		# push EFLAGS, so we can pop it and reenable interrupts and restore DF flag
	cli		# Disable interrupts, so we aren't interrupted.
	movl 8(%esp),%esi	# Source address
	movl 12(%esp),%edi	# Destination address
	
	movl %cr0,%eax		# Get the control register...
	andl $0x7fffffff,%eax	# and...
	movl %eax,%cr0		# Disable paging.

	movl $1024,%ecx		# 1024*4bytes = 4096 bytes
	cld		# copy from low address to high address , so clear DF flag
	rep movsl	# do copy!
	
	movl %cr0,%eax		# Get the control register...
	orl $0x80000000,%eax	# or...
	movl %eax,%cr0		# Enable paging.

	popf		# Pop EFLAGS back.
	ret

.global _zlox_idle_cpu
 _zlox_idle_cpu:
	pushf		# push EFLAGS
	sti
#null_loop:
	hlt
	#jmp null_loop
	popf		# Pop EFLAGS back.
	ret

