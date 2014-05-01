// zlox_syscall.c Defines the implementation of a system call system.

#include "zlox_syscall.h"
#include "zlox_isr.h"
#include "zlox_monitor.h"
#include "zlox_keyboard.h"
#include "zlox_elf.h"

static ZLOX_VOID zlox_syscall_handler(ZLOX_ISR_REGISTERS * regs);

ZLOX_DEFN_SYSCALL1(monitor_write, ZLOX_SYSCALL_MONITOR_WRITE, const char*);
ZLOX_DEFN_SYSCALL1(monitor_write_hex, ZLOX_SYSCALL_MONITOR_WRITE_HEX, const char*);
ZLOX_DEFN_SYSCALL1(monitor_write_dec, ZLOX_SYSCALL_MONITOR_WRITE_DEC, const char*);
ZLOX_DEFN_SYSCALL1(monitor_put, ZLOX_SYSCALL_MONITOR_PUT, char);
ZLOX_DEFN_SYSCALL1(execve, ZLOX_SYSCALL_EXECVE, const char*);

static ZLOX_VOID * syscalls[ZLOX_SYSCALL_NUMBER] =
{
	&zlox_monitor_write,
	&zlox_monitor_write_hex,
	&zlox_monitor_write_dec,
	&zlox_monitor_put,
	&zlox_execve,
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

	ZLOX_UINT32 oldesp,newesp; 

	// We don't know how many parameters the function wants, so we just
	// push them all onto the stack in the correct order. The function will
	// use all the parameters it wants, and we can pop them all back off afterwards.
	ZLOX_SINT32 ret;

	asm volatile("mov %%esp, %0" : "=r"(oldesp));

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

	asm volatile("mov %%esp, %0" : "=r"(newesp));

	if(oldesp != newesp)
	{
		if(oldesp > newesp)
			regs = (ZLOX_ISR_REGISTERS *)((ZLOX_UINT32)regs - (oldesp - newesp));
		else
			regs = (ZLOX_ISR_REGISTERS *)((ZLOX_UINT32)regs + (newesp - oldesp));
	}

	if(regs->eax == ZLOX_SYSCALL_EXECVE && ret > 0)
	{
		regs->eip = ret;
	}

	regs->eax = ret;
}

