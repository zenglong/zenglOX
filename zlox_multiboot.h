/* zlox_multiboot.h -- Declares the multiboot info structure.*/

#ifndef _ZLOX_MULTIBOOT_H_
#define _ZLOX_MULTIBOOT_H_

struct _ZLOX_MULTIBOOT
{
	ZLOX_UINT32 flags;
	ZLOX_UINT32 mem_lower;
	ZLOX_UINT32 mem_upper;
	ZLOX_UINT32 boot_device;
	ZLOX_UINT32 cmdline;
	ZLOX_UINT32 mods_count;
	ZLOX_UINT32 mods_addr;
	ZLOX_UINT32 num;
	ZLOX_UINT32 size;
	ZLOX_UINT32 addr;
	ZLOX_UINT32 shndx;
	ZLOX_UINT32 mmap_length;
	ZLOX_UINT32 mmap_addr;
	ZLOX_UINT32 drives_length;
	ZLOX_UINT32 drives_addr;
	ZLOX_UINT32 config_table;
	ZLOX_UINT32 boot_loader_name;
	ZLOX_UINT32 apm_table;
	ZLOX_UINT32 vbe_control_info;
	ZLOX_UINT32 vbe_mode_info;
	ZLOX_UINT32 vbe_mode;
	ZLOX_UINT32 vbe_interface_seg;
	ZLOX_UINT32 vbe_interface_off;
	ZLOX_UINT32 vbe_interface_len;
}  __attribute__((packed));

typedef struct _ZLOX_MULTIBOOT ZLOX_MULTIBOOT;

#endif //_ZLOX_MULTIBOOT_H_

