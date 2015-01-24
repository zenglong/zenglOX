// my_windows.h Defines some structures relating to the windows.

#ifndef _MY_WINDOWS_H_
#define _MY_WINDOWS_H_

#include "common.h"

typedef SINT32 (*MY_WINDOW_CALLBACK) (VOID * msg, VOID * my_window);

typedef struct _MY_WINDOW MY_WINDOW;

typedef struct _MY_RECT
{
	SINT32 x;
	SINT32 y;
	SINT32 width;
	SINT32 height;
} MY_RECT;

struct _MY_WINDOW
{
	SINT32 x;
	SINT32 y;
	SINT32 width;
	SINT32 height;
	UINT32 * bitmap;
	MY_WINDOW_CALLBACK mywin_callback;
	VOID * task;
	BOOL need_update;
	MY_RECT update_rect;
	MY_WINDOW * next;
	MY_WINDOW * prev;
	VOID * kbd_task;
	BOOL has_title;
	BOOL has_cmd;
	MY_RECT title_rect;
	MY_RECT close_btn_rect;
	MY_RECT cmd_rect;
	SINT32 cmd_cursor_x;
	SINT32 cmd_cursor_y;
	UINT32 cmd_font_color;
	UINT32 cmd_back_color;
	BOOL cmd_single_line_out;
};

#endif // _MY_WINDOWS_H_

