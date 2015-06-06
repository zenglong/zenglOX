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
#include "zlox_ata.h"
#include "zlox_iso.h"
#include "zlox_pci.h"
#include "zlox_network.h"
#include "zlox_ps2.h"
#include "zlox_mouse.h"
#include "zlox_vga.h"
#include "zlox_my_windows.h"
#include "zlox_audio.h"
#include "zlox_usb.h"

ZLOX_VOID zlox_test_ps2_keyboard();

extern ZLOX_UINT32 placement_address;
extern ZLOX_TASK * current_task;
extern ZLOX_BOOL ide_can_dma;
//zlox_vga.c
extern ZLOX_UINT8 * lfb_vid_memory;
//extern ZLOX_BOOL atapi_vmware_warning;
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
	// 将PIT定时器的频率设为1000Hz,即每1ms产生一次时间中断
	//zlox_init_timer(1000);

	ZLOX_ASSERT(mboot_ptr->mods_count > 0);
	initrd_location = *((ZLOX_UINT32*)mboot_ptr->mods_addr);
	initrd_end = *((ZLOX_UINT32*)(mboot_ptr->mods_addr+4));
	// grub会将initrd模块放置到kernel内核后面,所以将placement_address设置为initrd_end,
	// 这样zlox_init_paging分配页表时才能将initrd考虑进来,同时堆分配空间时就不会覆盖到initrd里的内容
	placement_address = initrd_end;

	zlox_klog_init();

	lfb_vid_memory = (ZLOX_UINT8 *)((ZLOX_MULTIBOOT_VBE_INFO *)(mboot_ptr->vbe_mode_info))->physbase;

	ZLOX_SINT32 audio_reset = zlox_audio_reset();
	if(audio_reset == 0)
		zlox_audio_alloc_res_before_init();

	// 开启分页,并创建堆
	zlox_init_paging_start((ZLOX_UINT32)(mboot_ptr->mem_lower + mboot_ptr->mem_upper));

	zlox_pci_init();

	zlox_pci_list();

	ZLOX_BOOL network_stat = zlox_network_init();

	if(zlox_vga_set_mode(ZLOX_VGA_MODE_VBE_1024X768X32) == -1)
	{
		while(1) // set vbe failed! so we died
			;
	}
	else
		zlox_monitor_write("vga [vbe] mode is init now!\n");

	zlox_init_my_mouse();

	if(network_stat == ZLOX_TRUE)
		zlox_monitor_write("network is init now!\n");

	zlox_init_paging_end();

	if(audio_reset == 0)
		zlox_audio_init();

	zlox_usb_init();

	/*for (ZLOX_UINT16 y = 0; y < 768; y++) {
		for (ZLOX_UINT16 x = 0; x < 1024; x++) {
			ZLOX_UINT8 f = y % 255;
			((ZLOX_UINT32 *)lfb_vid_memory)[x + y * 1024] = 0xFF000000 | (f * 0x10000) | (0 * 0x100) | 0;
		}
	}*/

	// 初始化任务系统
	zlox_initialise_tasking();

	// Initialise the initial ramdisk, and set it as the filesystem root.
	fs_root = zlox_initialise_initrd(initrd_location);

	// 初始化系统调用
	zlox_initialise_syscalls();

	if(zlox_ps2_init() == ZLOX_TRUE)
	{
		zlox_syscall_monitor_write("PS2 Controller is init now!\n");
		if(zlox_initKeyboard() == ZLOX_TRUE)
		{
			zlox_syscall_monitor_write("=========================\n");	
			zlox_syscall_monitor_write("Keyboard is init now!\n");
			zlox_syscall_monitor_write("=========================\n");
		}
		zlox_mouse_install();
		zlox_syscall_monitor_write("mouse is install now!\n");
	}

	zlox_init_ata();

	zlox_syscall_monitor_write("ata is init now!\n");

	// 切换到ring 3的用户模式
	zlox_switch_to_user_mode();

	zlox_syscall_monitor_write("I'm in user mode!\n");

	zlox_syscall_monitor_write("welcome to zenglOX v");
	ZLOX_SINT32 major,minor,revision;
	zlox_syscall_get_version(&major,&minor,&revision);
	zlox_syscall_monitor_write_dec(major);
	zlox_syscall_monitor_put('.');
	zlox_syscall_monitor_write_dec(minor);
	zlox_syscall_monitor_put('.');
	zlox_syscall_monitor_write_dec(revision);
	zlox_syscall_monitor_write("! I will execve a desktop\n"
				"you can input some command: ls , ps , cat , uname , cpuid , shell ,"
				" reboot , shutdown , ata , mount , unmount , testoverflow , fdisk , "
				"format , file , vga , dhcp isoget...\n\n");

	if(ide_can_dma)
		zlox_syscall_monitor_write("Congratulation! you can use DMA mode with ATA Drive! ");
	else
		zlox_syscall_monitor_write("you can only use PIO mode... ");

	zlox_syscall_monitor_write("\n\n");

	asm ("finit");

	ZLOX_SINT32 sec=0;
	ZLOX_UINT32 origtick = zlox_syscall_timer_get_tick();
	ZLOX_UINT32 frequency = zlox_syscall_timer_get_frequency();
	while(sec < 3)
	{
		while(ZLOX_TRUE)
		{
			zlox_syscall_idle_cpu();
			ZLOX_UINT32 curtick = zlox_syscall_timer_get_tick();
			if((curtick - origtick) > frequency)
				break;
		}
		zlox_syscall_monitor_write("..");
		sec++;
		origtick = zlox_syscall_timer_get_tick();
	}

	//while(1)
	//	;

	zlox_syscall_monitor_disable_scroll();

	zlox_syscall_execve("desktop");

	zlox_syscall_wait(current_task);

	ZLOX_SINT32 ret;
	ZLOX_TASK_MSG msg;
	while(ZLOX_TRUE)
	{
		ret = zlox_syscall_get_tskmsg(current_task,&msg,ZLOX_TRUE);
		if(ret != 1)
		{
			zlox_syscall_wait(current_task);
			// 只剩下一个初始任务了，就再创建一个desktop
			if(current_task->next == 0)
			{
				zlox_syscall_execve("desktop");
				zlox_syscall_wait(current_task);
			}
		}
		else if(msg.type == ZLOX_MT_TASK_FINISH)
		{
			zlox_syscall_finish(msg.finish_task.exit_task);
		}
	}

	return 0;
}

