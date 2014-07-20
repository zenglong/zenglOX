// shell.c -- 命令行程式

#include "common.h"
#include "syscall.h"
#include "task.h"

#define MAX_INPUT 100

char input_for_up[MAX_INPUT]={0};
char input_for_down[MAX_INPUT]={0};

int replace_input(char * input, char * replace, int * count)
{
	int len = strlen(replace);
	for(;(*count) > 0;(*count) -= 1)
		syscall_monitor_write("\b \b");
	strcpy(input,replace);
	input[len] = '\0';
	(*count) = len;
	syscall_monitor_write(input);
	return 0;
}

int main(VOID * task, int argc, char * argv[])
{
	int ret;
	BOOL isinUp = FALSE;
	char input[MAX_INPUT]={0};
	int count = 0;
	TASK_MSG msg;

	UNUSED(argc);
	UNUSED(argv);

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
			if(msg.keyboard.type == MKT_KEY)
			{
				switch(msg.keyboard.key)
				{
				case MKK_CURSOR_UP_PRESS:
					if(strlen(input_for_up) == 0)
						break;
					strcpy(input_for_down, input);
					replace_input(input,input_for_up,&count);
					isinUp = TRUE;
					break;
				case MKK_CURSOR_DOWN_PRESS:
					if(isinUp == TRUE)
					{
						replace_input(input,input_for_down,&count);
						isinUp = FALSE;
					}
					break;
				default:
					break;
				}

				continue;
			}
			if(msg.keyboard.type != MKT_ASCII)
				continue;

			if(msg.keyboard.ascii == '\n')
			{
				strcpy(input_for_up, input);
				syscall_monitor_put('\n');
				if(count > 0)
				{
					if(strcmp(input,"exit") == 0)
						return 0;
					else if(syscall_execve(input) == 0)
						syscall_wait(task);
					input[0] = '\0';
					count = 0;
				}
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
				// input里目前只存放可显示字符，可显示字符的范围为十进制格式的32到126
				if(msg.keyboard.ascii >= 32 && msg.keyboard.ascii <= 126)
				{
					input[count++] = msg.keyboard.ascii;
					input[count]='\0';
					syscall_monitor_put((char)msg.keyboard.ascii);
				}
			}
		}
		else if(msg.type == MT_TASK_FINISH)
		{
			syscall_finish(msg.finish_task.exit_task);
		}
	}

	return -1;
}

