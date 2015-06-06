// Defines the functions for windows 

#include "zlox_my_windows.h"
#include "zlox_kheap.h"
#include "zlox_monitor.h"
#include "zlox_vga.h"
#include "zlox_isr.h"

#define ZLOX_CMD_WINDOW_W_SPACE 1
#define ZLOX_CMD_WINDOW_H_SPACE 2

//zlox_vga.c
extern ZLOX_UINT8 * lfb_vid_memory;
extern ZLOX_UINT16 lfb_resolution_x;
extern ZLOX_UINT16 lfb_resolution_y;
extern ZLOX_UINT16 lfb_resolution_b;
//zlox_term_font.h included in zlox_vga.c
extern ZLOX_UINT8 number_font[][12];
extern ZLOX_UINT8 term_cursor_font[][12];
//zlox_task.c
extern ZLOX_TASK * current_task;

ZLOX_MY_WINDOW * mywin_list_header = ZLOX_NULL;
ZLOX_MY_WINDOW * mywin_list_end = ZLOX_NULL;
ZLOX_MY_WINDOW * mywin_list_kbd_input = ZLOX_NULL;

ZLOX_SINT32 my_mouse_x = 0;
ZLOX_SINT32 my_mouse_y = 0;
ZLOX_SINT32 my_mouse_w = 0;
ZLOX_SINT32 my_mouse_h = 0;
ZLOX_BOOL my_mouse_left_press = ZLOX_FALSE;
ZLOX_SINT32 my_mouse_left_press_x = 0;
ZLOX_SINT32 my_mouse_left_press_y = 0;
ZLOX_MY_RECT my_drag_rect = {0};
ZLOX_MY_WINDOW * my_drag_win = ZLOX_NULL;
ZLOX_BOOL my_mouse_isInit = ZLOX_FALSE;

ZLOX_UINT8 mouse_bitmap_byte[] = {
	0xFE, 0xFE, 0xFE, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xCA, 0xCA, 0xCA, 0x00, 
	0xD6, 0xD6, 0xD6, 0x00, 0xF8, 0xF8, 0xF8, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xF6, 0xF6, 0xF6, 0x00, 
	0x3B, 0x3B, 0x3B, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x93, 0x93, 0x93, 0x00, 0xFC, 0xFC, 0xFC, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xBA, 0xBA, 0xBA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBE, 0xBE, 0xBE, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFD, 0xFD, 0xFD, 0x00, 0x83, 0x83, 0x83, 0x00, 0x13, 0x13, 0x13, 0x00, 0x3B, 0x3B, 0x3B, 0x00, 
	0xF5, 0xF5, 0xF5, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xDD, 0xDD, 0xDD, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xF5, 0xF5, 0xF5, 0x00, 0x42, 0x42, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x76, 0x76, 0x76, 0x00, 0xFD, 0xFD, 0xFD, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0x4D, 0x4D, 0x4D, 0x00, 0xD8, 0xD8, 0xD8, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xBA, 0xBA, 0xBA, 0x00, 0x11, 0x11, 0x11, 0x00, 
	0x01, 0x01, 0x01, 0x00, 0xEF, 0xEF, 0xEF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x51, 0x51, 0x51, 0x00, 
	0xBC, 0xBC, 0xBC, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x5B, 0x5B, 0x5B, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x53, 0x53, 0x53, 0x00, 0xFE, 0xFE, 0xFE, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x17, 0x17, 0x17, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x3B, 0x3B, 0x3B, 0x00, 0xF7, 0xF7, 0xF7, 0x00, 0xFC, 0xFC, 0xFC, 0x00, 
	0x27, 0x27, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB8, 0xB8, 0xB8, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5D, 0x5D, 0x5D, 0x00, 
	0x6F, 0x6F, 0x6F, 0x00, 0x06, 0x06, 0x06, 0x00, 0x3A, 0x3A, 0x3A, 0x00, 0xFE, 0xFE, 0xFE, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xF7, 0xF7, 0xF7, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x03, 0x03, 0x03, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x31, 0x31, 0x00, 
	0xA1, 0xA1, 0xA1, 0x00, 0x93, 0x93, 0x93, 0x00, 0x78, 0x78, 0x78, 0x00, 0x6C, 0x6C, 0x6C, 0x00, 
	0x66, 0x66, 0x66, 0x00, 0xC5, 0xC5, 0xC5, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x10, 0x10, 0x10, 0x00, 0x18, 0x18, 0x18, 0x00, 0x06, 0x06, 0x06, 0x00, 0x05, 0x05, 0x05, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x9C, 0x9C, 0x9C, 0x00, 0xF7, 0xF7, 0xF7, 0x00, 0x01, 0x01, 0x01, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x14, 0x14, 0x00, 0x01, 0x01, 0x01, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x6F, 0x6F, 0x6F, 0x00, 0xFA, 0xFA, 0xFA, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x0A, 0x0A, 0x00, 0x0D, 0x0D, 0x0D, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x57, 0x57, 0x57, 0x00, 0xFC, 0xFC, 0xFC, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0x02, 0x02, 0x02, 0x00, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x36, 0x36, 0x36, 0x00, 0xEC, 0xEC, 0xEC, 0x00, 0xFD, 0xFD, 0xFD, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x09, 0x09, 0x09, 0x00, 0x08, 0x08, 0x08, 0x00, 
	0x06, 0x06, 0x06, 0x00, 0x01, 0x01, 0x01, 0x00, 0x0D, 0x0D, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x2B, 0x2B, 0x2B, 0x00, 0xFA, 0xFA, 0xFA, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x02, 0x02, 0x02, 0x00, 
	0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x1B, 0x1B, 0x1B, 0x00, 0xD7, 0xD7, 0xD7, 0x00, 0xFD, 0xFD, 0xFD, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x8D, 0x8D, 0x8D, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0x85, 0x85, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x62, 0x62, 0x00, 0xFD, 0xFD, 0xFD, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x02, 0x02, 0x02, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x37, 0x37, 0x00, 0xFB, 0xFB, 0xFB, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0x07, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x21, 0x21, 0x00, 0xF4, 0xF4, 0xF4, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0xD7, 0xD7, 0xD7, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x13, 0x13, 0x13, 0x00, 0xC6, 0xC6, 0xC6, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xA4, 0xA4, 0xA4, 0x00, 
	0xFC, 0xFC, 0xFC, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00,
};

static ZLOX_SINT32 zlox_draw_my_window(ZLOX_MY_WINDOW * my_window, 
		ZLOX_BOOL * need_update_mouse,ZLOX_MY_RECT * mouse_update_rect);

static ZLOX_SINT32 zlox_draw_my_mouse(ZLOX_MY_RECT * mouse_rect);

static ZLOX_BOOL zlox_detect_intersect_rect(ZLOX_MY_RECT * rect1, ZLOX_MY_RECT * rect2, ZLOX_MY_RECT * d_rect);

static ZLOX_BOOL zlox_detect_point_at_rect(ZLOX_SINT32 x, ZLOX_SINT32 y, ZLOX_MY_RECT * rect);

static ZLOX_SINT32 zlox_draw_mywin_list(ZLOX_MY_WINDOW * start, ZLOX_MY_WINDOW * end, ZLOX_MY_RECT * rect);

static ZLOX_SINT32 zlox_mymouse_first_click(ZLOX_TASK_MSG * msg);

static ZLOX_SINT32 zlox_mymouse_drag(ZLOX_TASK_MSG * msg);

static ZLOX_SINT32 zlox_mymouse_release(ZLOX_TASK_MSG * msg);

static ZLOX_BOOL zlox_cmd_window_scroll(ZLOX_SINT32 w_space, ZLOX_SINT32 h_space);

static ZLOX_SINT32 zlox_cmd_window_move_cursor(ZLOX_SINT32 w_space, ZLOX_SINT32 h_space);

static ZLOX_SINT32 zlox_cmd_window_write_char(ZLOX_SINT32 x, ZLOX_SINT32 y, ZLOX_SINT32 val, ZLOX_UINT32 color, ZLOX_UINT32 backColour);

static ZLOX_SINT32 zlox_erase_drag_rect(ZLOX_MY_WINDOW * start, ZLOX_MY_WINDOW * end, ZLOX_MY_RECT * rect);

static ZLOX_SINT32 zlox_draw_drag_rect(ZLOX_MY_RECT * rect);

ZLOX_SINT32 zlox_create_my_window(ZLOX_MY_WINDOW * my_window)
{
	ZLOX_MY_WINDOW * mywin = (ZLOX_MY_WINDOW *)zlox_kmalloc(sizeof(ZLOX_MY_WINDOW));
	zlox_memset((ZLOX_UINT8 *)mywin, 0, sizeof(ZLOX_MY_WINDOW));
	ZLOX_SINT32 pix_byte = lfb_resolution_b / 8;
	mywin->x = my_window->x;
	mywin->y = my_window->y;
	mywin->width = my_window->width;
	mywin->height = my_window->height;
	mywin->bitmap = (ZLOX_UINT32 *)zlox_kmalloc(my_window->width * my_window->height * pix_byte);
	mywin->mywin_callback = my_window->mywin_callback;
	mywin->task = my_window->task;
	mywin->kbd_task = my_window->task;
	((ZLOX_TASK *)mywin->task)->mywin = mywin;
	if(mywin_list_header == ZLOX_NULL)
	{
		mywin_list_header = mywin;
		mywin_list_end = mywin;
	}
	else
	{
		ZLOX_MY_WINDOW * tmpwin = mywin_list_header;
		for(;;)
		{
			if(tmpwin->next == ZLOX_NULL)
			{
				tmpwin->next = mywin;
				mywin->prev = tmpwin;
				break;
			}
			else
			{
				tmpwin = tmpwin->next;
			}
		}
		mywin_list_end = mywin;
	}
	ZLOX_TASK_MSG msg = {0};
	msg.type = ZLOX_MT_CREATE_MY_WINDOW;
	zlox_send_tskmsg((ZLOX_TASK *)mywin->task,&msg);
	return (ZLOX_SINT32)mywin;
}

