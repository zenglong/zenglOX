// zlox_usb_keyboard.c -- the C file relate to USB Keyboard

#include "zlox_usb_keyboard.h"
#include "zlox_monitor.h"
#include "zlox_task.h"
#include "zlox_my_windows.h"

// the follow defined in zlox_keyboard.c
#define ZLOX_LED_NUM_LOCK          2
#define ZLOX_LED_SCROLL_LOCK       1
#define ZLOX_LED_CAPS_LOCK         4

#define ZLOX_CK_SHIFT              1
#define ZLOX_CK_ALT                2
#define ZLOX_CK_CTRL               4

// zlox_keyboard.c
extern ZLOX_UINT8 led_status;
extern ZLOX_UINT8 control_keys;

typedef struct _ZLOX_USB_KBD_KEY_MAP
{
	ZLOX_UINT8 base[128];
	ZLOX_UINT8 shift[128];
	ZLOX_UINT8 numlock[128];
} ZLOX_USB_KBD_KEY_MAP;

typedef struct _ZLOX_USB_KBD
{
	ZLOX_USB_TRANSFER dataTransfer;
	ZLOX_UINT8 data[8];
	//ZLOX_UINT8 lastData[8];
	ZLOX_SINT32 usage_num;
	ZLOX_SINT32 repeat_cur;
	ZLOX_SINT32 repeat_num;
	ZLOX_SINT32 rep_dis_num;
} ZLOX_USB_KBD;

