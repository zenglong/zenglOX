# zlox_shutdown.s -- some functions relate to reboot and shutdown.
.section .data
shutdown_str:
	.asciz "Shutdown"

.section .text
.global _zlox_reboot
_zlox_reboot:
	# Wait for a empty Input Buffer
wait1:
	inb  $0x64,%al
	testb $0x2,%al
	jne  wait1
	
	# Send 0xFE to the keyboard controller.
	movb  $0xFE,%al
	outb  %al,$0x64

.global _zlox_shutdown
_zlox_shutdown:
	cli
	movw $0x2000,%ax	# ACPI shutdown (http://forum.osdev.org/viewtopic.php?t=16990) and used in chaOS
	movw $0xB004,%dx
	outw %ax,%dx

	movw $0x8900,%dx
	movl $0, %edi
loop:
	movb shutdown_str(, %edi, 1), %al
	outb %al,%dx
	inc %edi
	cmpl $8, %edi
	jne loop

	cli
	hlt

