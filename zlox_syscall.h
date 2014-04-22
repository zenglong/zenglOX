// zlox_syscall.h Defines the interface for and structures relating to the syscall dispatch system.

#ifndef _ZLOX_SYSCALL_H_
#define _ZLOX_SYSCALL_H_

#include "zlox_common.h"

#define ZLOX_SYSCALL_NUMBER 4

typedef enum _ZLOX_SYSCALL_ENUM{
	ZLOX_SYSCALL_MONITOR_WRITE,
	ZLOX_SYSCALL_MONITOR_WRITE_HEX,
	ZLOX_SYSCALL_MONITOR_WRITE_DEC,
	ZLOX_SYSCALL_MONITOR_PUT,
}ZLOX_SYSCALL_ENUM;

ZLOX_VOID zlox_initialise_syscalls();

#define ZLOX_DECL_SYSCALL1(fn,p1) ZLOX_SINT32 zlox_syscall_##fn(p1);

#define ZLOX_DEFN_SYSCALL1(fn, num, P1) \
int zlox_syscall_##fn(P1 p1) \
{ \
  ZLOX_SINT32 a; \
  asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((ZLOX_SINT32)p1)); \
  return a; \
}

ZLOX_DECL_SYSCALL1(monitor_write, const char*)
ZLOX_DECL_SYSCALL1(monitor_write_hex, const char*)
ZLOX_DECL_SYSCALL1(monitor_write_dec, const char*)
ZLOX_DECL_SYSCALL1(monitor_put, char)

#endif // _ZLOX_SYSCALL_H_