ZLOX_SINT32 zlox_dispatch_win_msg(ZLOX_TASK_MSG * msg, ZLOX_MY_WINDOW * my_window)
{
	switch(msg->type)
	{
	case ZLOX_MT_CREATE_MY_WINDOW:
		{
			ZLOX_BOOL need_update_mouse;
			ZLOX_MY_RECT mouse_update_rect;
			my_window->mywin_callback(msg, my_window);
			zlox_draw_my_window(my_window, &need_update_mouse, &mouse_update_rect);
			if(need_update_mouse == ZLOX_TRUE)
				zlox_draw_my_mouse(&mouse_update_rect);
			mywin_list_kbd_input = my_window;
			return 0;
		}
		break;
	case ZLOX_MT_CLOSE_MY_WINDOW:
	case ZLOX_MT_MOUSE:
	case ZLOX_MT_KEYBOARD:
		my_window->mywin_callback(msg, my_window);
		if(my_window->need_update)
		{
			ZLOX_BOOL need_update_mouse;
			ZLOX_MY_RECT mouse_update_rect;
			zlox_draw_my_window(my_window, &need_update_mouse, &mouse_update_rect);
			ZLOX_MY_RECT mytmp_rect;
			mytmp_rect.x = my_window->x + my_window->update_rect.x;
			mytmp_rect.y = my_window->y + my_window->update_rect.y;
			mytmp_rect.width = my_window->update_rect.width;
			mytmp_rect.height = my_window->update_rect.height;
			my_window->need_update = ZLOX_FALSE;
			zlox_memset((ZLOX_UINT8 *)&my_window->update_rect, 0, sizeof(ZLOX_MY_RECT));
			ZLOX_MY_WINDOW * tmpwin = my_window->next;
			for(;tmpwin != ZLOX_NULL;)
			{
				ZLOX_MY_RECT tmp_rect;
				tmp_rect.x = tmpwin->x;
				tmp_rect.y = tmpwin->y;
				tmp_rect.width = tmpwin->width;
				tmp_rect.height = tmpwin->height;
				ZLOX_MY_RECT d_rect;
				if(zlox_detect_intersect_rect(&mytmp_rect, 
					&tmp_rect, &d_rect) == ZLOX_TRUE)
				{
					tmpwin->need_update = ZLOX_TRUE;
					tmpwin->update_rect.x = d_rect.x - tmpwin->x;
					tmpwin->update_rect.y = d_rect.y - tmpwin->y;
					tmpwin->update_rect.width = d_rect.width;
					tmpwin->update_rect.height = d_rect.height;
					zlox_draw_my_window(tmpwin, ZLOX_NULL, ZLOX_NULL);
					tmpwin->need_update = ZLOX_FALSE;
					zlox_memset((ZLOX_UINT8 *)&tmpwin->update_rect, 0, sizeof(ZLOX_MY_RECT));
				}
				tmpwin = tmpwin->next;
			}
			if(need_update_mouse == ZLOX_TRUE)
				zlox_draw_my_mouse(&mouse_update_rect);
		}
		return 0;
		break;
	default:
		break;
	}
	return -1;
}

ZLOX_SINT32 zlox_update_for_mymouse(ZLOX_TASK_MSG * msg)
{
	ZLOX_MY_RECT mouse_tmp_rect;
	mouse_tmp_rect.x = my_mouse_x;
	mouse_tmp_rect.y = my_mouse_y;
	mouse_tmp_rect.width = my_mouse_w;
	mouse_tmp_rect.height = my_mouse_h;
	ZLOX_BOOL is_release_mouse = (!(msg->mouse.state & 0x01) && (my_mouse_left_press == ZLOX_TRUE));
	//if(!is_release_mouse)
	//{
		zlox_draw_mywin_list(mywin_list_header, mywin_list_end, &mouse_tmp_rect);
	//}

	my_mouse_x += msg->mouse.rel_x;
	my_mouse_y -= msg->mouse.rel_y;
	if(my_mouse_x < 0)
		my_mouse_x = 0;
	else if(my_mouse_x >= lfb_resolution_x - 3)
		my_mouse_x = lfb_resolution_x - 3;
	if(my_mouse_y < 0)
		my_mouse_y = 0;
	else if(my_mouse_y >= lfb_resolution_y - 3)
		my_mouse_y = lfb_resolution_y - 3;

	if(msg->mouse.state & 0x01)
	{
		if(my_mouse_left_press == ZLOX_FALSE) // first click
		{
			zlox_mymouse_first_click(msg);
		}
		else // drag mouse
		{
			zlox_mymouse_drag(msg);
		}
	}
	else if(is_release_mouse)
	{
		zlox_mymouse_release(msg);
	}
	//if(!is_release_mouse)
	//{
		mouse_tmp_rect.x = my_mouse_x;
		mouse_tmp_rect.y = my_mouse_y;
		mouse_tmp_rect.width = my_mouse_w;
		mouse_tmp_rect.height = my_mouse_h;
		zlox_draw_my_mouse(&mouse_tmp_rect);
	//}
	return 0;
}

ZLOX_SINT32 zlox_update_for_mykbd(ZLOX_TASK_MSG * msg)
{
	zlox_send_tskmsg((ZLOX_TASK *)mywin_list_kbd_input->kbd_task,msg);
	return 0;
}

ZLOX_SINT32 zlox_init_my_mouse()
{
	if(my_mouse_isInit == ZLOX_FALSE)
	{
		my_mouse_x = (ZLOX_SINT32)lfb_resolution_x / 3;
		my_mouse_y = (ZLOX_SINT32)lfb_resolution_y / 3;
		my_mouse_w = 13;
		my_mouse_h = 24;
		my_mouse_isInit = ZLOX_TRUE;
		return 0;
	}
	return -1;
}

ZLOX_SINT32 zlox_set_cmd_window(ZLOX_TASK * task, ZLOX_MY_WINDOW * cmd_win, ZLOX_BOOL autofind)
{
	if(cmd_win != ZLOX_NULL &&
		cmd_win->has_cmd == ZLOX_TRUE)
	{
		task->cmd_win = cmd_win;
		return (ZLOX_SINT32)cmd_win;
	}
	if(autofind == ZLOX_FALSE)
		return 0;
	if(task->cmd_win != ZLOX_NULL && task->cmd_win->has_cmd == ZLOX_TRUE)
		return (ZLOX_SINT32)task->cmd_win;
	if(task->mywin != ZLOX_NULL && task->mywin->has_cmd == ZLOX_TRUE)
	{
		task->cmd_win = task->mywin;
		return (ZLOX_SINT32)task->cmd_win;
	}
	ZLOX_TASK * parent = task->parent;
	while(parent != ZLOX_NULL)
	{
		if(parent->cmd_win != ZLOX_NULL && parent->cmd_win->has_cmd == ZLOX_TRUE)
		{
			task->cmd_win = parent->cmd_win;
			return (ZLOX_SINT32)task->cmd_win;
		}
		if(parent->mywin != ZLOX_NULL && parent->mywin->has_cmd == ZLOX_TRUE)
		{
			task->cmd_win = parent->mywin;
			return (ZLOX_SINT32)task->cmd_win;
		}
		parent = parent->parent;
	}
	return 0;
}

