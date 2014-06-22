# ld.s -- dynamic linker

.section .text
.globl _dl_runtime_resolve
_dl_runtime_resolve:
	call dl_fixup
	addl $0x8, %esp
	jmp *%eax

