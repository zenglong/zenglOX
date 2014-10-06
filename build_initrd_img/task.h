// task.h Defines the structures and prototypes needed to multitask.

#ifndef _TASK_H_
#define _TASK_H_

#include "common.h"
#include "elf.h"

typedef enum _MSG_TYPE
{
	MT_KEYBOARD,
	MT_TASK_FINISH,
	MT_NET_PACKET,
}MSG_TYPE;

typedef enum _MSG_KB_KEY
{
	MKK_F1_PRESS = 0x3B00,
	MKK_F2_PRESS = 0x3C00,
	MKK_F3_PRESS = 0x3D00,
	MKK_F4_PRESS = 0x3E00,
	MKK_F5_PRESS = 0x3F00,
	MKK_F6_PRESS = 0x4000,
	MKK_F7_PRESS = 0x4100,
	MKK_F8_PRESS = 0x4200,
	MKK_F9_PRESS = 0x4300,
	MKK_F10_PRESS = 0x4400,
	MKK_HOME_PRESS = 0xE047,
	MKK_CURSOR_UP_PRESS = 0xE048,
	MKK_PAGE_UP_PRESS = 0xE049,
	MKK_CURSOR_LEFT_PRESS = 0xE04B,
	MKK_CURSOR_RIGHT_PRESS = 0xE04D,
	MKK_END_PRESS = 0xE04F,
	MKK_CURSOR_DOWN_PRESS = 0xE050,
	MKK_PAGE_DOWN_PRESS = 0xE051,
	MKK_INSERT_PRESS = 0xE052,
	MKK_DELETE_PRESS = 0xE053,
} MSG_KB_KEY;

typedef enum _MSG_KB_TYPE
{
	MKT_ASCII,
	MKT_KEY,
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
	MSG_KB_KEY key;
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
	SINT32 packet_idx;
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
	VOID * heap;
	UINT32 kernel_stack;   // Kernel stack location.
	TASK_MSG_LIST msglist; // task message.
	ELF_LINK_MAP_LIST link_maps; // elf link map list;
	TSK_STATUS status;	// task status.
	CHAR * args;	// task args
	TASK * next; // The next task in a linked list.
	TASK * prev; // the prev task
	TASK * parent; // parent task
};

#endif // _TASK_H_