ZLOX_SINT32 zlox_cmd_window_put(ZLOX_CHAR c)
{
	ZLOX_MY_WINDOW * cmdwin = current_task->cmd_win;
	if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
	{
		zlox_set_cmd_window(current_task, ZLOX_NULL, ZLOX_TRUE);
		cmdwin = current_task->cmd_win;
		if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
			return -1;
	}
	ZLOX_SINT32 start_x = cmdwin->cmd_rect.x;
	ZLOX_SINT32 start_y = cmdwin->cmd_rect.y;
	ZLOX_SINT32 w_space = ZLOX_CMD_WINDOW_W_SPACE;
	ZLOX_SINT32 h_space = ZLOX_CMD_WINDOW_H_SPACE;
	ZLOX_UINT32 color = cmdwin->cmd_font_color;
	ZLOX_UINT32 backColour = cmdwin->cmd_back_color;
	ZLOX_SINT32 val = (ZLOX_SINT32)c;
	ZLOX_MY_RECT mouse_tmp_rect;
	ZLOX_MY_RECT intersect_rect;
	ZLOX_MY_RECT rect;
	if(val < 0)
		val = 0;

	ZLOX_SINT32 w_charnum = cmdwin->cmd_rect.width / (ZLOX_VGA_CHAR_WIDTH + w_space);
	//ZLOX_SINT32 h_charnum = cmdwin->cmd_rect.height / (ZLOX_VGA_CHAR_HEIGHT + h_space);
	ZLOX_SINT32 x = -1, y = -1;
	mouse_tmp_rect.x = my_mouse_x;
	mouse_tmp_rect.y = my_mouse_y;
	mouse_tmp_rect.width = my_mouse_w;
	mouse_tmp_rect.height = my_mouse_h;
	if(!cmdwin->cmd_single_line_out)
	{
		zlox_cmd_window_move_cursor(w_space, h_space);
		rect.x = start_x + cmdwin->cmd_cursor_x * (ZLOX_VGA_CHAR_WIDTH + w_space) + cmdwin->x;
		rect.y = start_y + cmdwin->cmd_cursor_y * (ZLOX_VGA_CHAR_HEIGHT + h_space) + cmdwin->y;
		rect.width = ZLOX_VGA_CHAR_WIDTH;
		rect.height = ZLOX_VGA_CHAR_HEIGHT;
		zlox_draw_mywin_list(cmdwin, mywin_list_end, &rect);
		if(zlox_detect_intersect_rect(&rect, &mouse_tmp_rect, &intersect_rect) == ZLOX_TRUE)
		{
			zlox_draw_my_mouse(&intersect_rect);
		}
	}

	// Handle a backspace, by moving the cursor back one space
	if (c == 0x08)
	{
		if(cmdwin->cmd_cursor_x)
			cmdwin->cmd_cursor_x--;
		else if(cmdwin->cmd_cursor_x == 0)
		{
			if(cmdwin->cmd_cursor_y != 0)
			{
				cmdwin->cmd_cursor_y = (cmdwin->cmd_cursor_y - 1);
				cmdwin->cmd_cursor_x = w_charnum - 1;
			}
		}
	}
	// Handle a tab by increasing the cursor's X, but only to a point
	// where it is divisible by 8.
	else if (c == 0x09)
	{
		cmdwin->cmd_cursor_x = (cmdwin->cmd_cursor_x + 8) & ~(8-1);
	}
	// Handle carriage return
	else if (c == '\r')
	{
		cmdwin->cmd_cursor_x = 0;
	}
	// Handle newline by moving cursor back to left and increasing the row
	else if (c == '\n')
	{
		cmdwin->cmd_cursor_x = 0;
		cmdwin->cmd_cursor_y++;
	}
	// Handle any other printable character.
	else if(c >= ' ')
	{
		x = start_x + cmdwin->cmd_cursor_x * (ZLOX_VGA_CHAR_WIDTH + w_space);
		y = start_y + cmdwin->cmd_cursor_y * (ZLOX_VGA_CHAR_HEIGHT + h_space);
		zlox_cmd_window_write_char(x, y, val, color, backColour);
		cmdwin->cmd_cursor_x++;
	}

	// Check if we need to insert a new line because we have reached the end
	// of the screen.
	if (cmdwin->cmd_cursor_x >= w_charnum)
	{
		cmdwin->cmd_cursor_x = 0;
		cmdwin->cmd_cursor_y ++;
	}

	ZLOX_BOOL is_scroll = ZLOX_FALSE;

	if(!cmdwin->cmd_single_line_out)
	{
		// Scroll the screen if needed.
		is_scroll = zlox_cmd_window_scroll(w_space, h_space);
		// Move the hardware cursor.
		zlox_cmd_window_move_cursor(w_space, h_space);
	}
	if(is_scroll == ZLOX_FALSE)
	{
		if(x != -1 && y != -1)
		{
			rect.x = x + cmdwin->x;
			rect.y = y + cmdwin->y;
			rect.width = ZLOX_VGA_CHAR_WIDTH;
			rect.height = ZLOX_VGA_CHAR_HEIGHT;
			zlox_draw_mywin_list(cmdwin, mywin_list_end, &rect);
			if(zlox_detect_intersect_rect(&rect, &mouse_tmp_rect, &intersect_rect) == ZLOX_TRUE)
			{
				zlox_draw_my_mouse(&intersect_rect);
			}
		}
		if(!cmdwin->cmd_single_line_out)
		{
			rect.x = start_x + cmdwin->cmd_cursor_x * (ZLOX_VGA_CHAR_WIDTH + w_space) + cmdwin->x;
			rect.y = start_y + cmdwin->cmd_cursor_y * (ZLOX_VGA_CHAR_HEIGHT + h_space) + cmdwin->y;
			zlox_draw_mywin_list(cmdwin, mywin_list_end, &rect);
			if(zlox_detect_intersect_rect(&rect, &mouse_tmp_rect, &intersect_rect) == ZLOX_TRUE)
			{
				zlox_draw_my_mouse(&intersect_rect);
			}
		}
	}
	else
	{
		rect.x = cmdwin->x + cmdwin->cmd_rect.x;
		rect.y = cmdwin->y + cmdwin->cmd_rect.y;
		rect.width = cmdwin->cmd_rect.width;
		rect.height = cmdwin->cmd_rect.height;
		zlox_draw_mywin_list(cmdwin, mywin_list_end, &rect);
		if(zlox_detect_intersect_rect(&rect, &mouse_tmp_rect, &intersect_rect) == ZLOX_TRUE)
		{
			zlox_draw_my_mouse(&intersect_rect);
		}
	}

	zlox_isr_detect_proc_irq();

	return val;
}

ZLOX_SINT32 zlox_cmd_window_write(const ZLOX_CHAR * c)
{
	ZLOX_MY_WINDOW * cmdwin = current_task->cmd_win;
	if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
	{
		zlox_set_cmd_window(current_task, ZLOX_NULL, ZLOX_TRUE);
		cmdwin = current_task->cmd_win;
		if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
			return -1;
	}
	ZLOX_SINT32 i = 0;
	ZLOX_SINT32 w_space = ZLOX_CMD_WINDOW_W_SPACE;
	ZLOX_SINT32 h_space = ZLOX_CMD_WINDOW_H_SPACE;
	if(!cmdwin->cmd_single_line_out)
	{
		while (c[i])
		{
			zlox_cmd_window_put(c[i++]);
		}
	}
	else
	{
		zlox_cmd_window_move_cursor(w_space, h_space);
		ZLOX_UINT8 orig_x = cmdwin->cmd_cursor_x;
		ZLOX_UINT8 orig_y = cmdwin->cmd_cursor_y;
		while(c[i] && cmdwin->cmd_cursor_y == orig_y)
		{
			zlox_cmd_window_put(c[i++]);
		}
		// 将行尾清空为空格
		if(cmdwin->cmd_cursor_y == orig_y)
		{
			ZLOX_CHAR tmp_c = ' ';
			while(cmdwin->cmd_cursor_y == orig_y)
			{
				zlox_cmd_window_put(tmp_c);
			}
		}
		cmdwin->cmd_cursor_x = orig_x;
		cmdwin->cmd_cursor_y = orig_y;
		zlox_cmd_window_move_cursor(w_space, h_space);
		ZLOX_MY_RECT mouse_tmp_rect;
		ZLOX_MY_RECT intersect_rect;
		ZLOX_MY_RECT rect;
		rect.x = cmdwin->cmd_rect.x + cmdwin->cmd_cursor_x * (ZLOX_VGA_CHAR_WIDTH + w_space) + cmdwin->x;
		rect.y = cmdwin->cmd_rect.y + cmdwin->cmd_cursor_y * (ZLOX_VGA_CHAR_HEIGHT + h_space) + cmdwin->y;
		rect.width = ZLOX_VGA_CHAR_WIDTH;
		rect.height = ZLOX_VGA_CHAR_HEIGHT;
		zlox_draw_mywin_list(cmdwin, mywin_list_end, &rect);
		mouse_tmp_rect.x = my_mouse_x;
		mouse_tmp_rect.y = my_mouse_y;
		mouse_tmp_rect.width = my_mouse_w;
		mouse_tmp_rect.height = my_mouse_h;
		if(zlox_detect_intersect_rect(&rect, &mouse_tmp_rect, &intersect_rect) == ZLOX_TRUE)
		{
			zlox_draw_my_mouse(&intersect_rect);
		}
	}
	return 0;
}

// Outputs an integer as Hex
ZLOX_SINT32 zlox_cmd_window_write_hex(ZLOX_UINT32 n)
{
	ZLOX_MY_WINDOW * cmdwin = current_task->cmd_win;
	if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
	{
		zlox_set_cmd_window(current_task, ZLOX_NULL, ZLOX_TRUE);
		cmdwin = current_task->cmd_win;
		if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
			return -1;
	}
	ZLOX_SINT32 tmp;
	ZLOX_SINT32 i;
	ZLOX_CHAR noZeroes;

	zlox_cmd_window_write("0x");

	noZeroes = 1;

	for (i = 28; i > 0; i -= 4)
	{
		tmp = (n >> i) & 0xF;
		if (tmp == 0 && noZeroes != 0)
		{
			continue;
		}
	
		if (tmp >= 0xA)
		{
			noZeroes = 0;
			zlox_cmd_window_put (tmp-0xA+'a' );
		}
		else
		{
			noZeroes = 0;
			zlox_cmd_window_put( tmp+'0' );
		}
	}
  
	tmp = n & 0xF;
	if (tmp >= 0xA)
	{
		zlox_cmd_window_put(tmp-0xA+'a');
	}
	else
	{
		zlox_cmd_window_put(tmp+'0');
	}
	return 0;
}

// Outputs an integer to the cmd window.
ZLOX_SINT32 zlox_cmd_window_write_dec(ZLOX_UINT32 n)
{
	ZLOX_MY_WINDOW * cmdwin = current_task->cmd_win;
	if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
	{
		zlox_set_cmd_window(current_task, ZLOX_NULL, ZLOX_TRUE);
		cmdwin = current_task->cmd_win;
		if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
			return -1;
	}
	ZLOX_SINT32 acc;
	ZLOX_CHAR c[32];
	ZLOX_SINT32 i;
	ZLOX_CHAR c2[32];
	ZLOX_SINT32 j;

	if (n == 0)
	{
		zlox_cmd_window_put('0');
		return 0;
	}
	
	acc = n;
	i = 0;	
	while (acc > 0)
	{
		c[i] = '0' + acc%10;
		acc /= 10;
		i++;
	}
	c[i] = 0;

	c2[i--] = 0;
	j = 0;
	// reverse the integer string's order
	while(i >= 0)
	{
		c2[i--] = c[j++];
	}
	zlox_cmd_window_write(c2);
	return 0;
}

