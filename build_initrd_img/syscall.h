// syscall.h Defines the interface for and structures relating to the syscall dispatch system.

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "common.h"

typedef enum _SYSCALL_ENUM{
	SYSCALL_MONITOR_WRITE,
	SYSCALL_MONITOR_WRITE_HEX,
	SYSCALL_MONITOR_WRITE_DEC,
	SYSCALL_MONITOR_PUT,
	SYSCALL_EXECVE,
	SYSCALL_GET_TSKMSG,
	SYSCALL_WAIT,
	SYSCALL_SET_INPUT_FOCUS,
	SYSCALL_GET_CURRENT_TASK,
	SYSCALL_EXIT,
	SYSCALL_FINISH,
	SYSCALL_GET_ARGS,
	SYSCALL_GET_INIT_ESP,
	SYSCALL_UMALLOC,
	SYSCALL_UFREE,
	SYSCALL_READ_FS,
	SYSCALL_READDIR_FS,
	SYSCALL_FINDDIR_FS,
	SYSCALL_GET_FS_ROOT,
	SYSCALL_GET_FRAME_INFO,
	SYSCALL_GET_KHEAP,
	SYSCALL_GET_VERSION,
	SYSCALL_REBOOT,
	SYSCALL_SHUTDOWN,
	SYSCALL_IDLE_CPU,
	SYSCALL_ATAPI_DRIVE_READ_SECTOR,
	SYSCALL_ATAPI_DRIVE_READ_CAPACITY,
	SYSCALL_ATA_GET_IDE_INFO,
	SYSCALL_MOUNT_ISO,
	SYSCALL_UNMOUNT_ISO,
	SYSCALL_OVERFLOW_TEST,
	SYSCALL_IDE_ATA_ACCESS,
	SYSCALL_MOUNT_ZENGLFS,
	SYSCALL_UNMOUNT_ZENGLFS,
	SYSCALL_WRITE_FS,
	SYSCALL_WRITEDIR_FS,
	SYSCALL_REMOVE_FS,
	SYSCALL_RENAME_FS,
	SYSCALL_VGA_SET_MODE,
	SYSCALL_VGA_UPDATE_SCREEN,
	SYSCALL_VGA_GET_TEXT_FONT,
	SYSCALL_NETWORK_GETINFO,
	SYSCALL_NETWORK_SET_FOCUS_TASK,
	SYSCALL_NETWORK_SEND,
	SYSCALL_NETWORK_GET_PACKET,
	SYSCALL_TIMER_GET_TICK,
	SYSCALL_NETWORK_SETINFO,
	SYSCALL_PCI_GET_DEVCONF_LST,
	SYSCALL_MONITOR_CLEAR,
	SYSCALL_MONITOR_SET_SINGLE,
	SYSCALL_MONITOR_SET_CURSOR,
	SYSCALL_WRITEDIR_FS_SAFE,
	SYSCALL_READDIR_FS_SAFE,
	SYSCALL_FINDDIR_FS_SAFE,
	SYSCALL_GET_CONTROL_KEYS,
	SYSCALL_RELEASE_CONTROL_KEYS,
	SYSCALL_MONITOR_DEL_LINE,
	SYSCALL_MONITOR_INSERT_LINE,
	SYSCALL_PS2_GET_STATUS,
	SYSCALL_CREATE_MY_WINDOW,
	SYSCALL_DISPATCH_WIN_MSG,
	SYSCALL_MOUSE_SET_SCALE,
	SYSCALL_SET_CMD_WINDOW,
	SYSCALL_CMD_WINDOW_PUT,
	SYSCALL_CMD_WINDOW_WRITE,
	SYSCALL_FINISH_ALL_CHILD,
	SYSCALL_CMD_WINDOW_WRITE_HEX,
	SYSCALL_CMD_WINDOW_WRITE_DEC,
	SYSCALL_CMD_WINDOW_DEL_LINE,
	SYSCALL_CMD_WINDOW_INSERT_LINE,
	SYSCALL_CMD_WINDOW_SET_CURSOR,
	SYSCALL_CMD_WINDOW_SET_SINGLE,
	SYSCALL_CMD_WINDOW_CLEAR,
}SYSCALL_ENUM;

#define DECL_SYSCALL0(fn) SINT32 syscall_##fn();
#define DECL_SYSCALL1(fn,p1) SINT32 syscall_##fn(p1);
#define DECL_SYSCALL2(fn,p1,p2) SINT32 syscall_##fn(p1,p2);
#define DECL_SYSCALL3(fn,p1,p2,p3) SINT32 syscall_##fn(p1,p2,p3);
#define DECL_SYSCALL4(fn,p1,p2,p3,p4) SINT32 syscall_##fn(p1,p2,p3,p4);
#define DECL_SYSCALL5(fn,p1,p2,p3,p4,p5) SINT32 syscall_##fn(p1,p2,p3,p4,p5);

#define DEFN_SYSCALL0(fn, num) \
SINT32 syscall_##fn() \
{ \
  SINT32 a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
  return a; \
}

#define DEFN_SYSCALL1(fn, num, P1) \
SINT32 syscall_##fn(P1 p1) \
{ \
  SINT32 a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((SINT32)p1)); \
  return a; \
}

#define DEFN_SYSCALL2(fn, num, P1, P2) \
SINT32 syscall_##fn(P1 p1, P2 p2) \
{ \
  SINT32 a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((SINT32)p1), "c" ((SINT32)p2)); \
  return a; \
}

