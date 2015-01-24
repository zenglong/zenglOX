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

	movl $64, %eax	# syscall_cmd_window_write
	movl $output, %ebx
	int $0x80

	movl $67, %eax	# syscall_cmd_window_write_dec
	movl major,%ebx
	int $0x80

	movl $63, %eax	# syscall_cmd_window_put
	movl $46,%ebx		# '.'
	int $0x80

	movl $67, %eax	# syscall_cmd_window_write_dec
	movl minor,%ebx
	int $0x80

	movl $63, %eax	# syscall_cmd_window_put
	movl $46,%ebx		# '.'
	int $0x80

	movl $67, %eax	# syscall_cmd_window_write_dec
	movl revision,%ebx
	int $0x80

	movl $9, %eax		# syscall_exit
	movl $0, %ebx
	int $0x80

