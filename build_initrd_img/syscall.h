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

#endif // _SYSCALL_H_

