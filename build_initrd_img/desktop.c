//desktop.c ---- GUI桌面相关的C源码

#include "common.h"
#include "syscall.h"
#include "my_windows.h"
#include "task.h"
#include "fs.h"

FLOAT mouse_scale = 1.0;

SINT32 desktop_win_callback (TASK_MSG * msg, MY_WINDOW * my_window)
{
	FLOAT tmp;
	char execve[10] = {'t','e','r','m'}; // execve "term" filename must be in stack !
	switch(msg->type)
	{
	case MT_CREATE_MY_WINDOW:
		for (UINT16 y = 0; y < 768; y++) {
			for (UINT16 x = 0; x < 1024; x++) {
				if(x >= 10 && x <= 50 && y >= 30 && y <= 70)
				{
					if(x == 10 || x == 50 || y == 30 || y == 70)
						my_window->bitmap[x + y * 1024] = 0xFF000000 | (0xff * 0x10000) | (0xff * 0x100) | 0xff;
					else
						my_window->bitmap[x + y * 1024] = 0xFF000000 | (0x0 * 0x10000) | (0x0 * 0x100) | 0x0;
				}
				else
					my_window->bitmap[x + y * 1024] = 0xFF000000 | (0x40 * 0x10000) | (0x8A * 0x100) | 0xE6;
			}
		}
		FS_NODE * fsnode = (FS_NODE *)syscall_umalloc(sizeof(FS_NODE));
		FS_NODE * fs_root = (FS_NODE *)syscall_get_fs_root();
		int ret = syscall_finddir_fs_safe(fs_root, "background.bmp", fsnode);
		if (ret != -1 && ((fsnode->flags & 0x7) == FS_FILE))
		{
			UINT8 * buf = (UINT8 *)syscall_umalloc(fsnode->length + 1);
			syscall_read_fs(fsnode,0,fsnode->length,buf);
			UINT16 * bmp = (UINT16 *)(buf + 54);
			int bmp_win_x = 414, bmp_win_y = 285 + 197;
			UINT16 tmp;
			UINT32 r, g, b;
			int i, j;
			for(j = 0;j < 197;j++, bmp_win_y--)
			{
				for(i = 0, bmp_win_x = 414; i < 195 ;i++, bmp_win_x++)
				{
					tmp = bmp[i + j * 196];
					r = (UINT32)(((tmp & 0x7C00) >> 10) << 3);
					g = (UINT32)(((tmp & 0x03E0) >> 5) << 3);
					b = (UINT32)(((tmp & 0x001F) >> 0) << 3);
					my_window->bitmap[bmp_win_x + bmp_win_y * 1024] = 
						0xFF000000 | (r * 0x10000) | (g * 0x100) | b;
				}
			}
			syscall_ufree(buf);
		}
		if(fsnode != NULL)
			syscall_ufree(fsnode);
		return 0;
		break;
	case MT_MOUSE:
		if(!(msg->mouse.rel_x >= 10 && msg->mouse.rel_x <= 50 && 
			msg->mouse.rel_y >= 30 && msg->mouse.rel_y <= 70))
		{
			for (UINT16 y = 30; y <= 70; y++) {
				for (UINT16 x = 10; x <= 50; x++) {
					if(x == 10 || x == 50 || y == 30 || y == 70)
						my_window->bitmap[x + y * 1024] = 0xFF000000 | (0xff * 0x10000) | (0xff * 0x100) | 0xff;
					else
						my_window->bitmap[x + y * 1024] = 0xFF000000 | (0x0 * 0x10000) | (0x0 * 0x100) | 0x0;
				}
			}
			my_window->need_update = TRUE;
			my_window->update_rect.x = 10;
			my_window->update_rect.y = 30;
			my_window->update_rect.width = 41;
			my_window->update_rect.height = 41;
			return 0;
		}
		if(msg->mouse.btn == MMB_LEFT_DOWN || msg->mouse.btn == MMB_LEFT_DRAG)
		{
			for (UINT16 y = 30; y <= 70; y++) {
				for (UINT16 x = 10; x <= 50; x++) {
					if(x == 10 || x == 50 || y == 30 || y == 70)
						my_window->bitmap[x + y * 1024] = 0xFF000000 | (0x0 * 0x10000) | (0x0 * 0x100) | 0x0;
					else
						my_window->bitmap[x + y * 1024] = 0xFF000000 | (0xff * 0x10000) | (0xff * 0x100) | 0xff;
				}
			}
			my_window->need_update = TRUE;
			my_window->update_rect.x = 10;
			my_window->update_rect.y = 30;
			my_window->update_rect.width = 41;
			my_window->update_rect.height = 41;
		}
		else if(msg->mouse.btn == MMB_LEFT_UP)
		{
			for (UINT16 y = 30; y <= 70; y++) {
				for (UINT16 x = 10; x <= 50; x++) {
					if(x == 10 || x == 50 || y == 30 || y == 70)
						my_window->bitmap[x + y * 1024] = 0xFF000000 | (0xff * 0x10000) | (0xff * 0x100) | 0xff;
					else
						my_window->bitmap[x + y * 1024] = 0xFF000000 | (0x0 * 0x10000) | (0x0 * 0x100) | 0x0;
				}
			}
			my_window->need_update = TRUE;
			my_window->update_rect.x = 10;
			my_window->update_rect.y = 30;
			my_window->update_rect.width = 41;
			my_window->update_rect.height = 41;
			syscall_execve(execve);
		}
		return 0;
		break;
	case MT_KEYBOARD:
		if(msg->keyboard.type != MKT_KEY)
			break;
		switch(msg->keyboard.key)
		{
		case MKK_F5_PRESS:
			my_window->need_update = TRUE;
			my_window->update_rect.x = 0;
			my_window->update_rect.y = 0;
			my_window->update_rect.width = 1024;
			my_window->update_rect.height = 768;
			return 0;
			break;
		case MKK_F2_PRESS:
			syscall_execve(execve);
			return 0;
			break;
		case MKK_CURSOR_UP_PRESS:
		case MKK_CURSOR_DOWN_PRESS:
			if(msg->keyboard.key == MKK_CURSOR_UP_PRESS)
				tmp = mouse_scale + 0.5;
			else
				tmp = mouse_scale - 0.5;
			if(tmp >= 1.0 && tmp <= 4.0)
			{
				mouse_scale = tmp;
				syscall_mouse_set_scale(&mouse_scale);
				syscall_monitor_write("set mouse scale to ");
				int tmp_int = tmp;
				syscall_monitor_write_dec(tmp_int);	
				if(tmp != (FLOAT)tmp_int)
					syscall_monitor_write(".5\n");
				else
					syscall_monitor_write(".0\n");
			}
			else
				syscall_monitor_write("mouse scale must in 1.0 to 4.0\n");
			return 0;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return -1;
}

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	TASK * tmp_task = ((TASK *)task)->prev;
	while(tmp_task != 0)
	{
		if(strcmp(tmp_task->args, "desktop") == 0)
		{
			syscall_cmd_window_write("the desktop is already running, do not start it again!");
			return 0;
		}
		tmp_task = tmp_task->prev;
	}

	MY_WINDOW desktop = {0};
	VOID * hdesktop = NULL;
	desktop.x = 0;
	desktop.y = 0;
	desktop.width = 1024;
	desktop.height = 768;
	desktop.mywin_callback = (MY_WINDOW_CALLBACK)desktop_win_callback;
	desktop.task = task;
	hdesktop = (VOID *)syscall_create_my_window(&desktop);
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
		case MT_MOUSE:
			syscall_dispatch_win_msg(&msg, hdesktop);
			break;
		case MT_KEYBOARD:
			if(msg.keyboard.type == MKT_KEY)
			{
				switch(msg.keyboard.key)
				{
				case MKK_F5_PRESS:
				case MKK_F2_PRESS:
				case MKK_CURSOR_UP_PRESS:
				case MKK_CURSOR_DOWN_PRESS:
					syscall_dispatch_win_msg(&msg, hdesktop);
					break;
				default:
					break;
				}
			}
			break;
		case MT_TASK_FINISH:
			syscall_finish(msg.finish_task.exit_task);
			break;
		default:
			break;
		}
	}
	return 0;
}