ZLOX_USB_KBD_KEY_MAP g_keymapUs101 = 
{
	.base =
	{
		[ZLOX_USB_KBD_KEY_1] = '1',
		[ZLOX_USB_KBD_KEY_2] = '2',
		[ZLOX_USB_KBD_KEY_3] = '3',
		[ZLOX_USB_KBD_KEY_4] = '4',
		[ZLOX_USB_KBD_KEY_5] = '5',
		[ZLOX_USB_KBD_KEY_6] = '6',
		[ZLOX_USB_KBD_KEY_7] = '7',
		[ZLOX_USB_KBD_KEY_8] = '8',
		[ZLOX_USB_KBD_KEY_9] = '9',
		[ZLOX_USB_KBD_KEY_0] = '0',
		[ZLOX_USB_KBD_KEY_MINUS] = '-',
		[ZLOX_USB_KBD_KEY_EQUALS] = '=',
		[ZLOX_USB_KBD_KEY_Q] = 'q',
		[ZLOX_USB_KBD_KEY_W] = 'w',
		[ZLOX_USB_KBD_KEY_E] = 'e',
		[ZLOX_USB_KBD_KEY_R] = 'r',
		[ZLOX_USB_KBD_KEY_T] = 't',
		[ZLOX_USB_KBD_KEY_Y] = 'y',
		[ZLOX_USB_KBD_KEY_U] = 'u',
		[ZLOX_USB_KBD_KEY_I] = 'i',
		[ZLOX_USB_KBD_KEY_O] = 'o',
		[ZLOX_USB_KBD_KEY_P] = 'p',
		[ZLOX_USB_KBD_KEY_LBRACKET] = '[',
		[ZLOX_USB_KBD_KEY_RBRACKET] = ']',
		[ZLOX_USB_KBD_KEY_A] = 'a',
		[ZLOX_USB_KBD_KEY_S] = 's',
		[ZLOX_USB_KBD_KEY_D] = 'd',
		[ZLOX_USB_KBD_KEY_F] = 'f',
		[ZLOX_USB_KBD_KEY_G] = 'g',
		[ZLOX_USB_KBD_KEY_H] = 'h',
		[ZLOX_USB_KBD_KEY_J] = 'j',
		[ZLOX_USB_KBD_KEY_K] = 'k',
		[ZLOX_USB_KBD_KEY_L] = 'l',
		[ZLOX_USB_KBD_KEY_SEMICOLON] = ';',
		[ZLOX_USB_KBD_KEY_APOSTROPHE] = '\'',
		[ZLOX_USB_KBD_KEY_GRAVE] = '`',
		[ZLOX_USB_KBD_KEY_BACKSLASH] = '\\',
		[ZLOX_USB_KBD_KEY_Z] = 'z',
		[ZLOX_USB_KBD_KEY_X] = 'x',
		[ZLOX_USB_KBD_KEY_C] = 'c',
		[ZLOX_USB_KBD_KEY_V] = 'v',
		[ZLOX_USB_KBD_KEY_B] = 'b',
		[ZLOX_USB_KBD_KEY_N] = 'n',
		[ZLOX_USB_KBD_KEY_M] = 'm',
		[ZLOX_USB_KBD_KEY_COMMA] = ',',
		[ZLOX_USB_KBD_KEY_PERIOD] = '.',
		[ZLOX_USB_KBD_KEY_KP_DEC] = '.',
		[ZLOX_USB_KBD_KEY_SLASH] = '/',
		[ZLOX_USB_KBD_KEY_KP_MUL] = '*',
		[ZLOX_USB_KBD_KEY_KP_DIV] = '/',
		[ZLOX_USB_KBD_KEY_SPACE] = ' ',
		[ZLOX_USB_KBD_KEY_KP_SUB] = '-',
		[ZLOX_USB_KBD_KEY_KP_ADD] = '+',
		[ZLOX_USB_KBD_KEY_RETURN] = '\n',
		[ZLOX_USB_KBD_KEY_ENTER] = '\n',
		[ZLOX_USB_KBD_KEY_BACKSPACE] = '\b',
		[ZLOX_USB_KBD_KEY_TAB] = '\t',
		[ZLOX_USB_KBD_KEY_ESCAPE] = ZLOX_USB_KBD_CH_ESCAPE,
	},
	.shift =
	{
		[ZLOX_USB_KBD_KEY_1] = '!',
		[ZLOX_USB_KBD_KEY_2] = '@',
		[ZLOX_USB_KBD_KEY_3] = '#',
		[ZLOX_USB_KBD_KEY_4] = '$',
		[ZLOX_USB_KBD_KEY_5] = '%',
		[ZLOX_USB_KBD_KEY_6] = '^',
		[ZLOX_USB_KBD_KEY_7] = '&',
		[ZLOX_USB_KBD_KEY_8] = '*',
		[ZLOX_USB_KBD_KEY_9] = '(',
		[ZLOX_USB_KBD_KEY_0] = ')',
		[ZLOX_USB_KBD_KEY_MINUS] = '_',
		[ZLOX_USB_KBD_KEY_EQUALS] = '+',
		[ZLOX_USB_KBD_KEY_Q] = 'Q',
		[ZLOX_USB_KBD_KEY_W] = 'W',
		[ZLOX_USB_KBD_KEY_E] = 'E',
		[ZLOX_USB_KBD_KEY_R] = 'R',
		[ZLOX_USB_KBD_KEY_T] = 'T',
		[ZLOX_USB_KBD_KEY_Y] = 'Y',
		[ZLOX_USB_KBD_KEY_U] = 'U',
		[ZLOX_USB_KBD_KEY_I] = 'I',
		[ZLOX_USB_KBD_KEY_O] = 'O',
		[ZLOX_USB_KBD_KEY_P] = 'P',
		[ZLOX_USB_KBD_KEY_LBRACKET] = '{',
		[ZLOX_USB_KBD_KEY_RBRACKET] = '}',
		[ZLOX_USB_KBD_KEY_A] = 'A',
		[ZLOX_USB_KBD_KEY_S] = 'S',
		[ZLOX_USB_KBD_KEY_D] = 'D',
		[ZLOX_USB_KBD_KEY_F] = 'F',
		[ZLOX_USB_KBD_KEY_G] = 'G',
		[ZLOX_USB_KBD_KEY_H] = 'H',
		[ZLOX_USB_KBD_KEY_J] = 'J',
		[ZLOX_USB_KBD_KEY_K] = 'K',
		[ZLOX_USB_KBD_KEY_L] = 'L',
		[ZLOX_USB_KBD_KEY_SEMICOLON] = ':',
		[ZLOX_USB_KBD_KEY_APOSTROPHE] = '"',
		[ZLOX_USB_KBD_KEY_GRAVE] = '~',
		[ZLOX_USB_KBD_KEY_BACKSLASH] = '|',
		[ZLOX_USB_KBD_KEY_Z] = 'Z',
		[ZLOX_USB_KBD_KEY_X] = 'X',
		[ZLOX_USB_KBD_KEY_C] = 'C',
		[ZLOX_USB_KBD_KEY_V] = 'V',
		[ZLOX_USB_KBD_KEY_B] = 'B',
		[ZLOX_USB_KBD_KEY_N] = 'N',
		[ZLOX_USB_KBD_KEY_M] = 'M',
		[ZLOX_USB_KBD_KEY_COMMA] = '<',
		[ZLOX_USB_KBD_KEY_PERIOD] = '>',
		[ZLOX_USB_KBD_KEY_SLASH] = '?',
		[ZLOX_USB_KBD_KEY_KP_MUL] = '*',
		[ZLOX_USB_KBD_KEY_KP_DIV] = '/',
		[ZLOX_USB_KBD_KEY_ENTER] = '\n',
		[ZLOX_USB_KBD_KEY_SPACE] = ' ',
		[ZLOX_USB_KBD_KEY_KP_SUB] = '-',
		[ZLOX_USB_KBD_KEY_KP_ADD] = '+',
		[ZLOX_USB_KBD_KEY_KP_DEC] = '.',
		[ZLOX_USB_KBD_KEY_TAB] = ZLOX_USB_KBD_CH_SHIFT_TAB,
	},
	.numlock =
	{
		[ZLOX_USB_KBD_KEY_KP0] = '0',
		[ZLOX_USB_KBD_KEY_KP1] = '1',
		[ZLOX_USB_KBD_KEY_KP2] = '2',
		[ZLOX_USB_KBD_KEY_KP3] = '3',
		[ZLOX_USB_KBD_KEY_KP4] = '4',
		[ZLOX_USB_KBD_KEY_KP5] = '5',
		[ZLOX_USB_KBD_KEY_KP6] = '6',
		[ZLOX_USB_KBD_KEY_KP7] = '7',
		[ZLOX_USB_KBD_KEY_KP8] = '8',
		[ZLOX_USB_KBD_KEY_KP9] = '9',
	},
};

