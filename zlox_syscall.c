// zlox_syscall.c Defines the implementation of a system call system.

#include "zlox_syscall.h"
#include "zlox_isr.h"
#include "zlox_monitor.h"
#include "zlox_keyboard.h"
#include "zlox_elf.h"
#include "zlox_task.h"
#include "zlox_uheap.h"
#include "zlox_fs.h"
#include "zlox_paging.h"
#include "zlox_ata.h"
#include "zlox_iso.h"
#include "zlox_zenglfs.h"
#include "zlox_vga.h"
#include "zlox_network.h"
#include "zlox_time.h"
#include "zlox_pci.h"
#include "zlox_ps2.h"

static ZLOX_VOID zlox_syscall_handler(ZLOX_ISR_REGISTERS * regs);
// _zlox_reboot() and _zlox_shutdown() is in zlox_shutdown.s
ZLOX_VOID _zlox_reboot();
ZLOX_VOID _zlox_shutdown();
//  _zlox_idle_cpu is in zlox_process.s
ZLOX_VOID _zlox_idle_cpu();

// defined in zlox_syscall.c
ZLOX_SINT32 zlox_overflow_test();

ZLOX_DEFN_SYSCALL1(monitor_write, ZLOX_SYSCALL_MONITOR_WRITE, const char*);
ZLOX_DEFN_SYSCALL1(monitor_write_hex, ZLOX_SYSCALL_MONITOR_WRITE_HEX, ZLOX_UINT32);
ZLOX_DEFN_SYSCALL1(monitor_write_dec, ZLOX_SYSCALL_MONITOR_WRITE_DEC, ZLOX_UINT32);
ZLOX_DEFN_SYSCALL1(monitor_put, ZLOX_SYSCALL_MONITOR_PUT, char);
ZLOX_DEFN_SYSCALL1(execve, ZLOX_SYSCALL_EXECVE, const char*);
ZLOX_DEFN_SYSCALL3(get_tskmsg,ZLOX_SYSCALL_GET_TSKMSG,void *,void *,ZLOX_BOOL);
ZLOX_DEFN_SYSCALL1(wait, ZLOX_SYSCALL_WAIT, void *);
ZLOX_DEFN_SYSCALL1(set_input_focus, ZLOX_SYSCALL_SET_INPUT_FOCUS, void *);
ZLOX_DEFN_SYSCALL0(get_currentTask,ZLOX_SYSCALL_GET_CURRENT_TASK);
ZLOX_DEFN_SYSCALL1(exit, ZLOX_SYSCALL_EXIT, int);
ZLOX_DEFN_SYSCALL1(finish, ZLOX_SYSCALL_FINISH, void *);
ZLOX_DEFN_SYSCALL1(get_args, ZLOX_SYSCALL_GET_ARGS, void *);
ZLOX_DEFN_SYSCALL1(get_init_esp, ZLOX_SYSCALL_GET_INIT_ESP, void *);
ZLOX_DEFN_SYSCALL1(umalloc, ZLOX_SYSCALL_UMALLOC, int);
ZLOX_DEFN_SYSCALL1(ufree, ZLOX_SYSCALL_UFREE, void *);
ZLOX_DEFN_SYSCALL4(read_fs,ZLOX_SYSCALL_READ_FS,void *,int,int,void *);
ZLOX_DEFN_SYSCALL2(readdir_fs,ZLOX_SYSCALL_READDIR_FS,void *,int);
ZLOX_DEFN_SYSCALL2(finddir_fs,ZLOX_SYSCALL_FINDDIR_FS,void *,void *);
ZLOX_DEFN_SYSCALL0(get_fs_root,ZLOX_SYSCALL_GET_FS_ROOT);
ZLOX_DEFN_SYSCALL2(get_frame_info,ZLOX_SYSCALL_GET_FRAME_INFO,void **,void *);
ZLOX_DEFN_SYSCALL0(get_kheap,ZLOX_SYSCALL_GET_KHEAP);
ZLOX_DEFN_SYSCALL3(get_version,ZLOX_SYSCALL_GET_VERSION,void *,void *,void *);
ZLOX_DEFN_SYSCALL0(reboot,ZLOX_SYSCALL_REBOOT);
ZLOX_DEFN_SYSCALL0(shutdown,ZLOX_SYSCALL_SHUTDOWN);
ZLOX_DEFN_SYSCALL0(idle_cpu,ZLOX_SYSCALL_IDLE_CPU);
ZLOX_DEFN_SYSCALL3(atapi_drive_read_sector,ZLOX_SYSCALL_ATAPI_DRIVE_READ_SECTOR,ZLOX_UINT32,ZLOX_UINT32,void *);
ZLOX_DEFN_SYSCALL2(atapi_drive_read_capacity,ZLOX_SYSCALL_ATAPI_DRIVE_READ_CAPACITY,ZLOX_UINT32,void *);
ZLOX_DEFN_SYSCALL0(ata_get_ide_info,ZLOX_SYSCALL_ATA_GET_IDE_INFO);
ZLOX_DEFN_SYSCALL0(mount_iso,ZLOX_SYSCALL_MOUNT_ISO);
ZLOX_DEFN_SYSCALL0(unmount_iso,ZLOX_SYSCALL_UNMOUNT_ISO);
ZLOX_DEFN_SYSCALL0(overflow_test,ZLOX_SYSCALL_OVERFLOW_TEST);
ZLOX_DEFN_SYSCALL5(ide_ata_access, ZLOX_SYSCALL_IDE_ATA_ACCESS, ZLOX_UINT8, ZLOX_UINT8, ZLOX_UINT32, ZLOX_UINT8, ZLOX_UINT8 *);
ZLOX_DEFN_SYSCALL2(mount_zenglfs, ZLOX_SYSCALL_MOUNT_ZENGLFS, ZLOX_UINT32, ZLOX_UINT32);
ZLOX_DEFN_SYSCALL0(unmount_zenglfs, ZLOX_SYSCALL_UNMOUNT_ZENGLFS);
ZLOX_DEFN_SYSCALL4(write_fs, ZLOX_SYSCALL_WRITE_FS, void *, ZLOX_UINT32 , ZLOX_UINT32 , ZLOX_UINT8 *);
ZLOX_DEFN_SYSCALL3(writedir_fs, ZLOX_SYSCALL_WRITEDIR_FS, void *, ZLOX_CHAR *, ZLOX_UINT16);
ZLOX_DEFN_SYSCALL1(remove_fs, ZLOX_SYSCALL_REMOVE_FS, void *);
ZLOX_DEFN_SYSCALL2(rename_fs, ZLOX_SYSCALL_RENAME_FS, void *, ZLOX_CHAR *);
ZLOX_DEFN_SYSCALL1(vga_set_mode, ZLOX_SYSCALL_VGA_SET_MODE, ZLOX_UINT32);
ZLOX_DEFN_SYSCALL2(vga_update_screen, ZLOX_SYSCALL_VGA_UPDATE_SCREEN, ZLOX_UINT8 *, ZLOX_UINT32);
ZLOX_DEFN_SYSCALL0(vga_get_text_font, ZLOX_SYSCALL_VGA_GET_TEXT_FONT);
ZLOX_DEFN_SYSCALL1(network_getinfo, ZLOX_SYSCALL_NETWORK_GETINFO, void *);
ZLOX_DEFN_SYSCALL1(network_set_focus_task, ZLOX_SYSCALL_NETWORK_SET_FOCUS_TASK, void *);
ZLOX_DEFN_SYSCALL2(network_send, ZLOX_SYSCALL_NETWORK_SEND, ZLOX_UINT8 *, ZLOX_UINT16);
ZLOX_DEFN_SYSCALL4(network_get_packet, ZLOX_SYSCALL_NETWORK_GET_PACKET, void *, ZLOX_SINT32 , ZLOX_UINT8 *, ZLOX_UINT16);
ZLOX_DEFN_SYSCALL0(timer_get_tick, ZLOX_SYSCALL_TIMER_GET_TICK);
ZLOX_DEFN_SYSCALL1(network_setinfo, ZLOX_SYSCALL_NETWORK_SETINFO, void *);
ZLOX_DEFN_SYSCALL1(pci_get_devconf_lst, ZLOX_SYSCALL_PCI_GET_DEVCONF_LST, void *);
ZLOX_DEFN_SYSCALL0(monitor_clear, ZLOX_SYSCALL_MONITOR_CLEAR);
ZLOX_DEFN_SYSCALL1(monitor_set_single, ZLOX_SYSCALL_MONITOR_SET_SINGLE, ZLOX_BOOL);
ZLOX_DEFN_SYSCALL2(monitor_set_cursor, ZLOX_SYSCALL_MONITOR_SET_CURSOR, ZLOX_UINT8, ZLOX_UINT8);
ZLOX_DEFN_SYSCALL4(writedir_fs_safe, ZLOX_SYSCALL_WRITEDIR_FS_SAFE, void * , ZLOX_CHAR *, ZLOX_UINT16 , void *);
ZLOX_DEFN_SYSCALL3(readdir_fs_safe, ZLOX_SYSCALL_READDIR_FS_SAFE, void * , ZLOX_UINT32 , void *);
ZLOX_DEFN_SYSCALL3(finddir_fs_safe, ZLOX_SYSCALL_FINDDIR_FS_SAFE, void *, ZLOX_CHAR *, void *);
ZLOX_DEFN_SYSCALL0(get_control_keys, ZLOX_SYSCALL_GET_CONTROL_KEYS);
ZLOX_DEFN_SYSCALL1(release_control_keys, ZLOX_SYSCALL_RELEASE_CONTROL_KEYS, ZLOX_UINT8);
ZLOX_DEFN_SYSCALL0(monitor_del_line, ZLOX_SYSCALL_MONITOR_DEL_LINE);
ZLOX_DEFN_SYSCALL0(monitor_insert_line, ZLOX_SYSCALL_MONITOR_INSERT_LINE);
ZLOX_DEFN_SYSCALL3(ps2_get_status, ZLOX_SYSCALL_PS2_GET_STATUS, ZLOX_BOOL *, ZLOX_BOOL *, ZLOX_BOOL *);