ZLOX_SINT32 zlox_destroy_my_window(ZLOX_MY_WINDOW * my_window)
{
	if(my_window == ZLOX_NULL)
		return -1;
	if(my_window->prev != ZLOX_NULL)
		my_window->prev->next = my_window->next;
	else // the first window
	{
		if(my_window->next != ZLOX_NULL)
			my_window->next->prev = ZLOX_NULL;
		mywin_list_header = 	my_window->next;
	}
	if(my_window->next != ZLOX_NULL)
		my_window->next->prev = my_window->prev;
	else // the end window
	{
		if(my_window->prev != ZLOX_NULL)
			my_window->prev->next = ZLOX_NULL;
		mywin_list_end = my_window->prev;
	}
	if(my_window == mywin_list_kbd_input)
		mywin_list_kbd_input = mywin_list_end;
	ZLOX_MY_RECT rect;
	rect.x = my_window->x;
	rect.y = my_window->y;
	rect.width = my_window->width;
	rect.height = my_window->height;
	zlox_draw_mywin_list(mywin_list_header, mywin_list_end, &rect);
	ZLOX_MY_RECT mouse_tmp_rect, intersect_rect;
	mouse_tmp_rect.x = my_mouse_x;
	mouse_tmp_rect.y = my_mouse_y;
	mouse_tmp_rect.width = my_mouse_w;
	mouse_tmp_rect.height = my_mouse_h;
	if(zlox_detect_intersect_rect(&rect, &mouse_tmp_rect, &intersect_rect) == ZLOX_TRUE)
	{
		zlox_draw_my_mouse(&intersect_rect);
	}
	if(my_window->bitmap != ZLOX_NULL)
		zlox_kfree(my_window->bitmap);
	zlox_memset((ZLOX_UINT8 *)my_window, 0, sizeof(ZLOX_MY_WINDOW));
	zlox_kfree(my_window);
	return 0;
}

ZLOX_SINT32 zlox_cmd_window_del_line()
{
	ZLOX_MY_WINDOW * cmdwin = current_task->cmd_win;
	if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
	{
		zlox_set_cmd_window(current_task, ZLOX_NULL, ZLOX_TRUE);
		cmdwin = current_task->cmd_win;
		if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
			return -1;
	}
	ZLOX_SINT32 w_space = ZLOX_CMD_WINDOW_W_SPACE;
	ZLOX_SINT32 h_space = ZLOX_CMD_WINDOW_H_SPACE;
	ZLOX_SINT32 h_total_perchar = ZLOX_VGA_CHAR_HEIGHT + h_space;
	ZLOX_SINT32 w_total_perchar = ZLOX_VGA_CHAR_WIDTH + w_space;
	ZLOX_SINT32 w_charnum = cmdwin->cmd_rect.width / w_total_perchar;
	ZLOX_SINT32 h_charnum = cmdwin->cmd_rect.height / h_total_perchar;
	if(cmdwin->cmd_cursor_y >= 0 && cmdwin->cmd_cursor_y <= (h_charnum - 1))
	{
		ZLOX_SINT32 h_cpy_num = (h_charnum - 1 - cmdwin->cmd_cursor_y) * h_total_perchar;
		ZLOX_SINT32 w_cpy_pix = w_charnum * w_total_perchar;
		ZLOX_SINT32 w_cpy_byte = w_cpy_pix * 4;
		ZLOX_UINT8 * dst, * src;
		for(ZLOX_SINT32 i=0; i < h_cpy_num;i++)
		{
			dst = (ZLOX_UINT8 *)&cmdwin->bitmap[cmdwin->cmd_rect.x + 
							(cmdwin->cmd_rect.y + cmdwin->cmd_cursor_y * h_total_perchar + i) *
							cmdwin->width];
			src = (ZLOX_UINT8 *)&cmdwin->bitmap[cmdwin->cmd_rect.x + 
						(cmdwin->cmd_rect.y + (cmdwin->cmd_cursor_y + 1) * h_total_perchar + i) *
						 cmdwin->width];
			zlox_memcpy(dst, src, w_cpy_byte);
		}
		h_cpy_num = (h_charnum - 1) * h_total_perchar;
		for(ZLOX_SINT32 i=0; i < h_total_perchar;i++)
		{
			for(ZLOX_SINT32 j=0; j < w_cpy_pix;j++)
				cmdwin->bitmap[cmdwin->cmd_rect.x + j + (cmdwin->cmd_rect.y + h_cpy_num + i) * cmdwin->width]
					= cmdwin->cmd_back_color;
		}
		zlox_cmd_window_move_cursor(w_space, h_space);
		ZLOX_MY_RECT rect, mouse_tmp_rect, intersect_rect;
		rect.x = cmdwin->x + cmdwin->cmd_rect.x;
		rect.y = cmdwin->y + cmdwin->cmd_rect.y + cmdwin->cmd_cursor_y * h_total_perchar;
		rect.width = w_cpy_pix;
		rect.height = (h_charnum - cmdwin->cmd_cursor_y) * h_total_perchar;
		zlox_draw_mywin_list(cmdwin, mywin_list_end, &rect);
		mouse_tmp_rect.x = my_mouse_x;
		mouse_tmp_rect.y = my_mouse_y;
		mouse_tmp_rect.width = my_mouse_w;
		mouse_tmp_rect.height = my_mouse_h;
		if(zlox_detect_intersect_rect(&rect, &mouse_tmp_rect, &intersect_rect) == ZLOX_TRUE)
		{
			zlox_draw_my_mouse(&intersect_rect);
		}
		return 0;
	}
	return -1;
}

ZLOX_SINT32 zlox_cmd_window_insert_line()
{
	ZLOX_MY_WINDOW * cmdwin = current_task->cmd_win;
	if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
	{
		zlox_set_cmd_window(current_task, ZLOX_NULL, ZLOX_TRUE);
		cmdwin = current_task->cmd_win;
		if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
			return -1;
	}
	ZLOX_SINT32 w_space = ZLOX_CMD_WINDOW_W_SPACE;
	ZLOX_SINT32 h_space = ZLOX_CMD_WINDOW_H_SPACE;
	ZLOX_SINT32 h_total_perchar = ZLOX_VGA_CHAR_HEIGHT + h_space;
	ZLOX_SINT32 w_total_perchar = ZLOX_VGA_CHAR_WIDTH + w_space;
	ZLOX_SINT32 w_charnum = cmdwin->cmd_rect.width / w_total_perchar;
	ZLOX_SINT32 h_charnum = cmdwin->cmd_rect.height / h_total_perchar;
	if(cmdwin->cmd_cursor_y >= 0 && cmdwin->cmd_cursor_y <= (h_charnum - 1))
	{
		ZLOX_SINT32 h_cpy_num = (h_charnum - 1 - cmdwin->cmd_cursor_y) * h_total_perchar;
		ZLOX_SINT32 w_cpy_pix = w_charnum * w_total_perchar;
		ZLOX_SINT32 w_cpy_byte = w_cpy_pix * 4;
		ZLOX_UINT8 * dst, * src;
		ZLOX_SINT32 src_h = (h_charnum - 1) * h_total_perchar - 1;
		ZLOX_SINT32 dst_h = h_charnum * h_total_perchar - 1;
		zlox_cmd_window_move_cursor(w_space, h_space);
		for(ZLOX_SINT32 i=0; i < h_cpy_num;i++)
		{
			dst = (ZLOX_UINT8 *)&cmdwin->bitmap[cmdwin->cmd_rect.x + 
							(cmdwin->cmd_rect.y + dst_h - i) *
							cmdwin->width];
			src = (ZLOX_UINT8 *)&cmdwin->bitmap[cmdwin->cmd_rect.x + 
						(cmdwin->cmd_rect.y + src_h - i) *
						 cmdwin->width];
			zlox_memcpy(dst, src, w_cpy_byte);
		}
		h_cpy_num = cmdwin->cmd_cursor_y * h_total_perchar;
		for(ZLOX_SINT32 i=0; i < h_total_perchar;i++)
		{
			for(ZLOX_SINT32 j=0; j < w_cpy_pix;j++)
				cmdwin->bitmap[cmdwin->cmd_rect.x + j + (cmdwin->cmd_rect.y + h_cpy_num + i) * cmdwin->width]
					= cmdwin->cmd_back_color;
		}
		zlox_cmd_window_move_cursor(w_space, h_space);
		ZLOX_MY_RECT rect, mouse_tmp_rect, intersect_rect;
		rect.x = cmdwin->x + cmdwin->cmd_rect.x;
		rect.y = cmdwin->y + cmdwin->cmd_rect.y + cmdwin->cmd_cursor_y * h_total_perchar;
		rect.width = w_cpy_pix;
		rect.height = (h_charnum - cmdwin->cmd_cursor_y) * h_total_perchar;
		zlox_draw_mywin_list(cmdwin, mywin_list_end, &rect);
		mouse_tmp_rect.x = my_mouse_x;
		mouse_tmp_rect.y = my_mouse_y;
		mouse_tmp_rect.width = my_mouse_w;
		mouse_tmp_rect.height = my_mouse_h;
		if(zlox_detect_intersect_rect(&rect, &mouse_tmp_rect, &intersect_rect) == ZLOX_TRUE)
		{
			zlox_draw_my_mouse(&intersect_rect);
		}
		return 0;
	}
	return -1;
}

