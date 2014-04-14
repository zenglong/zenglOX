// zlox_task.h Defines the structures and prototypes needed to multitask.

#ifndef _ZLOX_TASK_H_
#define _ZLOX_TASK_H_

#include "zlox_common.h"
#include "zlox_paging.h"

#define ZLOX_KERNEL_STACK_SIZE 2048	// Use a 2kb kernel stack.

typedef struct _ZLOX_TASK ZLOX_TASK;

// This structure defines a 'task' - a process.
struct _ZLOX_TASK
{
	ZLOX_SINT32 id; // Process ID.
	ZLOX_UINT32 esp, ebp; // Stack and base pointers.
	ZLOX_UINT32 eip; // Instruction pointer.
	ZLOX_UINT32 init_esp; // stack top
	ZLOX_PAGE_DIRECTORY * page_directory; // Page directory.
	ZLOX_UINT32 kernel_stack;   // Kernel stack location.
	ZLOX_TASK * next; // The next task in a linked list.
};

// Initialises the tasking system.
ZLOX_VOID zlox_initialise_tasking();

// Called by the timer hook, this changes the running process.
ZLOX_VOID zlox_switch_task();

// Forks the current process, spawning a new one
ZLOX_SINT32 zlox_fork();

// Returns the pid of the current process.
ZLOX_SINT32 zlox_getpid();

// switch to ring 3
ZLOX_VOID zlox_switch_to_user_mode();

#endif // _ZLOX_TASK_H_

