// entry.c -- 主入口函数

#include "common.h"
#include "syscall.h"
#include "task.h"

int main(VOID * task, int argc, char * argv[]);

int entry()
{
	VOID * task = (VOID *)syscall_get_currentTask();
	VOID * location = &main;
	char * args = ((char *)syscall_get_init_esp(task)) - 1;
	char * orig_args = (char *)syscall_get_args(task);
	int arg_num = 0;
	int ret;
	int length = strlen(orig_args);
	int i = length - 1;

	if(args >= orig_args)
		reverse_memcpy((UINT8 *)args,(UINT8 *)(orig_args + length),length + 1);
	else
		memcpy((UINT8 *)(args - length),(UINT8 *)orig_args,length + 1);

	args -= length;
	asm volatile("movl %0,%%esp"::"r"((UINT32)args));

	for(;i>=0;i--)
	{
		if(args[i] == ' ' && args[i+1] != ' ' && args[i+1] != '\0')
		{
			arg_num++;
			asm volatile("push %0" :: "r"((UINT32)(args + i + 1)));
		}
	}

	if(args[0] != ' ' && args[0] != '\0')
	{
		arg_num++;
		asm volatile("push %0" :: "r"((UINT32)args));
	}
	
	for(i=0;i < length;i++)
	{
		if(args[i] == ' ')
		{
			args[i] = '\0';
		}
	}

	asm volatile(" \
		movl %esp,%eax; \
		push %eax; \
		");
	
	asm volatile("push %1; \
			push %2; \
			call *%3; \
			" : "=a" (ret) : "b"(arg_num), "c"((UINT32)task), "0" (location));
	
	syscall_exit(ret);
	return 0;
}