ZLOX_SINT32 zlox_cmd_window_set_cursor(ZLOX_SINT32 x, ZLOX_SINT32 y)
{
	ZLOX_MY_WINDOW * cmdwin = current_task->cmd_win;
	if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
	{
		zlox_set_cmd_window(current_task, ZLOX_NULL, ZLOX_TRUE);
		cmdwin = current_task->cmd_win;
		if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
			return -1;
	}
	ZLOX_SINT32 w_space = ZLOX_CMD_WINDOW_W_SPACE;
	ZLOX_SINT32 h_space = ZLOX_CMD_WINDOW_H_SPACE;
	ZLOX_SINT32 h_total_perchar = ZLOX_VGA_CHAR_HEIGHT + h_space;
	ZLOX_SINT32 w_total_perchar = ZLOX_VGA_CHAR_WIDTH + w_space;
	ZLOX_SINT32 w_charnum = cmdwin->cmd_rect.width / w_total_perchar;
	ZLOX_SINT32 h_charnum = cmdwin->cmd_rect.height / h_total_perchar;
	if(x < 0 || x > (w_charnum - 1))
		return -1;
	if(y < 0 || y > (h_charnum - 1))
		return -1;
	zlox_cmd_window_move_cursor(w_space, h_space);
	ZLOX_MY_RECT rect, mouse_tmp_rect, intersect_rect;
	rect.x = cmdwin->x + cmdwin->cmd_rect.x + cmdwin->cmd_cursor_x * (ZLOX_VGA_CHAR_WIDTH + w_space);
	rect.y = cmdwin->y + cmdwin->cmd_rect.y + cmdwin->cmd_cursor_y * (ZLOX_VGA_CHAR_HEIGHT + h_space);
	rect.width = ZLOX_VGA_CHAR_WIDTH;
	rect.height = ZLOX_VGA_CHAR_HEIGHT;
	zlox_draw_mywin_list(cmdwin, mywin_list_end, &rect);
	mouse_tmp_rect.x = my_mouse_x;
	mouse_tmp_rect.y = my_mouse_y;
	mouse_tmp_rect.width = my_mouse_w;
	mouse_tmp_rect.height = my_mouse_h;
	if(zlox_detect_intersect_rect(&rect, &mouse_tmp_rect, &intersect_rect) == ZLOX_TRUE)
	{
		zlox_draw_my_mouse(&intersect_rect);
	}

	cmdwin->cmd_cursor_x = x;
	cmdwin->cmd_cursor_y = y;
	zlox_cmd_window_move_cursor(w_space, h_space);
	rect.x = cmdwin->x + cmdwin->cmd_rect.x + cmdwin->cmd_cursor_x * (ZLOX_VGA_CHAR_WIDTH + w_space);
	rect.y = cmdwin->y + cmdwin->cmd_rect.y + cmdwin->cmd_cursor_y * (ZLOX_VGA_CHAR_HEIGHT + h_space);
	zlox_draw_mywin_list(cmdwin, mywin_list_end, &rect);
	if(zlox_detect_intersect_rect(&rect, &mouse_tmp_rect, &intersect_rect) == ZLOX_TRUE)
	{
		zlox_draw_my_mouse(&intersect_rect);
	}
	return 0;
}

ZLOX_SINT32 zlox_cmd_window_set_single(ZLOX_BOOL flag)
{
	ZLOX_MY_WINDOW * cmdwin = current_task->cmd_win;
	if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
	{
		zlox_set_cmd_window(current_task, ZLOX_NULL, ZLOX_TRUE);
		cmdwin = current_task->cmd_win;
		if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
			return -1;
	}
	cmdwin->cmd_single_line_out = flag;
	return 0;
}

ZLOX_SINT32 zlox_cmd_window_clear()
{
	ZLOX_MY_WINDOW * cmdwin = current_task->cmd_win;
	if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
	{
		zlox_set_cmd_window(current_task, ZLOX_NULL, ZLOX_TRUE);
		cmdwin = current_task->cmd_win;
		if(cmdwin == ZLOX_NULL || cmdwin->has_cmd == ZLOX_FALSE)
			return -1;
	}
	ZLOX_SINT32 w_space = ZLOX_CMD_WINDOW_W_SPACE;
	ZLOX_SINT32 h_space = ZLOX_CMD_WINDOW_H_SPACE;
	ZLOX_SINT32 h_total_perchar = ZLOX_VGA_CHAR_HEIGHT + h_space;
	ZLOX_SINT32 w_total_perchar = ZLOX_VGA_CHAR_WIDTH + w_space;
	ZLOX_SINT32 w_charnum = cmdwin->cmd_rect.width / w_total_perchar;
	ZLOX_SINT32 h_charnum = cmdwin->cmd_rect.height / h_total_perchar;
	ZLOX_SINT32 w_total_pix = w_charnum * w_total_perchar;
	ZLOX_SINT32 h_total_pix = h_charnum * h_total_perchar;
	ZLOX_SINT32 x, y;
	for(y=0; y < h_total_pix; y++)
	{
		for(x=0; x < w_total_pix; x++)
		{
			cmdwin->bitmap[cmdwin->cmd_rect.x + x + (cmdwin->cmd_rect.y + y) * cmdwin->width]
					= cmdwin->cmd_back_color;
		}
	}
	cmdwin->cmd_cursor_x = 0;
	cmdwin->cmd_cursor_y = 0;
	zlox_cmd_window_move_cursor(w_space, h_space);

	ZLOX_MY_RECT rect, mouse_tmp_rect, intersect_rect;
	rect.x = cmdwin->x + cmdwin->cmd_rect.x;
	rect.y = cmdwin->y + cmdwin->cmd_rect.y;
	rect.width = cmdwin->cmd_rect.width;
	rect.height = cmdwin->cmd_rect.height;
	zlox_draw_mywin_list(cmdwin, mywin_list_end, &rect);
	mouse_tmp_rect.x = my_mouse_x;
	mouse_tmp_rect.y = my_mouse_y;
	mouse_tmp_rect.width = my_mouse_w;
	mouse_tmp_rect.height = my_mouse_h;
	if(zlox_detect_intersect_rect(&rect, &mouse_tmp_rect, &intersect_rect) == ZLOX_TRUE)
	{
		zlox_draw_my_mouse(&intersect_rect);
	}
	return 0;
}

ZLOX_SINT32 zlox_shift_tab_window()
{
	ZLOX_MY_WINDOW * stab_win = mywin_list_header->next;
	if(my_drag_win != ZLOX_NULL)
	{
		return -1;
	}
	if(stab_win != ZLOX_NULL && stab_win != mywin_list_end)
	{
		if(stab_win != mywin_list_kbd_input)
			mywin_list_kbd_input = stab_win;
		ZLOX_MY_WINDOW * lastwin = mywin_list_end;
		stab_win->prev->next = stab_win->next;
		stab_win->next->prev = stab_win->prev;
		lastwin->next = stab_win;
		stab_win->prev = lastwin;
		stab_win->next = ZLOX_NULL;
		stab_win->need_update = ZLOX_FALSE;
		zlox_draw_my_window(stab_win, ZLOX_NULL, ZLOX_NULL);
		lastwin = mywin_list_end = stab_win;
		ZLOX_MY_RECT rect, mouse_tmp_rect, intersect_rect;
		mouse_tmp_rect.x = my_mouse_x;
		mouse_tmp_rect.y = my_mouse_y;
		mouse_tmp_rect.width = my_mouse_w;
		mouse_tmp_rect.height = my_mouse_h;
		rect.x = stab_win->x;
		rect.y = stab_win->y;
		rect.width = stab_win->width;
		rect.height = stab_win->height;
		if(zlox_detect_intersect_rect(&rect, &mouse_tmp_rect, &intersect_rect) == ZLOX_TRUE)
		{
			zlox_draw_my_mouse(&intersect_rect);
		}
	}
	return 0 ;
}

static ZLOX_SINT32 zlox_draw_my_window(ZLOX_MY_WINDOW * my_window, 
		ZLOX_BOOL * need_update_mouse,ZLOX_MY_RECT * mouse_update_rect)
{
	ZLOX_MY_WINDOW * mywin = my_window;
	ZLOX_UINT32 * vid_mem = (ZLOX_UINT32 *)lfb_vid_memory;
	ZLOX_SINT32 x, y , j, w, h, src_x, src_y;
	ZLOX_SINT32 pix_byte = lfb_resolution_b / 8;
	if(mywin->need_update)
	{
		x = mywin->x + mywin->update_rect.x;
		y = mywin->y + mywin->update_rect.y;
		src_x = mywin->update_rect.x;
		src_y = mywin->update_rect.y;
		w = mywin->update_rect.width;
		h = mywin->update_rect.height;
	}
	else
	{
		x = mywin->x;
		y = mywin->y;
		src_x = 0;
		src_y = 0;
		w = mywin->width;
		h = mywin->height;
	}
	ZLOX_MY_RECT tmp_rect;
	ZLOX_MY_RECT lfb_tmp_rect;
	ZLOX_MY_RECT intersect_rect;
	tmp_rect.x = x;
	tmp_rect.y = y;
	tmp_rect.width = w;
	tmp_rect.height = h;
	lfb_tmp_rect.x = 0;
	lfb_tmp_rect.y = 0;
	lfb_tmp_rect.width = lfb_resolution_x;
	lfb_tmp_rect.height = lfb_resolution_y;
	if(zlox_detect_intersect_rect(&tmp_rect, &lfb_tmp_rect, &intersect_rect) == ZLOX_FALSE)
	{
		if(need_update_mouse != ZLOX_NULL)
			(*need_update_mouse) = ZLOX_FALSE;
		return -1;
	}
	x = intersect_rect.x;
	y = intersect_rect.y;
	src_x = intersect_rect.x - mywin->x;
	src_y = intersect_rect.y - mywin->y;
	w= intersect_rect.width;
	h = intersect_rect.height;
	for(j = 0; j < h ; y++, src_y++, j++)
	{
		zlox_memcpy((ZLOX_UINT8 *)&vid_mem[x + y * lfb_resolution_x], 
				(ZLOX_UINT8 *)&mywin->bitmap[src_x + src_y * mywin->width], 
				w * pix_byte);
	}
	if(need_update_mouse != ZLOX_NULL && mouse_update_rect != ZLOX_NULL)
	{
		tmp_rect.x = my_mouse_x;
		tmp_rect.y = my_mouse_y;
		tmp_rect.width = my_mouse_w;
		tmp_rect.height = my_mouse_h;
		(*need_update_mouse) = zlox_detect_intersect_rect(&tmp_rect, &intersect_rect, mouse_update_rect);
	}
	return 0;
}