static ZLOX_VOID * syscalls[ZLOX_SYSCALL_NUMBER] =
{
	&zlox_monitor_write,
	&zlox_monitor_write_hex,
	&zlox_monitor_write_dec,
	&zlox_monitor_put,
	&zlox_execve,
	&zlox_get_tskmsg,
	&zlox_wait,
	&zlox_set_input_focus,
	&zlox_get_currentTask,
	&zlox_exit,
	&zlox_finish,
	&zlox_get_args,
	&zlox_get_init_esp,
	&zlox_umalloc,
	&zlox_ufree,
	&zlox_read_fs,
	&zlox_readdir_fs,
	&zlox_finddir_fs,
	&zlox_get_fs_root,
	&zlox_get_frame_info,
	&zlox_get_kheap,
	&zlox_get_version,
	&_zlox_reboot,
	&_zlox_shutdown,
	&_zlox_idle_cpu,
	&zlox_atapi_drive_read_sector,
	&zlox_atapi_drive_read_capacity,
	&zlox_ata_get_ide_info,
	&zlox_mount_iso,
	&zlox_unmount_iso,
	&zlox_overflow_test,
	&zlox_ide_ata_access,
	&zlox_mount_zenglfs,
	&zlox_unmount_zenglfs,
	&zlox_write_fs,
	&zlox_writedir_fs,
	&zlox_remove_fs,
	&zlox_rename_fs,
	&zlox_vga_set_mode,
	&zlox_vga_update_screen,
	&zlox_vga_get_text_font,
	&zlox_network_getinfo,
	&zlox_network_set_focus_task,
	&zlox_network_send,
	&zlox_network_get_packet,
	&zlox_timer_get_tick,
	&zlox_network_setinfo,
	&zlox_pci_get_devconf_lst,
	&zlox_monitor_clear,
	&zlox_monitor_set_single,
	&zlox_monitor_set_cursor,
	&zlox_writedir_fs_safe,
	&zlox_readdir_fs_safe,
	&zlox_finddir_fs_safe,
	&zlox_get_control_keys,
	&zlox_release_control_keys,
	&zlox_monitor_del_line,
	&zlox_monitor_insert_line,
	&zlox_ps2_get_status,
};

