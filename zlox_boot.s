#zlox_boot.s -- zenglOX kernel start assemble
.equ ZLOX_MBOOT_PAGE_ALIGN,1 #Load Kernel on a page boundary
.equ ZLOX_MBOOT_GETMEM_INFO,1<<1 #Tell MBoot provide your kernel with memory info
.equ ZLOX_MBOOT_MAGIC,0x1BADB002 #Multiboot Magic value

.equ ZLOX_MBOOT_FLAGS,ZLOX_MBOOT_PAGE_ALIGN | ZLOX_MBOOT_GETMEM_INFO
.equ ZLOX_MBOOT_CHECKSUM,-(ZLOX_MBOOT_MAGIC + ZLOX_MBOOT_FLAGS)

.section .zlox_multiboot
.align 4
.global _zlox_boot_mb_header
_zlox_boot_mb_header:
  .long ZLOX_MBOOT_MAGIC
  .long ZLOX_MBOOT_FLAGS
  .long ZLOX_MBOOT_CHECKSUM

  .long _zlox_boot_mb_header
  .long _code
  .long _bss
  .long _end
  .long _zlox_boot_start

.section .text
.global _zlox_boot_start
_zlox_boot_start:
  pushl %esp
  pushl %ebx
  cli
  call zlox_kernel_main
  hlt
zlox_boot_hang:
  jmp zlox_boot_hang