static ZLOX_SINT32 zlox_draw_my_mouse(ZLOX_MY_RECT * mouse_rect)
{
	ZLOX_MY_RECT lfb_tmp_rect;
	lfb_tmp_rect.x = 0;
	lfb_tmp_rect.y = 0;
	lfb_tmp_rect.width = lfb_resolution_x;
	lfb_tmp_rect.height = lfb_resolution_y;
	ZLOX_MY_RECT intersect_rect;
	ZLOX_BOOL is_intersect = zlox_detect_intersect_rect(&lfb_tmp_rect, mouse_rect, &intersect_rect);
	ZLOX_UINT32 * vid_mem = (ZLOX_UINT32 *)lfb_vid_memory;
	ZLOX_SINT32 x, m_x, y, m_y , i, j;
	ZLOX_SINT32 diff_y = intersect_rect.y - my_mouse_y;
	ZLOX_SINT32 diff_x = intersect_rect.x - my_mouse_x;
	ZLOX_UINT32 * tmp_bitmap = (ZLOX_UINT32 *)mouse_bitmap_byte;
	if(is_intersect == ZLOX_FALSE)
		return -1;
	for(x = intersect_rect.x, y = intersect_rect.y, m_y = my_mouse_h - 1 - diff_y, j = 0; 
		j < intersect_rect.height; 
		y++, m_y--, j++)
	{
		for(i = 0, m_x = diff_x; i < intersect_rect.width; i++, m_x++)
		{
			if(tmp_bitmap[m_x + m_y * my_mouse_w] < 0x00D7D7D7)
			{
				if((vid_mem[x + i + y * lfb_resolution_x] & 0x0FFFFFF) == 0x0)
					vid_mem[x + i + y * lfb_resolution_x] = 0xFFFFFFFF;
				else
					vid_mem[x + i + y * lfb_resolution_x] = tmp_bitmap[m_x + m_y * my_mouse_w];
			}
		}
	}
	return 0;
}

static ZLOX_BOOL zlox_detect_intersect_rect(ZLOX_MY_RECT * rect1, ZLOX_MY_RECT * rect2, ZLOX_MY_RECT * d_rect)
{
	ZLOX_SINT32 r1_x = rect1->x + rect1->width - 1;
	ZLOX_SINT32 r1_y = rect1->y + rect1->height - 1;
	ZLOX_SINT32 r2_x = rect2->x + rect2->width - 1;
	ZLOX_SINT32 r2_y = rect2->y + rect2->height - 1;
	ZLOX_SINT32 d_x = 0;
	ZLOX_SINT32 d_y = 0;

	if(rect1->x >= rect2->x && rect1->x <= r2_x)
		d_rect->x = rect1->x;
	else if(rect2->x >= rect1->x && rect2->x <= r1_x)
		d_rect->x = rect2->x;
	else
		return ZLOX_FALSE;

	if(rect1->y >= rect2->y && rect1->y <= r2_y)
		d_rect->y = rect1->y;
	else if(rect2->y >= rect1->y && rect2->y <= r1_y)
		d_rect->y = rect2->y;
	else
		return ZLOX_FALSE;

	if(r1_x >= rect2->x && r1_x <= r2_x)
		d_x = r1_x;
	else if(r2_x >= rect1->x && r2_x <= r1_x)
		d_x = r2_x;
	else
		return ZLOX_FALSE;

	if(r1_y >= rect2->y && r1_y <= r2_y)
		d_y = r1_y;
	else if(r2_y >= rect1->y && r2_y <= r1_y)
		d_y = r2_y;
	else
		return ZLOX_FALSE;

	d_rect->width = d_x - d_rect->x + 1;
	d_rect->height = d_y - d_rect->y + 1;
	return ZLOX_TRUE;
}

static ZLOX_BOOL zlox_detect_point_at_rect(ZLOX_SINT32 x, ZLOX_SINT32 y, ZLOX_MY_RECT * rect)
{
	if(x >= rect->x && x <= (rect->x + rect->width - 1) &&
		y >= rect->y && y <= (rect->y + rect->height -1))
		return ZLOX_TRUE;
	return ZLOX_FALSE;
}

static ZLOX_SINT32 zlox_draw_mywin_list(ZLOX_MY_WINDOW * start, ZLOX_MY_WINDOW * end, ZLOX_MY_RECT * rect)
{
	ZLOX_MY_RECT win_tmp_rect;
	ZLOX_MY_RECT intersect_rect;
	ZLOX_MY_WINDOW * mywin = start;
	for(;mywin != ZLOX_NULL;)
	{
		win_tmp_rect.x = mywin->x;
		win_tmp_rect.y = mywin->y;
		win_tmp_rect.width = mywin->width;
		win_tmp_rect.height = mywin->height;
		if(zlox_detect_intersect_rect(&win_tmp_rect, rect, &intersect_rect) == ZLOX_TRUE)
		{
			mywin->need_update = ZLOX_TRUE;
			mywin->update_rect.x = intersect_rect.x - mywin->x;
			mywin->update_rect.y = intersect_rect.y - mywin->y;
			mywin->update_rect.width = intersect_rect.width;
			mywin->update_rect.height = intersect_rect.height;
			zlox_draw_my_window(mywin, ZLOX_NULL, ZLOX_NULL);
			mywin->need_update = ZLOX_FALSE;
			zlox_memset((ZLOX_UINT8 *)&mywin->update_rect, 0, sizeof(ZLOX_MY_RECT));
		}
		if(mywin == end)
			break;
		mywin = mywin->next;
	}
	return 0;
}

static ZLOX_SINT32 zlox_mymouse_first_click(ZLOX_TASK_MSG * msg)
{
	ZLOX_MY_WINDOW * lastwin = mywin_list_end;
	ZLOX_MY_WINDOW * mywin = lastwin;
	ZLOX_MY_WINDOW * found_win = ZLOX_NULL;
	for(;;)
	{
		if(my_mouse_x >= mywin->x && my_mouse_x <= (mywin->x + mywin->width - 1) &&
			my_mouse_y >= mywin->y && my_mouse_y <= (mywin->y + mywin->height -1))
		{
			found_win = mywin;
			break;
		}
		if(mywin == mywin_list_header)
			break;
		else
			mywin = mywin->prev;
	}
	if(found_win !=  ZLOX_NULL)
	{
		if(found_win != mywin_list_kbd_input)
			mywin_list_kbd_input = found_win;
		if(found_win != mywin_list_header &&
			found_win != lastwin)
		{
			found_win->prev->next = found_win->next;
			found_win->next->prev = found_win->prev;
			lastwin->next = found_win;
			found_win->prev = lastwin;
			found_win->next = ZLOX_NULL;
			found_win->need_update = ZLOX_FALSE;
			zlox_draw_my_window(found_win, ZLOX_NULL, ZLOX_NULL);
			lastwin = mywin_list_end = found_win;
		}
		ZLOX_BOOL flag_title = ZLOX_FALSE;
		ZLOX_BOOL flag_close = ZLOX_FALSE;
		ZLOX_MY_RECT rect;
		if(found_win->has_title == ZLOX_TRUE)
		{
			rect.x = found_win->x + found_win->title_rect.x;
			rect.y = found_win->y + found_win->title_rect.y;
			rect.width =  found_win->title_rect.width;
			rect.height = found_win->title_rect.height;
			if(zlox_detect_point_at_rect(my_mouse_x, my_mouse_y, &rect))
			{
				my_drag_win = found_win;
				my_drag_rect.x = found_win->x;
				my_drag_rect.y = found_win->y;
				my_drag_rect.width = found_win->width;
				my_drag_rect.height = found_win->height;
				flag_title = ZLOX_TRUE;
			}
			else
			{
				rect.x = found_win->x + found_win->close_btn_rect.x;
				rect.y = found_win->y + found_win->close_btn_rect.y;
				rect.width =  found_win->close_btn_rect.width;
				rect.height = found_win->close_btn_rect.height;
				if(zlox_detect_point_at_rect(my_mouse_x, my_mouse_y, &rect))
					flag_close = ZLOX_TRUE;
			}
		}
		if(flag_title == ZLOX_FALSE && flag_close == ZLOX_FALSE)
		{
			msg->mouse.btn = ZLOX_MMB_LEFT_DOWN;
			msg->mouse.rel_x = my_mouse_x - found_win->x;
			msg->mouse.rel_y = my_mouse_y - found_win->y;
			zlox_send_tskmsg((ZLOX_TASK *)found_win->task,msg);
		}
		else if(flag_close == ZLOX_TRUE)
		{
			msg->type = ZLOX_MT_CLOSE_MY_WINDOW;
			zlox_send_tskmsg((ZLOX_TASK *)found_win->task,msg);
		}
		my_mouse_left_press = ZLOX_TRUE;
		my_mouse_left_press_x = my_mouse_x;
		my_mouse_left_press_y = my_mouse_y;
	}
	return 0;
}

