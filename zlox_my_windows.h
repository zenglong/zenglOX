// zlox_my_windows.h Defines some structures relating to the windows.

#ifndef _ZLOX_MY_WINDOWS_H_
#define _ZLOX_MY_WINDOWS_H_

#include "zlox_common.h"

typedef struct _ZLOX_MY_WINDOW ZLOX_MY_WINDOW;

#include "zlox_task.h"

typedef ZLOX_SINT32 (*ZLOX_MY_WINDOW_CALLBACK) (ZLOX_VOID * msg, ZLOX_VOID * my_window);

typedef struct _ZLOX_MY_RECT
{
	ZLOX_SINT32 x;
	ZLOX_SINT32 y;
	ZLOX_SINT32 width;
	ZLOX_SINT32 height;
} ZLOX_MY_RECT;

struct _ZLOX_MY_WINDOW
{
	ZLOX_SINT32 x;
	ZLOX_SINT32 y;
	ZLOX_SINT32 width;
	ZLOX_SINT32 height;
	ZLOX_UINT32 * bitmap;
	ZLOX_MY_WINDOW_CALLBACK mywin_callback;
	ZLOX_VOID * task;
	ZLOX_BOOL need_update;
	ZLOX_MY_RECT update_rect;
	ZLOX_MY_WINDOW * next;
	ZLOX_MY_WINDOW * prev;
	ZLOX_VOID * kbd_task;
	ZLOX_BOOL has_title;
	ZLOX_BOOL has_cmd;
	ZLOX_MY_RECT title_rect;
	ZLOX_MY_RECT close_btn_rect;
	ZLOX_MY_RECT cmd_rect;
	ZLOX_SINT32 cmd_cursor_x;
	ZLOX_SINT32 cmd_cursor_y;
	ZLOX_UINT32 cmd_font_color;
	ZLOX_UINT32 cmd_back_color;
	ZLOX_BOOL cmd_single_line_out;
};

ZLOX_SINT32 zlox_create_my_window(ZLOX_MY_WINDOW * my_window);

ZLOX_SINT32 zlox_dispatch_win_msg(ZLOX_TASK_MSG * msg, ZLOX_MY_WINDOW * my_window);

ZLOX_SINT32 zlox_update_for_mymouse(ZLOX_TASK_MSG * msg);

ZLOX_SINT32 zlox_update_for_mykbd(ZLOX_TASK_MSG * msg);

ZLOX_SINT32 zlox_init_my_mouse();

ZLOX_SINT32 zlox_set_cmd_window(ZLOX_TASK * task, ZLOX_MY_WINDOW * cmd_win, ZLOX_BOOL autofind);

ZLOX_SINT32 zlox_cmd_window_put(ZLOX_CHAR c);

ZLOX_SINT32 zlox_cmd_window_write(const ZLOX_CHAR * c);

ZLOX_SINT32 zlox_cmd_window_write_hex(ZLOX_UINT32 n);

ZLOX_SINT32 zlox_cmd_window_write_dec(ZLOX_UINT32 n);

ZLOX_SINT32 zlox_destroy_my_window(ZLOX_MY_WINDOW * my_window);

ZLOX_SINT32 zlox_cmd_window_del_line();

ZLOX_SINT32 zlox_cmd_window_insert_line();

ZLOX_SINT32 zlox_cmd_window_set_cursor(ZLOX_SINT32 x, ZLOX_SINT32 y);

ZLOX_SINT32 zlox_cmd_window_set_single(ZLOX_BOOL flag);

ZLOX_SINT32 zlox_cmd_window_clear();

#endif // _ZLOX_MY_WINDOWS_H_

