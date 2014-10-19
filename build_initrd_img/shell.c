// shell.c -- 命令行程式

#include "common.h"
#include "syscall.h"
#include "task.h"
#include "fs.h"

#define MAX_INPUT 120

char input_for_up[MAX_INPUT]={0};
char input_for_down[MAX_INPUT]={0};
char tmp_filenames[MAX_INPUT]={0};
FS_NODE tmp_fsnode;
FS_NODE * tmp_fs_root = NULL;

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

static SINT32 set_filename(CHAR ** filename)
{
	SINT32 i;
	for(i=0;(*filename)[i]!='\0';i++)
	{
		if((*filename)[i] != ' ')
			break;
	}
	(*filename) = (*filename) + i;
	CHAR * tmp_filename = (*filename);
	for(i=0;tmp_filename[i]!='\0';i++)
	{
		if(tmp_filename[i] == ' ')
		{
			tmp_filename[i] = '\0';
			return i;
		}
	}
	return -1;
}

static SINT32 restore_filename(CHAR * filename,SINT32 pos)
{
	if(pos > 0)
		filename[pos] = ' ';
	return 0;
}

BOOL detect_fix_filename(char * input)
{
	SINT32 len =strlen(input);
	BOOL need_add_bin = TRUE;
	char * tmp_filename = tmp_filenames;
	if(len<=0)
		return FALSE;
	strcpy(tmp_filename, input);
	SINT32 ret_pos = set_filename((CHAR **)&tmp_filename);
	len = strlen(tmp_filename);
	for(SINT32 i=0;i<len;i++)
	{
		if(tmp_filename[i]=='/')
			need_add_bin = FALSE;
	}
	if(tmp_fs_root == NULL)
		tmp_fs_root = (FS_NODE *)syscall_get_fs_root();
	SINT32 ret_int = (SINT32)syscall_finddir_fs_safe(tmp_fs_root, tmp_filename, &tmp_fsnode);
	if(ret_int == -1 && need_add_bin == TRUE)
	{
		char hd_bin[10] = "hd/bin/";
		restore_filename(tmp_filename, ret_pos);
		len = strlen(tmp_filename);
		SINT32 len_hd_bin = strlen(hd_bin);
		char * src = tmp_filename + len - 1;
		char * dst = src + len_hd_bin;
		reverse_memcpy((UINT8 *)dst, (UINT8 *)src, len);
		memcpy((UINT8 *)tmp_filename, (UINT8 *)hd_bin, len_hd_bin);
		tmp_filename[len + len_hd_bin] = '\0';
		ret_pos = set_filename((CHAR **)&tmp_filename);
		ret_int = (SINT32)syscall_finddir_fs_safe(tmp_fs_root, tmp_filename, &tmp_fsnode);
		if(ret_int == -1)
			return FALSE;
		restore_filename(tmp_filename, ret_pos);
		strcpy(input, tmp_filename);
	}
	return TRUE;
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

	BOOL ps2_init_status, ps2_first_port_status, ps2_sec_port_status;
	syscall_ps2_get_status(&ps2_init_status, &ps2_first_port_status, &ps2_sec_port_status);
	if(ps2_init_status == FALSE)
	{
		syscall_monitor_write("PS/2 Controller self test failed ! , you can't use keyboard and mouse..."
					" you may be have to reboot manual\n");
		while(TRUE)
		{
			syscall_idle_cpu();
		}
	}
	else if(ps2_first_port_status == FALSE)
	{
		syscall_monitor_write("first PS/2 device reset failed, you can't use keyboard..."
					" you may be have to reboot manual\n");
		while(TRUE)
		{
			syscall_idle_cpu();
		}
	}

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
					if(strcmp(input_for_up, input) == 0)
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
				syscall_monitor_put('\n');
				if(count > 0)
				{
					strcpy(input_for_up, input);
					if(strcmp(input,"exit") == 0)
						return 0;
					else if(detect_fix_filename(input) == FALSE)
					{
						char * input_tmp = input;
						set_filename((CHAR **)&input_tmp);
						syscall_monitor_write("the file \"");
						syscall_monitor_write(input);
						syscall_monitor_write("\" is not exists");
					}
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

			if(count < (MAX_INPUT-20))
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