static ZLOX_SINT32 zlox_mymouse_drag(ZLOX_TASK_MSG * msg)
{
	//ZLOX_MY_RECT rect;
	if(my_drag_win != ZLOX_NULL && my_drag_rect.width > 0 && my_drag_rect.height > 0)
	{
		zlox_erase_drag_rect(mywin_list_header, mywin_list_end, &my_drag_rect);
		my_drag_rect.x += my_mouse_x - my_mouse_left_press_x;
		my_drag_rect.y += my_mouse_y - my_mouse_left_press_y;
		zlox_draw_drag_rect(&my_drag_rect);
	}
	else
	{
		ZLOX_BOOL flag_win = ZLOX_FALSE;
		ZLOX_MY_RECT rect;
		if(mywin_list_kbd_input->has_title == ZLOX_TRUE)
		{
			rect.x = mywin_list_kbd_input->x;
			rect.y = mywin_list_kbd_input->y + mywin_list_kbd_input->title_rect.height;
			rect.width = mywin_list_kbd_input->width;
			rect.height = mywin_list_kbd_input->height - mywin_list_kbd_input->title_rect.height;
			if(zlox_detect_point_at_rect(my_mouse_x, my_mouse_y, &rect))
				flag_win = ZLOX_TRUE;
		}
		else
		{
			rect.x = mywin_list_kbd_input->x;
			rect.y = mywin_list_kbd_input->y;
			rect.width = mywin_list_kbd_input->width;
			rect.height = mywin_list_kbd_input->height;
			if(zlox_detect_point_at_rect(my_mouse_x, my_mouse_y, &rect))
				flag_win = ZLOX_TRUE;
		}
		if(flag_win)
		{
			msg->mouse.btn = ZLOX_MMB_LEFT_DRAG;
			msg->mouse.rel_x = my_mouse_x - mywin_list_kbd_input->x;
			msg->mouse.rel_y = my_mouse_y - mywin_list_kbd_input->y;
			zlox_send_tskmsg((ZLOX_TASK *)mywin_list_kbd_input->task,msg);
		}
	}
	my_mouse_left_press = ZLOX_TRUE;
	my_mouse_left_press_x = my_mouse_x;
	my_mouse_left_press_y = my_mouse_y;
	return 0;
}

static ZLOX_SINT32 zlox_mymouse_release(ZLOX_TASK_MSG * msg)
{
	ZLOX_BOOL flag_win = ZLOX_FALSE;
	ZLOX_MY_RECT rect;

	if(my_drag_win != ZLOX_NULL && my_drag_rect.width > 0 && my_drag_rect.height > 0)
	{
		rect.x = my_drag_win->x;
		rect.y = my_drag_win->y;
		rect.width = my_drag_win->width;
		rect.height = my_drag_win->height;
		zlox_draw_mywin_list(mywin_list_header, my_drag_win->prev, &rect);
		if(my_drag_win->next != ZLOX_NULL)
			zlox_draw_mywin_list(my_drag_win->next, mywin_list_end, &rect);
		my_drag_win->x = my_drag_rect.x;
		my_drag_win->y = my_drag_rect.y;
		my_drag_win->width = my_drag_rect.width;
		my_drag_win->height = my_drag_rect.height;
		zlox_draw_mywin_list(my_drag_win, mywin_list_end, &my_drag_rect);
		my_drag_win = ZLOX_NULL;
		my_drag_rect.x = my_drag_rect.y = my_drag_rect.width = my_drag_rect.height = 0;
		my_mouse_left_press = ZLOX_FALSE;
		return 0;
	}

	if(mywin_list_kbd_input->has_title == ZLOX_TRUE)
	{
		rect.x = mywin_list_kbd_input->x;
		rect.y = mywin_list_kbd_input->y + mywin_list_kbd_input->title_rect.height;
		rect.width = mywin_list_kbd_input->width;
		rect.height = mywin_list_kbd_input->height - mywin_list_kbd_input->title_rect.height;
		if(zlox_detect_point_at_rect(my_mouse_x, my_mouse_y, &rect))
			flag_win = ZLOX_TRUE;
	}
	else
	{
		rect.x = mywin_list_kbd_input->x;
		rect.y = mywin_list_kbd_input->y;
		rect.width = mywin_list_kbd_input->width;
		rect.height = mywin_list_kbd_input->height;
		if(zlox_detect_point_at_rect(my_mouse_x, my_mouse_y, &rect))
			flag_win = ZLOX_TRUE;
	}
	if(flag_win)
	{
		msg->mouse.btn = ZLOX_MMB_LEFT_UP;
		msg->mouse.rel_x = my_mouse_x - mywin_list_kbd_input->x;
		msg->mouse.rel_y = my_mouse_y - mywin_list_kbd_input->y;
		zlox_send_tskmsg((ZLOX_TASK *)mywin_list_kbd_input->task,msg);
	}
	my_mouse_left_press = ZLOX_FALSE;
	return 0;
}

static ZLOX_BOOL zlox_cmd_window_scroll(ZLOX_SINT32 w_space, ZLOX_SINT32 h_space)
{
	ZLOX_MY_WINDOW * cmdwin = current_task->cmd_win;
	ZLOX_SINT32 h_total_perchar = ZLOX_VGA_CHAR_HEIGHT + h_space;
	ZLOX_SINT32 w_total_perchar = ZLOX_VGA_CHAR_WIDTH + w_space;
	ZLOX_SINT32 w_charnum = cmdwin->cmd_rect.width / w_total_perchar;
	ZLOX_SINT32 h_charnum = cmdwin->cmd_rect.height / h_total_perchar;
	if(cmdwin->cmd_cursor_y >= h_charnum)
	{
		ZLOX_SINT32 h_cpy_num = (h_charnum - 1) * h_total_perchar;
		ZLOX_SINT32 w_cpy_pix = w_charnum * w_total_perchar;
		ZLOX_SINT32 w_cpy_byte = w_cpy_pix * 4;
		ZLOX_UINT8 * dst, * src;
		for(ZLOX_SINT32 i=0; i < h_cpy_num;i++)
		{
			dst = (ZLOX_UINT8 *)&cmdwin->bitmap[cmdwin->cmd_rect.x + (cmdwin->cmd_rect.y + i) * cmdwin->width];
			src = (ZLOX_UINT8 *)&cmdwin->bitmap[cmdwin->cmd_rect.x + 
						(cmdwin->cmd_rect.y + h_total_perchar + i) * cmdwin->width];
			zlox_memcpy(dst, src, w_cpy_byte);
		}
		for(ZLOX_SINT32 i=0; i < h_total_perchar;i++)
		{
			for(ZLOX_SINT32 j=0; j < w_cpy_pix;j++)
				cmdwin->bitmap[cmdwin->cmd_rect.x + j + (cmdwin->cmd_rect.y + h_cpy_num + i) * cmdwin->width]
					= cmdwin->cmd_back_color;
		}
		cmdwin->cmd_cursor_y = h_charnum - 1;
		return ZLOX_TRUE;
	}
	return ZLOX_FALSE;
}

static ZLOX_SINT32 zlox_cmd_window_move_cursor(ZLOX_SINT32 w_space, ZLOX_SINT32 h_space)
{
	ZLOX_UINT8 * c = term_cursor_font[0];
	ZLOX_MY_WINDOW * cmdwin = current_task->cmd_win;
	ZLOX_SINT32 x = cmdwin->cmd_rect.x + cmdwin->cmd_cursor_x * (ZLOX_VGA_CHAR_WIDTH + w_space);
	ZLOX_SINT32 y = cmdwin->cmd_rect.y + cmdwin->cmd_cursor_y * (ZLOX_VGA_CHAR_HEIGHT + h_space);
	for (ZLOX_UINT8 i = 0; i < ZLOX_VGA_CHAR_HEIGHT; ++i) {
		for (ZLOX_UINT8 j = 0; j < ZLOX_VGA_CHAR_WIDTH; ++j) {
			ZLOX_SINT32 index = (x+j) + (y+i) * cmdwin->width;
			if (c[i] & (1 << (8-j))) {
				if(cmdwin->bitmap[index] == cmdwin->cmd_font_color)
					cmdwin->bitmap[index] = cmdwin->cmd_back_color;
				else
					cmdwin->bitmap[index] = cmdwin->cmd_font_color;
			}
		}
	}
	return 0;
}

static ZLOX_SINT32 zlox_cmd_window_write_char(ZLOX_SINT32 x, ZLOX_SINT32 y, ZLOX_SINT32 val, ZLOX_UINT32 color, ZLOX_UINT32 backColour)
{
	if (val > 128) {
		val = 4;
	}
	ZLOX_UINT8 * c = number_font[val];
	ZLOX_MY_WINDOW * cmdwin = current_task->cmd_win;
	for (ZLOX_UINT8 i = 0; i < ZLOX_VGA_CHAR_HEIGHT; ++i) {
		for (ZLOX_UINT8 j = 0; j < ZLOX_VGA_CHAR_WIDTH; ++j) {
			ZLOX_SINT32 index = (x+j) + (y+i) * cmdwin->width;
			if (c[i] & (1 << (8-j))) {				
				cmdwin->bitmap[index] = color;
			}
			else 
			{
				cmdwin->bitmap[index] = backColour;
			}
		}
	}
	return val;
}

