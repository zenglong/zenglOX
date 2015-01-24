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

struct _ZLOX_MULTIBOOT_VBE_INFO {
	ZLOX_UINT16 attributes;
	ZLOX_UINT8  winA, winB;
	ZLOX_UINT16 granularity;
	ZLOX_UINT16 winsize;
	ZLOX_UINT16 segmentA, segmentB;
	ZLOX_UINT32 realFctPtr;
	ZLOX_UINT16 pitch;

	ZLOX_UINT16 Xres, Yres;
	ZLOX_UINT8  Wchar, Ychar, planes, bpp, banks;
	ZLOX_UINT8  memory_model, bank_size, image_pages;
	ZLOX_UINT8  reserved0;

	ZLOX_UINT8  red_mask, red_position;
	ZLOX_UINT8  green_mask, green_position;
	ZLOX_UINT8  blue_mask, blue_position;
	ZLOX_UINT8  rsv_mask, rsv_position;
	ZLOX_UINT8  directcolor_attributes;

	ZLOX_UINT32 physbase;
	ZLOX_UINT32 reserved1;
	ZLOX_UINT16 reserved2;
} __attribute__ ((packed));

typedef struct _ZLOX_MULTIBOOT ZLOX_MULTIBOOT;

typedef struct _ZLOX_MULTIBOOT_VBE_INFO ZLOX_MULTIBOOT_VBE_INFO;

#endif //_ZLOX_MULTIBOOT_H_

