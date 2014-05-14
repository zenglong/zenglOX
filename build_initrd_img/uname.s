#uname.s -- display zenglOX version info
.section .data
output:
	.asciz "zenglOX v"
major:
	.int 0
minor:
	.int 0
revision:
	.int 0
.section .text
.globl _start
_start:
	movl $21, %eax	# syscall_get_version
	movl $major,%ebx
	movl $minor,%ecx
	movl $revision,%edx
	int $0x80

	movl $0, %eax		# syscall_monitor_write
	movl $output, %ebx
	int $0x80

	movl $2, %eax		# syscall_monitor_write_dec
	movl major,%ebx
	int $0x80

	movl $3, %eax		# syscall_monitor_put
	movl $46,%ebx		# '.'
	int $0x80

	movl $2, %eax		# syscall_monitor_write_dec
	movl minor,%ebx
	int $0x80

	movl $3, %eax		# syscall_monitor_put
	movl $46,%ebx		# '.'
	int $0x80

	movl $2, %eax		# syscall_monitor_write_dec
	movl revision,%ebx
	int $0x80

	movl $9, %eax		# syscall_exit
	movl $0, %ebx
	int $0x80

