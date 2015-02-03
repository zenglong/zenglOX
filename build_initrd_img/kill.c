// kill.c -- 终止指定的进程

#include "common.h"
#include "syscall.h"
#include "task.h"

int main(TASK * task, int argc, char * argv[])
{
	if(argc == 2 && strIsNum(argv[1]))
	{
		UINT32 pid;
		pid = strToUInt(argv[1]);
		TASK * first_task;
		TASK * tmp_task;
		TASK * kill_task = NULL;
		tmp_task = task;
		while(tmp_task != 0)
		{
			first_task = tmp_task;
			tmp_task = first_task->prev;
		}
		tmp_task = first_task;
		do{
			if(tmp_task->id == (SINT32)pid)
			{
				kill_task = tmp_task;
				break;
			}
			tmp_task = tmp_task->next;
		}while(tmp_task != 0);

		// 不能将当前任务, 或者将与当前任务具有"血缘"关系的任务给终止掉. (比如kill的父任务,或kill的父任务的父任务等)
		if(kill_task != NULL)
		{
			tmp_task = task;
			while(TRUE)
			{
				if(tmp_task == kill_task)
				{
					kill_task = NULL;
					break;
				}
				else if(tmp_task == first_task)
					break;
				tmp_task = tmp_task->parent;
			}
		}

		if(kill_task == NULL)
		{
			syscall_cmd_window_write("invalid pid or the pid is not allowed");
			return -1;
		}
		syscall_finish_all_child(kill_task);
		syscall_exit_do(kill_task, -1, FALSE);
	}
	else
	{
		syscall_cmd_window_write("usage: kill pid");
	}
	return 0;
}

