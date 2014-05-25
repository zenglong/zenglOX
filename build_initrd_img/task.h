// task.h Defines the structures and prototypes needed to multitask.

#ifndef _TASK_H_
#define _TASK_H_

#include "common.h"

typedef enum _MSG_TYPE
{
	MT_KEYBOARD,
	MT_TASK_FINISH,
}MSG_TYPE;

typedef enum _MSG_KB_TYPE
{
	MKT_ASCII,
}MSG_KB_TYPE;

typedef enum _TSK_STATUS
{
	TS_WAIT,
	TS_ATA_WAIT,
	TS_RUNNING,
	TS_FINISH,
	TS_ZOMBIE,
}TSK_STATUS;

typedef struct _TASK_MSG_KEYBOARD
{
	MSG_KB_TYPE type;
	UINT32 ascii;
}TASK_MSG_KEYBOARD;

typedef struct _TASK TASK;

typedef struct _TASK_MSG_FINISH
{
	TASK * exit_task;
	SINT32 exit_code;
}TASK_MSG_FINISH;

typedef struct _TASK_MSG
{
	MSG_TYPE type;
	TASK_MSG_KEYBOARD keyboard;
	TASK_MSG_FINISH finish_task; // 消息中存储的结束任务的相关信息
}TASK_MSG;

typedef struct _TASK_MSG_LIST
{
	BOOL isInit;
	SINT32 count;
	SINT32 size;
	SINT32 cur;
	SINT32 finish_task_num;
	TASK_MSG * ptr;
}TASK_MSG_LIST;

// This structure defines a 'task' - a process.
struct _TASK
{
	UINT32 sign; // Process sign
	SINT32 id; // Process ID.
	UINT32 esp, ebp; // Stack and base pointers.
	UINT32 eip; // Instruction pointer.
	UINT32 init_esp; // stack top
	VOID * page_directory; // Page directory.
	UINT32 kernel_stack;   // Kernel stack location.
	TASK_MSG_LIST msglist; // task message.
	TSK_STATUS status;	// task status.
	CHAR * args;	// task args
	TASK * next; // The next task in a linked list.
	TASK * prev; // the prev task
	TASK * parent; // parent task
};

#endif // _TASK_H_

