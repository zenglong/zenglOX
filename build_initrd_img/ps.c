// ps.c -- 列举出任务信息和内存信息

#include "common.h"
#include "syscall.h"
#include "task.h"
#include "kheap.h"

int main(TASK * task, int argc, char * argv[])
{
	if(argc == 1)
	{
		TASK * first_task;
		TASK * tmp_task;
		tmp_task = task;
		while(tmp_task != 0)
		{
			first_task = tmp_task;
			tmp_task = first_task->prev;
		}
		tmp_task = first_task;
		do{
			syscall_monitor_write("ID:");
			syscall_monitor_write_dec(tmp_task->id);
			syscall_monitor_write(" status:");
			switch(tmp_task->status)
			{
			case TS_WAIT:
				syscall_monitor_write("wait");
				break;
			case TS_RUNNING:
				syscall_monitor_write("running");
				break;
			case TS_FINISH:
				syscall_monitor_write("finish");
				break;
			case TS_ZOMBIE:
				syscall_monitor_write("zombie");
				break;
			}
			syscall_monitor_write(" args:\"");
			if(tmp_task->args != 0)
				syscall_monitor_write(tmp_task->args);
			syscall_monitor_put('"');
			if(tmp_task->parent == 0)
				syscall_monitor_write(" system first task");
			else
			{
				syscall_monitor_write(" ParentID:");
				syscall_monitor_write_dec(tmp_task->parent->id);
			}
			syscall_monitor_put('\n');
			tmp_task = tmp_task->next;
		}while(tmp_task != 0);
	}
	else if(argc > 1 && strcmp(argv[1],"-m")==0)
	{
		UINT32 * frames;
		UINT32 nframes;
		UINT32 i, j, count;
		UINT32 nframe_lines;
		syscall_get_frame_info((void **)&frames,&nframes);
		nframe_lines = nframes/(8*4);
		count = 0;
		for (i = 0; i < nframe_lines; i++)
		{
			if(frames[i] == 0xFFFFFFFF)
				count += 32;
			else if(frames[i] == 0)
				continue;
			else if (frames[i] != 0xFFFFFFFF) // nothing free, exit early.
			{
				// at least one bit is free here.
				for (j = 0; j < 32; j++)
				{
					UINT32 toTest = 0x1 << j;
					if (frames[i]&toTest)
					{
						count++;
					}
				}
			}
		}
		syscall_monitor_write("total memory: ");
		syscall_monitor_write_dec(nframes * 4);
		syscall_monitor_write(" Kbyte\n");
		syscall_monitor_write("usage memory: ");
		syscall_monitor_write_dec(count * 4);
		syscall_monitor_write(" Kbyte \n");
		syscall_monitor_write("free memory: ");
		syscall_monitor_write_dec((nframes - count) * 4);
		syscall_monitor_write(" Kbyte \n");
		
		HEAP * kheap = (HEAP *)syscall_get_kheap();
		syscall_monitor_write("kheap start: ");
		syscall_monitor_write_hex(kheap->start_address);
		syscall_monitor_write(" end: ");
		syscall_monitor_write_hex(kheap->end_address);
		syscall_monitor_write(" size: ");
		syscall_monitor_write_dec(kheap->end_address - kheap->start_address);
		syscall_monitor_write(" byte \n");
		syscall_monitor_write("kheap hole number: ");
		syscall_monitor_write_dec(kheap->index.size);
		syscall_monitor_put('\n');
	}
	else
	{
		syscall_monitor_write("usage: ps [-m]");
	}
	return 0;
}

