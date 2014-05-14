# reboot -- reboot your computer
.section .text
.globl _start
_start:
	movl $22,%eax		# syscall_reboot
	int $0x80