static ZLOX_VOID zlox_usb_kbd_process(ZLOX_USB_KBD * kbd)
{
	ZLOX_UINT8 * data = kbd->data;
	ZLOX_SINT32 usage_num = 0;
	ZLOX_SINT32 usage_cur = -1;
	ZLOX_BOOL status_control = ZLOX_FALSE;
	ZLOX_BOOL ctrl_press = ZLOX_FALSE;
	ZLOX_BOOL ctrl_release = ZLOX_FALSE;

	// debug print
	/*for (ZLOX_UINT32 i = 0; i < 8; ++i)
	{
		zlox_monitor_write_hex(data[i]);
		zlox_monitor_write(" ");
	}
	zlox_monitor_write("\n");*/

	if((data[0] & ZLOX_USB_KBD_KEY_LSHIFT) || 
		(data[0] & ZLOX_USB_KBD_KEY_RSHIFT))
	{
		if(!(control_keys & ZLOX_CK_SHIFT))
		{
			control_keys |= ZLOX_CK_SHIFT;
			status_control = ZLOX_TRUE;
			kbd->rep_dis_num = 0;
		}
	}
	else
	{
		if(control_keys & ZLOX_CK_SHIFT)
		{
			control_keys &= (0xFF - ZLOX_CK_SHIFT);
			status_control = ZLOX_TRUE;
		}
	}

	if((data[0] & ZLOX_USB_KBD_KEY_LALT) || 
		(data[0] & ZLOX_USB_KBD_KEY_RALT))
	{
		if(!(control_keys & ZLOX_CK_ALT))
		{
			control_keys |= ZLOX_CK_ALT;
			status_control = ZLOX_TRUE;
			kbd->rep_dis_num = 0;
		}
	}
	else
	{
		if(control_keys & ZLOX_CK_ALT)
		{
			control_keys &= (0xFF - ZLOX_CK_ALT);
			status_control = ZLOX_TRUE;
		}
	}

	if((data[0] & ZLOX_USB_KBD_KEY_LCONTROL) || 
		(data[0] & ZLOX_USB_KBD_KEY_RCONTROL))
	{
		if(!(control_keys & ZLOX_CK_CTRL))
		{
			control_keys |= ZLOX_CK_CTRL;
			status_control = ZLOX_TRUE;
			kbd->rep_dis_num = 0;
			ctrl_press = ZLOX_TRUE;
		}
	}
	else
	{
		if(control_keys & ZLOX_CK_CTRL)
		{
			control_keys &= (0xFF - ZLOX_CK_CTRL);
			status_control = ZLOX_TRUE;
			kbd->rep_dis_num = 16;
			ctrl_release = ZLOX_TRUE;
		}
	}

	for (ZLOX_SINT32 i = 0; i < 8; ++i)
	{
		if(i >= 2 && i<= 7 && data[i] != 0)
			usage_num++;
	}

	if(usage_num > kbd->usage_num)
	{
		usage_cur = usage_num - 1;
		kbd->repeat_cur = -1;
	}
	else if(usage_num < kbd->usage_num)
	{
		kbd->repeat_cur = -1;
	}
	else
	{
		usage_cur = kbd->repeat_cur + 1;
		if(usage_num == 0)
			usage_cur = -1;
		else if(usage_cur >= usage_num)
			usage_cur = 0;
		kbd->repeat_cur = usage_cur;
	}

	if(kbd->repeat_cur < 0)
	{
		kbd->repeat_num = 0;
		kbd->rep_dis_num = 0;
	}
	else
		kbd->repeat_num++;

	if(ctrl_press == ZLOX_TRUE)
	{
		ZLOX_TASK_MSG ascii_msg = {0};
		ascii_msg.type = ZLOX_MT_KEYBOARD;
		ascii_msg.keyboard.type = ZLOX_MKT_KEY;
		ascii_msg.keyboard.key = ZLOX_MKK_CTRL_PRESS;
		zlox_update_for_mykbd(&ascii_msg);
	}
	else if(ctrl_release == ZLOX_TRUE)
	{
		ZLOX_TASK_MSG ascii_msg = {0};
		ascii_msg.type = ZLOX_MT_KEYBOARD;
		ascii_msg.keyboard.type = ZLOX_MKT_KEY;
		ascii_msg.keyboard.key = ZLOX_MKK_CTRL_RELEASE;
		zlox_update_for_mykbd(&ascii_msg);
	}

	if(usage_cur >= 0)
	{
		ZLOX_SINT32 index = usage_cur + 2;
		ZLOX_UINT8 key_ascii = 0;
		ZLOX_UINT32 key_code = 0;

		//if(led_status & ZLOX_LED_NUM_LOCK)
		//{
			key_ascii = g_keymapUs101.numlock[data[index]];
		//}

		if(!key_ascii)
		{
			if(control_keys & ZLOX_CK_SHIFT)
			{
				key_ascii = g_keymapUs101.shift[data[index]];
			}
			else
			{
				key_ascii = g_keymapUs101.base[data[index]];
			}
		}

		if(status_control)
		{
			;
		}
		else if(key_ascii == ZLOX_USB_KBD_CH_SHIFT_TAB)
			zlox_shift_tab_window();
		else if(key_ascii)
		{
			if(led_status & ZLOX_LED_CAPS_LOCK)
			{
				if (key_ascii >= 'a' && key_ascii <= 'z')
				{
					key_ascii += 'A' - 'a';
				}
				else if (key_ascii >= 'A' && key_ascii <= 'Z')
				{
					key_ascii += 'a' - 'A';
				}
			}

			ZLOX_TASK_MSG ascii_msg = {0};
			ascii_msg.type = ZLOX_MT_KEYBOARD;
			ascii_msg.keyboard.type = ZLOX_MKT_ASCII;
			ascii_msg.keyboard.ascii = key_ascii;

			/*if(key_ascii)
			{
				if(kbd->rep_dis_num <= 0)
				{
					if(kbd->repeat_num % 2 == 0)
					{
						zlox_update_for_mykbd(&ascii_msg);
					}
					else
					{
						kbd->repeat_cur = ((kbd->repeat_cur - 1) >= -1) ? 
								(kbd->repeat_cur - 1) : -1;
					}

					if(kbd->rep_dis_num < 0)
						kbd->rep_dis_num = 0;
				}
				else
				{
					kbd->rep_dis_num--;
				}
			}*/

			if(kbd->repeat_num == 0 || 
			   (kbd->repeat_num > 0 && kbd->rep_dis_num <= 0))
			{
				switch(data[index])
				{
				case ZLOX_USB_KBD_KEY_KP_DIV:
				case ZLOX_USB_KBD_KEY_ENTER:
					if(kbd->repeat_num % 2 == 0)
						zlox_update_for_mykbd(&ascii_msg);
					break;
				default:
					//if(kbd->repeat_num % 2 == 0)
					zlox_update_for_mykbd(&ascii_msg);
					break;
				}
			}
			else if(kbd->rep_dis_num > 0)
			{
				kbd->rep_dis_num--;
			}
		}
		else
		{
			switch(data[index])
			{
			// qemu 2.2.0 use KEY_CAPS_LOCK2(0x32) for CAPS key
			case ZLOX_USB_KBD_KEY_CAPS_LOCK2:
			case ZLOX_USB_KBD_KEY_CAPS_LOCK:
				if(kbd->repeat_cur < 0)
				{
					led_status ^= ZLOX_LED_CAPS_LOCK;
				}
				break;
			/*case ZLOX_USB_KBD_KEY_NUM_LOCK:
				if(kbd->repeat_cur < 0)
				{
					led_status ^= ZLOX_LED_NUM_LOCK;
				}
				break;*/
			case ZLOX_USB_KBD_KEY_SCROLL_LOCK:
				if(kbd->repeat_cur < 0)
				{
					led_status ^= ZLOX_LED_SCROLL_LOCK;
				}
				break;
			case ZLOX_USB_KBD_KEY_F1:
				key_code = ZLOX_MKK_F1_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_F2:
				key_code = ZLOX_MKK_F2_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_F3:
				key_code = ZLOX_MKK_F3_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_F4:
				key_code = ZLOX_MKK_F4_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_F5:
				key_code = ZLOX_MKK_F5_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_F6:
				key_code = ZLOX_MKK_F6_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_F7:
				key_code = ZLOX_MKK_F7_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_F8:
				key_code = ZLOX_MKK_F8_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_F9:
				key_code = ZLOX_MKK_F9_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_F10:
				key_code = ZLOX_MKK_F10_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_HOME:
				key_code = ZLOX_MKK_HOME_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_UP:
				key_code = ZLOX_MKK_CURSOR_UP_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_PAGE_UP:
				key_code = ZLOX_MKK_PAGE_UP_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_LEFT:
				key_code = ZLOX_MKK_CURSOR_LEFT_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_RIGHT:
				key_code = ZLOX_MKK_CURSOR_RIGHT_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_END:
				key_code = ZLOX_MKK_END_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_DOWN:
				key_code = ZLOX_MKK_CURSOR_DOWN_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_PAGE_DOWN:
				key_code = ZLOX_MKK_PAGE_DOWN_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_INSERT:
				key_code = ZLOX_MKK_INSERT_PRESS;
				break;
			case ZLOX_USB_KBD_KEY_DELETE:
				key_code = ZLOX_MKK_DELETE_PRESS;
				break;
			}
			if(key_code)
			{
				if(kbd->repeat_num % 2 == 0)
				{
					ZLOX_TASK_MSG ascii_msg = {0};
					ascii_msg.type = ZLOX_MT_KEYBOARD;
					ascii_msg.keyboard.type = ZLOX_MKT_KEY;
					ascii_msg.keyboard.key = key_code;
					zlox_update_for_mykbd(&ascii_msg);
				}
				else
				{
					kbd->repeat_cur = ((kbd->repeat_cur - 1) >= -1) ? 
							(kbd->repeat_cur - 1) : -1;
				}
			}
		}
	}

	kbd->usage_num = usage_num;
}

