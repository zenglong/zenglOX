// zlox_syscall.c Defines the implementation of a system call system.

#include "zlox_syscall.h"
#include "zlox_isr.h"
#include "zlox_monitor.h"

static ZLOX_VOID zlox_syscall_handler(ZLOX_ISR_REGISTERS * regs);

ZLOX_DEFN_SYSCALL1(monitor_write, 0, const char*);
ZLOX_DEFN_SYSCALL1(monitor_write_hex, 1, const char*);
ZLOX_DEFN_SYSCALL1(monitor_write_dec, 2, const char*);

static ZLOX_VOID * syscalls[3] =
{
    &zlox_monitor_write,
    &zlox_monitor_write_hex,
    &zlox_monitor_write_dec,
};

ZLOX_UINT32 num_syscalls = 3;

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

    regs->eax = ret;
}