ZLOX_UINT32 num_syscalls = ZLOX_SYSCALL_NUMBER;

ZLOX_VOID zlox_initialise_syscalls()
{
	// Register our syscall handler.
	zlox_register_interrupt_callback (0x80, &zlox_syscall_handler);
}

static ZLOX_VOID zlox_syscall_handler(ZLOX_ISR_REGISTERS * regs)
{
	// Firstly, check if the requested syscall number is valid.
	// The syscall number is found in EAX.
	if (regs->eax >= num_syscalls)
		return;

	// Get the required syscall location.
	ZLOX_VOID * location = syscalls[regs->eax];

	// We don't know how many parameters the function wants, so we just
	// push them all onto the stack in the correct order. The function will
	// use all the parameters it wants, and we can pop them all back off afterwards.
	ZLOX_SINT32 ret;

	asm volatile (" \
	  push %1; \
	  push %2; \
	  push %3; \
	  push %4; \
	  push %5; \
	  call *%6; \
	  pop %%ebx; \
	  pop %%ebx; \
	  pop %%ebx; \
	  pop %%ebx; \
	  pop %%ebx; \
	" : "=a" (ret) : "D" (regs->edi), "S" (regs->esi), "d" (regs->edx), "c" (regs->ecx), "b" (regs->ebx), "0" (location));

	if(regs->eax == ZLOX_SYSCALL_EXECVE && 
		((ret != -1) && ((ZLOX_UINT32)ret > 0))
		)
	{
		regs->eip = ret;
	}

	regs->eax = ret;
}

ZLOX_SINT32 zlox_get_version(ZLOX_SINT32 * major, ZLOX_SINT32 * minor, ZLOX_SINT32 * revision)
{
	*major = ZLOX_MAJOR_VERSION;
	*minor = ZLOX_MINOR_VERSION;
	*revision = ZLOX_REVISION;
	return 0;
}

ZLOX_SINT32 zlox_overflow_test()
{
	ZLOX_CHAR test[128] = {0};
	test[0] = test[0] + 0;
	zlox_overflow_test(); // 无穷递归，迫使内核栈溢出，当内核栈溢出时，会触发 double fault，弹出详细的错误信息, 普通的page fault很难检测出内核栈溢出
	return 0;
}

