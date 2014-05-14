# shutdown -- power off your computer
.section .text
.globl _start
_start:
	movl $23,%eax		# syscall_shutdown
	int $0x80