#define DEFN_SYSCALL3(fn, num, P1, P2, P3) \
SINT32 syscall_##fn(P1 p1, P2 p2, P3 p3) \
{ \
  SINT32 a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((SINT32)p1), "c" ((SINT32)p2), "d"((SINT32)p3)); \
  return a; \
}

#define DEFN_SYSCALL4(fn, num, P1, P2, P3, P4) \
SINT32 syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4) \
{ \
  SINT32 a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((SINT32)p1), "c" ((SINT32)p2), "d" ((SINT32)p3), "S" ((SINT32)p4)); \
  return a; \
}

#define DEFN_SYSCALL5(fn, num,P1, P2, P3, P4, P5) \
SINT32 syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) \
{ \
  SINT32 a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((SINT32)p1), "c" ((SINT32)p2), "d" ((SINT32)p3), "S" ((SINT32)p4), "D" ((SINT32)p5)); \
  return a; \
}

DECL_SYSCALL1(monitor_write, const char*)
DECL_SYSCALL1(monitor_write_hex, UINT32)
DECL_SYSCALL1(monitor_write_dec, UINT32)
DECL_SYSCALL1(monitor_put, char)
DECL_SYSCALL1(execve,const char*)
DECL_SYSCALL3(get_tskmsg,void *,void *,BOOL)
DECL_SYSCALL1(wait,void *)
DECL_SYSCALL1(set_input_focus,void *)
DECL_SYSCALL0(get_currentTask)
DECL_SYSCALL1(exit,int)
DECL_SYSCALL1(finish,void *)
DECL_SYSCALL1(get_args,void *)
DECL_SYSCALL1(get_init_esp,void *)
DECL_SYSCALL1(umalloc,int)
DECL_SYSCALL1(ufree,void *)
DECL_SYSCALL4(read_fs,void *,int,int,void *)
DECL_SYSCALL2(readdir_fs,void *,int)
DECL_SYSCALL2(finddir_fs,void *,void *)
DECL_SYSCALL0(get_fs_root)
DECL_SYSCALL2(get_frame_info,void **,void *)
DECL_SYSCALL0(get_kheap)
DECL_SYSCALL3(get_version,void *,void *,void *)
DECL_SYSCALL0(reboot)
DECL_SYSCALL0(shutdown)
DECL_SYSCALL0(idle_cpu)
DECL_SYSCALL3(atapi_drive_read_sector,UINT32,UINT32,void *)
DECL_SYSCALL2(atapi_drive_read_capacity,UINT32,void *)
DECL_SYSCALL0(ata_get_ide_info)
DECL_SYSCALL0(mount_iso)
DECL_SYSCALL0(unmount_iso)
DECL_SYSCALL0(overflow_test)
DECL_SYSCALL5(ide_ata_access, UINT8, UINT8, UINT32, UINT8, UINT8 *)
DECL_SYSCALL2(mount_zenglfs, UINT32 , UINT32)
DECL_SYSCALL0(unmount_zenglfs)
DECL_SYSCALL4(write_fs, void *, UINT32 , UINT32 , UINT8 *)
DECL_SYSCALL3(writedir_fs, void *, CHAR *, UINT16)
DECL_SYSCALL1(remove_fs, void *)
DECL_SYSCALL2(rename_fs, void *, CHAR *)
DECL_SYSCALL1(vga_set_mode, UINT32)
DECL_SYSCALL2(vga_update_screen, UINT8 *, UINT32)
DECL_SYSCALL0(vga_get_text_font)
DECL_SYSCALL1(network_getinfo, void *)
DECL_SYSCALL1(network_set_focus_task, void *)
DECL_SYSCALL2(network_send, UINT8 *, UINT16)
DECL_SYSCALL4(network_get_packet, void *, SINT32 , UINT8 *, UINT16)
DECL_SYSCALL0(timer_get_tick)
DECL_SYSCALL1(network_setinfo, void *)
DECL_SYSCALL1(pci_get_devconf_lst, void *)
DECL_SYSCALL0(monitor_clear)
DECL_SYSCALL1(monitor_set_single, BOOL)
DECL_SYSCALL2(monitor_set_cursor, UINT8, UINT8)
DECL_SYSCALL4(writedir_fs_safe, void * , CHAR *, UINT16 , void *)
DECL_SYSCALL3(readdir_fs_safe, void * , UINT32 , void *)
DECL_SYSCALL3(finddir_fs_safe, void *, CHAR *, void *)
DECL_SYSCALL0(get_control_keys)
DECL_SYSCALL1(release_control_keys, UINT8)
DECL_SYSCALL0(monitor_del_line)
DECL_SYSCALL0(monitor_insert_line)
DECL_SYSCALL3(ps2_get_status, BOOL *, BOOL *, BOOL *)
DECL_SYSCALL1(create_my_window, void *)
DECL_SYSCALL2(dispatch_win_msg, void *, void *)
DECL_SYSCALL1(mouse_set_scale, FLOAT *)
DECL_SYSCALL3(set_cmd_window, void *, void *, BOOL)
DECL_SYSCALL1(cmd_window_put, char)
DECL_SYSCALL1(cmd_window_write, const char *)
DECL_SYSCALL1(finish_all_child, void *)
DECL_SYSCALL1(cmd_window_write_hex, UINT32)
DECL_SYSCALL1(cmd_window_write_dec, UINT32)
DECL_SYSCALL0(cmd_window_del_line)
DECL_SYSCALL0(cmd_window_insert_line)
DECL_SYSCALL2(cmd_window_set_cursor, SINT32, SINT32)
DECL_SYSCALL1(cmd_window_set_single, BOOL)
DECL_SYSCALL0(cmd_window_clear)

#endif // _SYSCALL_H_

