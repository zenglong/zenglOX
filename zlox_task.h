// zlox_task.h Defines the structures and prototypes needed to multitask.

#ifndef _ZLOX_TASK_H_
#define _ZLOX_TASK_H_

#include "zlox_common.h"

typedef struct _ZLOX_TASK ZLOX_TASK;

#include "zlox_paging.h"
#include "zlox_elf.h"

#define ZLOX_KERNEL_STACK_SIZE 0x2000	// Use a 8kb kernel stack.
#define ZLOX_USER_STACK_SIZE 0x2000	// Use a 8kb user stack.
#define ZLOX_TSK_MSGLIST_SIZE 20		// 任务消息列表的初始化及动态扩容的大小
#define ZLOX_TSK_SIGN 0x4B534154		// 任务结构体的有效标识，即"TASK"的ASCII码
#define ZLOX_PID_REUSE_LIST_SIZE 20	// PID重利用列表的初始化及动态扩容的大小

typedef enum _ZLOX_MSG_TYPE
{
	ZLOX_MT_KEYBOARD,
	ZLOX_MT_TASK_FINISH,
}ZLOX_MSG_TYPE;

typedef enum _ZLOX_MSG_KB_TYPE
{
	ZLOX_MKT_ASCII,
}ZLOX_MSG_KB_TYPE;

typedef enum _ZLOX_TSK_STATUS
{
	ZLOX_TS_WAIT,
	ZLOX_TS_ATA_WAIT,
	ZLOX_TS_RUNNING,
	ZLOX_TS_FINISH,
	ZLOX_TS_ZOMBIE,
}ZLOX_TSK_STATUS;

typedef struct _ZLOX_TASK_MSG_KEYBOARD
{
	ZLOX_MSG_KB_TYPE type;
	ZLOX_UINT32 ascii;
}ZLOX_TASK_MSG_KEYBOARD;

typedef struct _ZLOX_TASK_MSG_FINISH
{
	ZLOX_TASK * exit_task;
	ZLOX_SINT32 exit_code;
}ZLOX_TASK_MSG_FINISH;

typedef struct _ZLOX_TASK_MSG
{
	ZLOX_MSG_TYPE type;
	ZLOX_TASK_MSG_KEYBOARD keyboard;
	ZLOX_TASK_MSG_FINISH finish_task; // 消息中存储的结束任务的相关信息
}ZLOX_TASK_MSG;

typedef struct _ZLOX_TASK_MSG_LIST
{
	ZLOX_BOOL isInit;
	ZLOX_SINT32 count;
	ZLOX_SINT32 size;
	ZLOX_SINT32 cur;
	ZLOX_SINT32 finish_task_num;
	ZLOX_TASK_MSG * ptr;
}ZLOX_TASK_MSG_LIST;

// This structure defines a 'task' - a process.
struct _ZLOX_TASK
{
	ZLOX_UINT32 sign; // Process sign
	ZLOX_SINT32 id; // Process ID.
	ZLOX_UINT32 esp, ebp; // Stack and base pointers.
	ZLOX_UINT32 eip; // Instruction pointer.
	ZLOX_UINT32 init_esp; // stack top
	ZLOX_PAGE_DIRECTORY * page_directory; // Page directory.
	ZLOX_UINT32 kernel_stack;   // Kernel stack location.
	ZLOX_TASK_MSG_LIST msglist; // task message.
	ZLOX_ELF_LINK_MAP_LIST link_maps; // elf link map list;
	ZLOX_TSK_STATUS status;	// task status.
	ZLOX_CHAR * args;	// task args
	ZLOX_TASK * next; // The next task in a linked list.
	ZLOX_TASK * prev; // the prev task
	ZLOX_TASK * parent; // parent task
};

typedef struct _ZLOX_PID_REUSE_LIST
{
	ZLOX_BOOL isInit;
	ZLOX_SINT32 count;
	ZLOX_SINT32 size;
	ZLOX_SINT32 * ptr;
}ZLOX_PID_REUSE_LIST;

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

// 向task任务发送消息
ZLOX_SINT32 zlox_send_tskmsg(ZLOX_TASK * task , ZLOX_TASK_MSG * msg);

// 从task任务中获取消息
ZLOX_SINT32 zlox_get_tskmsg(ZLOX_TASK * task,ZLOX_TASK_MSG * msgbuf,ZLOX_BOOL needPop);

// 将task任务设置为wait等待状态
ZLOX_SINT32 zlox_wait(ZLOX_TASK * task);

// 将task任务设置为获取键盘之类的输入数据的任务，这样键盘就会向该任务发送按键消息
ZLOX_SINT32 zlox_set_input_focus(ZLOX_TASK * task);

// 获取当前任务的指针值
ZLOX_TASK * zlox_get_currentTask();

// 结束当前任务，并向父任务或首任务发送结束消息
ZLOX_SINT32 zlox_exit(ZLOX_SINT32 exit_code);

// 将需要结束的任务的相关资源给释放掉，并从任务列表里移除
ZLOX_SINT32 zlox_finish(ZLOX_TASK * task);

// 获取task任务的参数
ZLOX_UINT32 zlox_get_args(ZLOX_TASK * task);

// 获取task任务用户栈的初始值
ZLOX_UINT32 zlox_get_init_esp(ZLOX_TASK * task);

#endif // _ZLOX_TASK_H_

