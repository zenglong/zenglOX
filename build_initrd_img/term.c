//term.c ---- GUI终端相关的C源码

#include "common.h"
#include "syscall.h"
#include "my_windows.h"
#include "task.h"

MY_WINDOW term = {0};

SINT32 term_win_callback (TASK_MSG * msg, MY_WINDOW * my_window)
{
	MY_RECT * tr;
	switch(msg->type)
	{
	case MT_CREATE_MY_WINDOW:
		for (UINT16 y = 0; y < term.height; y++) {
			for (UINT16 x = 0; x < term.width; x++) {
				if(x == 0 || x == (term.width - 1) || y == 0 || y == (term.height - 1))
					my_window->bitmap[x + y * term.width] = 0xFF000000 | (0xff * 0x10000) | (0xff * 0x100) | 0xff;
				else
					my_window->bitmap[x + y * term.width] = 0xFF000000 | (0x0 * 0x10000) | (0x0 * 0x100) | 0x0;
			}
		}
		my_window->has_title = TRUE;
		my_window->title_rect.x = 0;
		my_window->title_rect.y = 0;
		my_window->title_rect.width = term.width - 20;
		my_window->title_rect.height = 20;
		tr = &my_window->title_rect;
		for (UINT16 y = tr->y; y < (tr->y + tr->height); y++) {
			for (UINT16 x = tr->x; x < (tr->x + tr->width); x++) {
				if(x == tr->x || x == (tr->x + tr->width - 1) || 
					y == tr->y || y == (tr->y + tr->height - 1))
					my_window->bitmap[x + y * term.width] = 0xFF000000 | (0xff * 0x10000) | (0xff * 0x100) | 0xff;
				else
					my_window->bitmap[x + y * term.width] = 0xFF000000 | (0x28 * 0x10000) | (0x31 * 0x100) | 0xE8;
			}
		}
		my_window->close_btn_rect.x = my_window->title_rect.width;
		my_window->close_btn_rect.y = 0;
		my_window->close_btn_rect.width = my_window->title_rect.height;
		my_window->close_btn_rect.height = my_window->title_rect.height;
		tr = &my_window->close_btn_rect;
		for (UINT16 y = tr->y; y < (tr->y + tr->height); y++) {
			for (UINT16 x = tr->x; x < (tr->x + tr->width); x++) {
				if(x == tr->x || x == (tr->x + tr->width - 1) || 
					y == tr->y || y == (tr->y + tr->height - 1))
					my_window->bitmap[x + y * term.width] = 0xFF000000 | (0xff * 0x10000) | (0xff * 0x100) | 0xff;
				else
					my_window->bitmap[x + y * term.width] = 0xFF000000 | (0xF7 * 0x10000) | (0x0B * 0x100) | 0x26;
			}
		}
		my_window->has_cmd = TRUE;
		my_window->cmd_rect.x = 2;
		my_window->cmd_rect.y = my_window->title_rect.height + 1;
		my_window->cmd_rect.width = term.width - 4;
		my_window->cmd_rect.height = term.height - my_window->title_rect.height - 3;
		my_window->cmd_font_color = 0xFFFFFFFF;
		my_window->cmd_back_color = 0xFF000000;
		syscall_set_cmd_window(my_window->task, NULL, TRUE);
		return 0;
		break;
	case MT_KEYBOARD:
		if(msg->keyboard.type != MKT_ASCII)
			break;
		//char key_press[2];
		//key_press[0] = (char)msg->keyboard.ascii;
		//key_press[1] = '\0';
		//syscall_monitor_write("task id: ");
		//TASK * tmp = (TASK *)my_window->kbd_task;
		//syscall_monitor_write_dec((UINT32)tmp->id);
		//syscall_cmd_window_write("input char : ");
		syscall_cmd_window_put((char)msg->keyboard.ascii);
		//syscall_cmd_window_write("\n");
		return 0;
		break;
	case MT_CLOSE_MY_WINDOW:
		//syscall_cmd_window_write("close window \n");
		syscall_monitor_write("close window task id: ");
		TASK * tmp = (TASK *)my_window->task;
		syscall_monitor_write_dec((UINT32)tmp->id);
		syscall_monitor_write("\n");
		return 0;
		break;
	default:
		break;
	}
	return -1;
}

char * startExeGl = "shell";

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	char startExe[10] = {0}; // execve filename must be in stack !
	strcpy(startExe, startExeGl);
	VOID * hterm = NULL;
	term.x = 110 + (((TASK *)task)->id * 30 % 500);
	term.y = 60 + (((TASK *)task)->id * 30 % 300);
	term.width = 724;
	term.height = 373;
	term.mywin_callback = (MY_WINDOW_CALLBACK)term_win_callback;
	term.task = task;
	hterm = (VOID *)syscall_create_my_window(&term);
	TASK_MSG msg;
	int ret;
	while(TRUE)
	{
		ret = syscall_get_tskmsg(task,&msg,TRUE);
		if(ret != 1)
		{
			syscall_idle_cpu();
			continue;
		}
		switch(msg.type)
		{
		case MT_CREATE_MY_WINDOW:
			syscall_dispatch_win_msg(&msg, hterm);
			syscall_execve(startExe);
			break;
		case MT_CLOSE_MY_WINDOW:
			syscall_finish_all_child(task);
			return 0;
			break;
		//case MT_KEYBOARD:
			//syscall_dispatch_win_msg(&msg, hterm);
			//break;
		case MT_TASK_FINISH:
			syscall_finish(msg.finish_task.exit_task);
			return msg.finish_task.exit_code;
			break;
		default:
			break;
		}
	}
	return 0;
}

