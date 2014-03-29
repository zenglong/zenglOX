/*zlox_kernel.c Defines the C-code kernel entry point, calls initialisation routines.*/

#include "zlox_monitor.h"
#include "zlox_descriptor_tables.h"
#include "zlox_time.h"
#include "zlox_paging.h"
#include "zlox_kheap.h"
#include "zlox_multiboot.h"
#include "zlox_fs.h"
#include "zlox_initrd.h"

extern ZLOX_UINT32 placement_address;

//zenglOX kernel main entry
ZLOX_SINT32 zlox_kernel_main(ZLOX_MULTIBOOT * mboot_ptr)
{
	ZLOX_UINT32 initrd_location;
	ZLOX_UINT32 initrd_end;

	// init gdt and idt
	zlox_init_descriptor_tables();	

	// Initialise the screen (by clearing it)
	zlox_monitor_clear();

	ZLOX_ASSERT(mboot_ptr->mods_count > 0);
	initrd_location = *((ZLOX_UINT32*)mboot_ptr->mods_addr);
	initrd_end = *((ZLOX_UINT32*)(mboot_ptr->mods_addr+4));
	// grub会将initrd模块放置到kernel内核后面,所以将placement_address设置为initrd_end,
	// 这样zlox_init_paging分配页表时才能将initrd考虑进来,同时堆分配空间时就不会覆盖到initrd里的内容
	placement_address = initrd_end;

	// 开启分页,并创建堆
	zlox_init_paging();

	// Initialise the initial ramdisk, and set it as the filesystem root.
	fs_root = zlox_initialise_initrd(initrd_location);

	// list the contents of /
	ZLOX_SINT32 i = 0;
	ZLOX_DIRENT *node = 0;
	while ( (node = zlox_readdir_fs(fs_root, i)) != 0)
	{
		zlox_monitor_write("Found file ");
		zlox_monitor_write(node->name);
		ZLOX_FS_NODE *fsnode = zlox_finddir_fs(fs_root, node->name);

		if ((fsnode->flags&0x7) == ZLOX_FS_DIRECTORY)
		{
			zlox_monitor_write("\n\t(directory)\n");
		}
		else
		{
			zlox_monitor_write("\n\t contents: \"");
			ZLOX_CHAR buf[256];
			ZLOX_UINT32 sz = zlox_read_fs(fsnode, 0, 256, (ZLOX_UINT8 *)buf);
			ZLOX_UINT32 j;
			for (j = 0; j < sz; j++)
				zlox_monitor_put(buf[j]);
			
			zlox_monitor_write("\"\n");
		}
		i++;
	}

	// Write out a sample string
	zlox_monitor_write("hello world!\nwelcome to zenglOX v0.0.7!\n");

	/*asm volatile("int $0x3");
	asm volatile("int $0x4");
	
	asm volatile("sti");
	zlox_init_timer(50);*/

	return (ZLOX_SINT32)mboot_ptr;
}

