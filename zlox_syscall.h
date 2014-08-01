// zlox_syscall.h Defines the interface for and structures relating to the syscall dispatch system.

#ifndef _ZLOX_SYSCALL_H_
#define _ZLOX_SYSCALL_H_

#include "zlox_common.h"

#define ZLOX_SYSCALL_NUMBER 41

#define ZLOX_MAJOR_VERSION 1 //zenglOX 主版本号
#define ZLOX_MINOR_VERSION 6 //zenglOX 子版本号
#define ZLOX_REVISION 0      //zenglOX 修正版本号

typedef enum _ZLOX_SYSCALL_ENUM{
	ZLOX_SYSCALL_MONITOR_WRITE,
	ZLOX_SYSCALL_MONITOR_WRITE_HEX,
	ZLOX_SYSCALL_MONITOR_WRITE_DEC,
	ZLOX_SYSCALL_MONITOR_PUT,
	ZLOX_SYSCALL_EXECVE,
	ZLOX_SYSCALL_GET_TSKMSG,
	ZLOX_SYSCALL_WAIT,
	ZLOX_SYSCALL_SET_INPUT_FOCUS,
	ZLOX_SYSCALL_GET_CURRENT_TASK,
	ZLOX_SYSCALL_EXIT,
	ZLOX_SYSCALL_FINISH,
	ZLOX_SYSCALL_GET_ARGS,
	ZLOX_SYSCALL_GET_INIT_ESP,
	ZLOX_SYSCALL_UMALLOC,
	ZLOX_SYSCALL_UFREE,
	ZLOX_SYSCALL_READ_FS,
	ZLOX_SYSCALL_READDIR_FS,
	ZLOX_SYSCALL_FINDDIR_FS,
	ZLOX_SYSCALL_GET_FS_ROOT,
	ZLOX_SYSCALL_GET_FRAME_INFO,
	ZLOX_SYSCALL_GET_KHEAP,
	ZLOX_SYSCALL_GET_VERSION,
	ZLOX_SYSCALL_REBOOT,
	ZLOX_SYSCALL_SHUTDOWN,
	ZLOX_SYSCALL_IDLE_CPU,
	ZLOX_SYSCALL_ATAPI_DRIVE_READ_SECTOR,
	ZLOX_SYSCALL_ATAPI_DRIVE_READ_CAPACITY,
	ZLOX_SYSCALL_ATA_GET_IDE_INFO,
	ZLOX_SYSCALL_MOUNT_ISO,
	ZLOX_SYSCALL_UNMOUNT_ISO,
	ZLOX_SYSCALL_OVERFLOW_TEST,
	ZLOX_SYSCALL_IDE_ATA_ACCESS,
	ZLOX_SYSCALL_MOUNT_ZENGLFS,
	ZLOX_SYSCALL_UNMOUNT_ZENGLFS,
	ZLOX_SYSCALL_WRITE_FS,
	ZLOX_SYSCALL_WRITEDIR_FS,
	ZLOX_SYSCALL_REMOVE_FS,
	ZLOX_SYSCALL_RENAME_FS,
	ZLOX_SYSCALL_VGA_SET_MODE,
	ZLOX_SYSCALL_VGA_UPDATE_SCREEN,
	ZLOX_SYSCALL_VGA_GET_TEXT_FONT,
}ZLOX_SYSCALL_ENUM;

ZLOX_VOID zlox_initialise_syscalls();

#define ZLOX_DECL_SYSCALL0(fn) ZLOX_SINT32 zlox_syscall_##fn();
#define ZLOX_DECL_SYSCALL1(fn,p1) ZLOX_SINT32 zlox_syscall_##fn(p1);
#define ZLOX_DECL_SYSCALL2(fn,p1,p2) ZLOX_SINT32 zlox_syscall_##fn(p1,p2);
#define ZLOX_DECL_SYSCALL3(fn,p1,p2,p3) ZLOX_SINT32 zlox_syscall_##fn(p1,p2,p3);
#define ZLOX_DECL_SYSCALL4(fn,p1,p2,p3,p4) ZLOX_SINT32 zlox_syscall_##fn(p1,p2,p3,p4);
#define ZLOX_DECL_SYSCALL5(fn,p1,p2,p3,p4,p5) ZLOX_SINT32 zlox_syscall_##fn(p1,p2,p3,p4,p5);

#define ZLOX_DEFN_SYSCALL0(fn, num) \
ZLOX_SINT32 zlox_syscall_##fn() \
{ \
  ZLOX_SINT32 a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
  return a; \
}

#define ZLOX_DEFN_SYSCALL1(fn, num, P1) \
ZLOX_SINT32 zlox_syscall_##fn(P1 p1) \
{ \
  ZLOX_SINT32 a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((ZLOX_SINT32)p1)); \
  return a; \
}

#define ZLOX_DEFN_SYSCALL2(fn, num, P1, P2) \
ZLOX_SINT32 zlox_syscall_##fn(P1 p1, P2 p2) \
{ \
  ZLOX_SINT32 a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((ZLOX_SINT32)p1), "c" ((ZLOX_SINT32)p2)); \
  return a; \
}

