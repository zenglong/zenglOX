/*zlox_kernel.c Defines the C-code kernel entry point, calls initialisation routines.*/

#include "zlox_monitor.h"
#include "zlox_descriptor_tables.h"
#include "zlox_time.h"
#include "zlox_paging.h"
#include "zlox_kheap.h"
#include "zlox_multiboot.h"
#include "zlox_fs.h"
#include "zlox_initrd.h"
#include "zlox_task.h"
#include "zlox_syscall.h"
#include "zlox_keyboard.h"
#include "zlox_elf.h"

extern ZLOX_UINT32 placement_address;
ZLOX_UINT32 initial_esp;

//zenglOX kernel main entry
ZLOX_SINT32 zlox_kernel_main(ZLOX_MULTIBOOT * mboot_ptr, ZLOX_UINT32 initial_stack)
{
	ZLOX_UINT32 initrd_location;
	ZLOX_UINT32 initrd_end;

	initial_esp = initial_stack;

	// init gdt and idt
	zlox_init_descriptor_tables();	

	// Initialise the screen (by clearing it)
	zlox_monitor_clear();

	asm volatile("sti");
	// 将PIT定时器的频率设为50Hz,即每20ms产生一次时间中断
	zlox_init_timer(50);

	ZLOX_ASSERT(mboot_ptr->mods_count > 0);
	initrd_location = *((ZLOX_UINT32*)mboot_ptr->mods_addr);
	initrd_end = *((ZLOX_UINT32*)(mboot_ptr->mods_addr+4));
	// grub会将initrd模块放置到kernel内核后面,所以将placement_address设置为initrd_end,
	// 这样zlox_init_paging分配页表时才能将initrd考虑进来,同时堆分配空间时就不会覆盖到initrd里的内容
	placement_address = initrd_end;

	// 开启分页,并创建堆
	zlox_init_paging();

	// 初始化任务系统
	zlox_initialise_tasking();

	// Initialise the initial ramdisk, and set it as the filesystem root.
	fs_root = zlox_initialise_initrd(initrd_location);

	// 初始化系统调用
	zlox_initialise_syscalls();

	zlox_initKeyboard();

	zlox_syscall_monitor_write("=========================\n");	

	zlox_syscall_monitor_write("Keyboard is init now!\n");

	zlox_syscall_monitor_write("=========================\n");

	// 切换到ring 3的用户模式
	zlox_switch_to_user_mode();

	zlox_syscall_monitor_write("I'm in user mode!\n");

	zlox_syscall_monitor_write("welcome to zenglOX v0.0.11! I will execve a ELF file\n");

	zlox_syscall_execve("cpuid");

	for(;;)
		;

	return 0;
}

