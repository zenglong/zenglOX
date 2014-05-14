#cpuid.s Sample program to extract the processor Vendor ID
.section .data
output:
	.asciz "The processor Vendor ID is 'xxxxxxxxxxxx'\n"
.section .text
.globl _start
_start:
	movl $0, %eax
	cpuid
	movl $output, %edi
	movl %ebx, 28(%edi)
	movl %edx, 32(%edi)
	movl %ecx, 36(%edi)
	movl $0, %eax
	movl $output, %ebx
	int $0x80
	movl $9, %eax
	movl $0, %ebx
	int $0x80

