// shell.c -- 命令行程式

#include "common.h"
#include "syscall.h"
#include "task.h"

#define MAX_INPUT 100

int main(VOID * task, int argc, char * argv[])
{
	int ret;
	int count = 0;
	char input[MAX_INPUT]={0};
	TASK_MSG msg;

	UNUSED(argc);
	UNUSED(argv);

	/*syscall_monitor_write("zenglOX> ");
	for(int i=0;i < argc;i++)
	{
		syscall_monitor_write(argv[i]);
		syscall_monitor_put(' ');
	}
	syscall_monitor_put('\n');*/

	/*char * test = (char *)syscall_umalloc(50);
	strcpy(test,"hello world\n");
	syscall_monitor_write(test);
	syscall_ufree(test);*/

	syscall_monitor_write("zenglOX> ");
	syscall_set_input_focus(task);
	while(TRUE)
	{
		ret = syscall_get_tskmsg(task,&msg,TRUE);
		if(ret != 1)
		{
			syscall_idle_cpu();
			continue;
		}

		if(msg.type == MT_KEYBOARD)
		{
			if(msg.keyboard.type != MKT_ASCII)
				continue;

			if(msg.keyboard.ascii == '\n')
			{
				syscall_monitor_put('\n');
				if(strcmp(input,"exit") == 0)
					return 0;
				else if(syscall_execve(input) == 0)
					syscall_wait(task);
				count = 0;
				syscall_monitor_write("\nzenglOX> ");
				syscall_set_input_focus(task);
				continue;
			}
			else if(msg.keyboard.ascii == '\b')
			{
				if(count > 0)
				{
					syscall_monitor_write("\b \b");
					input[--count] = '\0';
				}
				continue;
			}
			else if(msg.keyboard.ascii == '\t')
				continue;

			if(count < (MAX_INPUT-1))
			{
				input[count++] = msg.keyboard.ascii;
				input[count]='\0';
				syscall_monitor_put((char)msg.keyboard.ascii);
			}
		}
		else if(msg.type == MT_TASK_FINISH)
		{
			syscall_finish(msg.finish_task.exit_task);
		}
	}

	return -1;
}