static ZLOX_VOID zlox_usb_kbd_poll(ZLOX_USB_DEVICE * dev)
{
	ZLOX_USB_KBD * kbd = dev->drv;
	ZLOX_USB_TRANSFER * t = &kbd->dataTransfer;

	if (t->complete)
	{
		if (t->success)
		{
			zlox_usb_kbd_process(kbd);
		}

		t->complete = ZLOX_FALSE;
		dev->hcIntr(dev, t);
	}
}

ZLOX_BOOL zlox_usb_kbd_init(ZLOX_USB_DEVICE * dev)
{
	if (dev->intfDesc.intfClass != ZLOX_USB_CLASS_HID ||
		dev->intfDesc.intfSubClass != ZLOX_USB_SUBCLASS_BOOT ||
		dev->intfDesc.intfProtocol != ZLOX_USB_PROTOCOL_KBD)
		return ZLOX_FALSE;

	zlox_monitor_write("Initializing usb keyboard\n");

	ZLOX_USB_KBD * kbd = (ZLOX_USB_KBD *)zlox_usb_get_cont_phys(sizeof(ZLOX_USB_KBD));
	zlox_memset((ZLOX_UINT8 *)kbd, 0, sizeof(ZLOX_USB_KBD));
	kbd->repeat_cur = -1;

	dev->drv = kbd;
	dev->drvPoll = zlox_usb_kbd_poll;

	ZLOX_UINT32 intfIndex = dev->intfDesc.intfIndex;

	// Only send interrupt report when data changes
	zlox_usb_dev_request(dev,
		ZLOX_USB_RT_HOST_TO_DEV | ZLOX_USB_RT_CLASS | ZLOX_USB_RT_INTF,
		ZLOX_USB_REQ_SET_IDLE, 0, intfIndex,
		0, 0);

	// Prepare transfer
	ZLOX_USB_TRANSFER * t = &kbd->dataTransfer;
	zlox_memset((ZLOX_UINT8 *)t, 0, sizeof(ZLOX_USB_TRANSFER));
	t->endp = &dev->endp;
	t->req = ZLOX_NULL;
	t->data = kbd->data;
	t->len = 8;
	t->complete = ZLOX_FALSE;
	t->success = ZLOX_FALSE;

	dev->hcIntr(dev, t);
	return ZLOX_TRUE;
}