#define ZLOX_DEFN_SYSCALL3(fn, num, P1, P2, P3) \
ZLOX_SINT32 zlox_syscall_##fn(P1 p1, P2 p2, P3 p3) \
{ \
  ZLOX_SINT32 a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((ZLOX_SINT32)p1), "c" ((ZLOX_SINT32)p2), "d"((ZLOX_SINT32)p3)); \
  return a; \
}

#define ZLOX_DEFN_SYSCALL4(fn, num, P1, P2, P3, P4) \
ZLOX_SINT32 zlox_syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4) \
{ \
  ZLOX_SINT32 a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((ZLOX_SINT32)p1), "c" ((ZLOX_SINT32)p2), "d" ((ZLOX_SINT32)p3), "S" ((ZLOX_SINT32)p4)); \
  return a; \
}

#define ZLOX_DEFN_SYSCALL5(fn, num,P1, P2, P3, P4, P5) \
ZLOX_SINT32 zlox_syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) \
{ \
  ZLOX_SINT32 a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((ZLOX_SINT32)p1), "c" ((ZLOX_SINT32)p2), "d" ((ZLOX_SINT32)p3), "S" ((ZLOX_SINT32)p4), "D" ((ZLOX_SINT32)p5)); \
  return a; \
}

ZLOX_DECL_SYSCALL1(monitor_write, const char*)
ZLOX_DECL_SYSCALL1(monitor_write_hex, ZLOX_UINT32)
ZLOX_DECL_SYSCALL1(monitor_write_dec, ZLOX_UINT32)
ZLOX_DECL_SYSCALL1(monitor_put, char)
ZLOX_DECL_SYSCALL1(execve,const char*)
ZLOX_DECL_SYSCALL3(get_tskmsg,void *,void *,ZLOX_BOOL)
ZLOX_DECL_SYSCALL1(wait,void *)
ZLOX_DECL_SYSCALL1(set_input_focus,void *)
ZLOX_DECL_SYSCALL0(get_currentTask)
ZLOX_DECL_SYSCALL1(exit,int)
ZLOX_DECL_SYSCALL1(finish,void *)
ZLOX_DECL_SYSCALL1(get_args,void *)
ZLOX_DECL_SYSCALL1(get_init_esp,void *)
ZLOX_DECL_SYSCALL1(umalloc,int)
ZLOX_DECL_SYSCALL1(ufree,void *)
ZLOX_DECL_SYSCALL4(read_fs,void *,int,int,void *)
ZLOX_DECL_SYSCALL2(readdir_fs,void *,int)
ZLOX_DECL_SYSCALL2(finddir_fs,void *,void *)
ZLOX_DECL_SYSCALL0(get_fs_root)
ZLOX_DECL_SYSCALL2(get_frame_info,void **,void *)
ZLOX_DECL_SYSCALL0(get_kheap)
ZLOX_DECL_SYSCALL3(get_version,void *,void *,void *)
ZLOX_DECL_SYSCALL0(reboot)
ZLOX_DECL_SYSCALL0(shutdown)
ZLOX_DECL_SYSCALL0(idle_cpu)
ZLOX_DECL_SYSCALL3(atapi_drive_read_sector,ZLOX_UINT32,ZLOX_UINT32,void *)
ZLOX_DECL_SYSCALL2(atapi_drive_read_capacity,ZLOX_UINT32,void *)
ZLOX_DECL_SYSCALL0(ata_get_ide_info)
ZLOX_DECL_SYSCALL0(mount_iso)
ZLOX_DECL_SYSCALL0(unmount_iso)
ZLOX_DECL_SYSCALL0(overflow_test)
ZLOX_DECL_SYSCALL5(ide_ata_access, ZLOX_UINT8, ZLOX_UINT8, ZLOX_UINT32, ZLOX_UINT8, ZLOX_UINT8 *)
ZLOX_DECL_SYSCALL2(mount_zenglfs, ZLOX_UINT32, ZLOX_UINT32)
ZLOX_DECL_SYSCALL0(unmount_zenglfs)
ZLOX_DECL_SYSCALL4(write_fs, void *, ZLOX_UINT32 , ZLOX_UINT32 , ZLOX_UINT8 *)
ZLOX_DECL_SYSCALL3(writedir_fs, void *, ZLOX_CHAR *, ZLOX_UINT16)
ZLOX_DECL_SYSCALL1(remove_fs, void *)
ZLOX_DECL_SYSCALL2(rename_fs, void *, ZLOX_CHAR *)
ZLOX_DECL_SYSCALL1(vga_set_mode, ZLOX_UINT32)
ZLOX_DECL_SYSCALL2(vga_update_screen, ZLOX_UINT8 *, ZLOX_UINT32)
ZLOX_DECL_SYSCALL0(vga_get_text_font)

ZLOX_SINT32 zlox_get_version(ZLOX_SINT32 * major, ZLOX_SINT32 * minor, ZLOX_SINT32 * revision);

#endif // _ZLOX_SYSCALL_H_