/*static ZLOX_SINT32 zlox_erase_drag_rect(ZLOX_MY_WINDOW * start, ZLOX_MY_WINDOW * end, ZLOX_MY_RECT * rect)
{
	ZLOX_MY_RECT lfb_tmp_rect;
	lfb_tmp_rect.x = 0;
	lfb_tmp_rect.y = 0;
	lfb_tmp_rect.width = lfb_resolution_x;
	lfb_tmp_rect.height = lfb_resolution_y;
	ZLOX_MY_RECT intersect_rect_lfb;
	ZLOX_MY_RECT intersect_rect;
	ZLOX_MY_RECT win_rect;
	ZLOX_MY_WINDOW * tmpwin = start;
	ZLOX_UINT32 * vid_mem = (ZLOX_UINT32 *)lfb_vid_memory;
	while(ZLOX_TRUE)
	{
		if(zlox_detect_intersect_rect(&lfb_tmp_rect, rect, &intersect_rect_lfb))
		{
			win_rect.x = tmpwin->x;
			win_rect.y = tmpwin->y;
			win_rect.width = tmpwin->width;
			win_rect.height = tmpwin->height;
			if(zlox_detect_intersect_rect(&win_rect, &intersect_rect_lfb, &intersect_rect))
			{
				ZLOX_SINT32 x, y, win_x, win_y;
				for(y = intersect_rect.y; y < (intersect_rect.y + intersect_rect.height); y++)
				{
					for(x = intersect_rect.x; x < (intersect_rect.x + intersect_rect.width);)
					{
						if(x == rect->x || x == (rect->x + rect->width - 1) || 
							y == rect->y || y == (rect->y + rect->height - 1))
						{
							win_x = x - tmpwin->x;
							win_y = y - tmpwin->y;
							vid_mem[x + y * lfb_resolution_x] = tmpwin->bitmap[win_x + win_y * tmpwin->width];
						}
						if(y == intersect_rect.y || y == (intersect_rect.y + intersect_rect.height - 1))
							x++;
						else
							x += (intersect_rect.width - 1) > 0 ? (intersect_rect.width - 1) : 1;
					}
				}
			}
		}
		if(tmpwin == end)
			break;
		tmpwin = tmpwin->next;
	}
	return 0;
}*/

static ZLOX_SINT32 zlox_erase_drag_rect(ZLOX_MY_WINDOW * start, ZLOX_MY_WINDOW * end, ZLOX_MY_RECT * rect)
{
	ZLOX_MY_RECT lfb_tmp_rect;
	lfb_tmp_rect.x = 0;
	lfb_tmp_rect.y = 0;
	lfb_tmp_rect.width = lfb_resolution_x;
	lfb_tmp_rect.height = lfb_resolution_y;
	ZLOX_MY_RECT intersect_rect_lfb;
	ZLOX_MY_RECT intersect_rect;
	ZLOX_MY_RECT win_rect;
	ZLOX_MY_WINDOW * tmpwin = start;
	ZLOX_SINT32 pix_byte = lfb_resolution_b / 8;
	ZLOX_UINT32 * vid_mem = (ZLOX_UINT32 *)lfb_vid_memory;
	while(ZLOX_TRUE)
	{
		if(zlox_detect_intersect_rect(&lfb_tmp_rect, rect, &intersect_rect_lfb))
		{
			win_rect.x = tmpwin->x;
			win_rect.y = tmpwin->y;
			win_rect.width = tmpwin->width;
			win_rect.height = tmpwin->height;
			if(zlox_detect_intersect_rect(&win_rect, &intersect_rect_lfb, &intersect_rect))
			{
				ZLOX_SINT32 x, y, win_x, win_y;
				ZLOX_BOOL need_draw_top = ZLOX_TRUE;
				ZLOX_BOOL need_draw_bottom = ZLOX_TRUE;
				if(intersect_rect.y == rect->y)
				{
					x = intersect_rect.x;
					y = intersect_rect.y;
					win_x = x - tmpwin->x;
					win_y = y - tmpwin->y;
					if(intersect_rect.width > 0)
					{
						zlox_memcpy((ZLOX_UINT8 *)&vid_mem[x + y * lfb_resolution_x], 
							(ZLOX_UINT8 *)&tmpwin->bitmap[win_x + win_y * tmpwin->width], 
							intersect_rect.width * pix_byte);
						if(need_draw_top)
							need_draw_top = ZLOX_FALSE;
					}
				}

				if(intersect_rect.height > 1 && rect->height > 1)
				{
					if((intersect_rect.y + intersect_rect.height - 1) == 
					   (rect->y + rect->height - 1))
					{
						x = intersect_rect.x;
						y = intersect_rect.y + intersect_rect.height - 1;
						win_x = x - tmpwin->x;
						win_y = y - tmpwin->y;
						if(intersect_rect.width > 0)
						{
							zlox_memcpy((ZLOX_UINT8 *)&vid_mem[x + y * lfb_resolution_x], 
								(ZLOX_UINT8 *)&tmpwin->bitmap[win_x + win_y * tmpwin->width], 
								intersect_rect.width * pix_byte);
							if(need_draw_bottom)
								need_draw_bottom = ZLOX_FALSE;
						}
					}
				}

				for(y = intersect_rect.y; y < (intersect_rect.y + intersect_rect.height); y++)
				{
					if(y == intersect_rect.y)
						if(need_draw_top == ZLOX_FALSE)
							continue;
					if(y == (intersect_rect.y + intersect_rect.height - 1))
						if(need_draw_bottom == ZLOX_FALSE)
							continue;
					for(x = intersect_rect.x; x < (intersect_rect.x + intersect_rect.width);)
					{
						if(x == rect->x || 
						   x == (rect->x + rect->width - 1))
						{
							win_x = x - tmpwin->x;
							win_y = y - tmpwin->y;
							vid_mem[x + y * lfb_resolution_x] = 
									tmpwin->bitmap[win_x + win_y * tmpwin->width];
						}
						x += (intersect_rect.width - 1) > 0 ? (intersect_rect.width - 1) : 1;
					}
				}
			}
		}
		if(tmpwin == end)
			break;
		tmpwin = tmpwin->next;
	}
	return 0;
}

/*static ZLOX_SINT32 zlox_draw_drag_rect(ZLOX_MY_RECT * rect)
{
	ZLOX_MY_RECT lfb_tmp_rect;
	lfb_tmp_rect.x = 0;
	lfb_tmp_rect.y = 0;
	lfb_tmp_rect.width = lfb_resolution_x;
	lfb_tmp_rect.height = lfb_resolution_y;
	ZLOX_MY_RECT intersect_rect;
	ZLOX_UINT32 * vid_mem = (ZLOX_UINT32 *)lfb_vid_memory;
	if(zlox_detect_intersect_rect(&lfb_tmp_rect, rect, &intersect_rect))
	{
		ZLOX_SINT32 x, y;
		for(y = intersect_rect.y; y < (intersect_rect.y + intersect_rect.height); y++)
		{
			for(x = intersect_rect.x; x < (intersect_rect.x + intersect_rect.width);)
			{
				if(x == rect->x || x == (rect->x + rect->width - 1) || 
					y == rect->y || y == (rect->y + rect->height - 1))
				{
					vid_mem[x + y * lfb_resolution_x] = 0xFFFFFFFF;
				}
				if(y == intersect_rect.y || y == (intersect_rect.y + intersect_rect.height - 1))
					x++;
				else
					x += (intersect_rect.width - 1) > 0 ? (intersect_rect.width - 1) : 1;
			}
		}
	}
	return 0;
}*/

static ZLOX_SINT32 zlox_draw_drag_rect(ZLOX_MY_RECT * rect)
{
	ZLOX_MY_RECT lfb_tmp_rect;
	lfb_tmp_rect.x = 0;
	lfb_tmp_rect.y = 0;
	lfb_tmp_rect.width = lfb_resolution_x;
	lfb_tmp_rect.height = lfb_resolution_y;
	ZLOX_MY_RECT intersect_rect;
	ZLOX_SINT32 pix_byte = lfb_resolution_b / 8;
	ZLOX_UINT32 * vid_mem = (ZLOX_UINT32 *)lfb_vid_memory;
	if(zlox_detect_intersect_rect(&lfb_tmp_rect, rect, &intersect_rect))
	{
		ZLOX_SINT32 x, y;
		ZLOX_BOOL need_draw_top = ZLOX_TRUE;
		ZLOX_BOOL need_draw_bottom = ZLOX_TRUE;
		if(intersect_rect.y == rect->y)
		{
			x = intersect_rect.x;
			y = intersect_rect.y;
			if(intersect_rect.width > 0)
			{
				zlox_memset((ZLOX_UINT8 *)&vid_mem[x + y * lfb_resolution_x], 0xFF, 
					intersect_rect.width * pix_byte);
				if(need_draw_top)
					need_draw_top = ZLOX_FALSE;
				
			}
		}

		if(intersect_rect.height > 1 && rect->height > 1)
		{
			if((intersect_rect.y + intersect_rect.height - 1) == 
			   (rect->y + rect->height - 1))
			{
				x = intersect_rect.x;
				y = intersect_rect.y + intersect_rect.height - 1;
				if(intersect_rect.width > 0)
				{
					zlox_memset((ZLOX_UINT8 *)&vid_mem[x + y * lfb_resolution_x], 0xFF, 
						intersect_rect.width * pix_byte);
					if(need_draw_bottom)
						need_draw_bottom = ZLOX_FALSE;
				}
			}
		}

		for(y = intersect_rect.y; y < (intersect_rect.y + intersect_rect.height); y++)
		{
			if(y == intersect_rect.y)
				if(need_draw_top == ZLOX_FALSE)
					continue;
			if(y == (intersect_rect.y + intersect_rect.height - 1))
				if(need_draw_bottom == ZLOX_FALSE)
					continue;
			for(x = intersect_rect.x; x < (intersect_rect.x + intersect_rect.width);)
			{
				if(x == rect->x || x == (rect->x + rect->width - 1))
				{
					vid_mem[x + y * lfb_resolution_x] = 0xFFFFFFFF;
				}
				x += (intersect_rect.width - 1) > 0 ? (intersect_rect.width - 1) : 1;
			}
		}
	}
	return 0;
}

