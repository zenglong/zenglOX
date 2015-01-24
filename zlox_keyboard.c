// zlox_keyboard.c implement interrupt and functions of keyboard

#include "zlox_keyboard.h"
#include "zlox_isr.h"
#include "zlox_monitor.h"
#include "zlox_task.h"
#include "zlox_my_windows.h"

#define ZLOX_LED_NUM_LOCK		2
#define ZLOX_LED_SCROLL_LOCK		1
#define ZLOX_LED_CAPS_LOCK		4

#define ZLOX_CK_SHIFT		1
#define	ZLOX_CK_ALT		2
#define ZLOX_CK_CTRL		4

extern ZLOX_TASK * input_focus_task;
extern ZLOX_BOOL ps2_first_port_status;

ZLOX_BOOL fire_key_interrupt = ZLOX_FALSE;

ZLOX_VOID zlox_setleds();

ZLOX_UINT8 led_status = 0;

ZLOX_UINT8 control_keys = 0;

ZLOX_UINT8 press_key = 0;

/* keyboard_buffer stores every key typed */
ZLOX_UINT8 keyboard_buffer[255];
/* keyboard_buffer_size stores the number of keys in the buffer */
ZLOX_UINT8 keyboard_buffer_size = 0;

ZLOX_UINT32 scanToAscii_table[][8] = 
{
/* 	ASCII -	Shift - Ctrl - 	Alt - 	Num - 	Caps - 	Shift Caps - 	Shift Num */
{  	0,	0,	0,	0,	0,	0,	0,		0},
{	0x1B,	0x1B,	0x1B,	0,	0x1B,	0x1B,	0x1B,		0x1B},
/* 1 -> 9 */
{	0x31,	0x21,	0,	0x7800,	0x31,	0x31,	0x21,		0x21},
{	0x32,	0x40,	0x0300,	0x7900,	0x32,	0x32,	0x40,		0x40},
{	0x33,	0x23,	0,	0x7A00, 0x33,	0x33,	0x23,		0x23},
{	0x34,	0x24,	0,	0x7B00, 0x34,	0x34,	0x24,		0x24},
{	0x35,	0x25,	0,	0x7C00,	0x35,	0x35,	0x25,		0x25},
{	0x36,	0x5E,	0x1E,	0x7D00, 0x36,	0x36,	0x5E,		0x5E},
{	0x37,	0x26,	0,	0x7E00,	0x37,	0x37,	0x26,		0x26},
{	0x38,	0x2A,	0,	0x7F00, 0x38,	0x38,	0x2A,		0x2A},
{	0x39,	0x28,	0,	0x8000, 0x39,	0x39,	0x28,		0x28},
{	0x30,	0x29,	0,	0x8100,	0x30,	0x30,	0x29,		0x29},
/* -, =, Bksp, Tab */
{	0x2D,	0x5F,	0x1F,	0x8200,	0x2D,	0x2D,	0x5F,		0x5F},
{	0x3D,	0x2B,	0,	0x8300,	0x3D,	0x3D,	0x2B,		0x2B},
{	0x08,	0x08,	0x7F,	0,	0x08,	0x08,	0x08,		0x08},
{	0x09,	0x0F00,	0,	0,	0x09,	0x09,	0x0F00,		0x0F00},
/*	QWERTYUIOP[] */
{	0x71,	0x51,	0x11,	0x1000,	0x71,	0x51,	0x71,		0x51},
{	0x77,	0x57,	0x17,	0x1100,	0x77,	0x57,	0x77,		0x57},
{	0x65,	0x45,	0x05,	0x1200,	0x65,	0x45,	0x65,		0x45},
{	0x72,	0x52,	0x12,	0x1300,	0x72,	0x52,	0x72,		0x52},
{	0x74,	0x54,	0x14,	0x1400,	0x74,	0x54,	0x74,		0x54},
{	0x79,	0x59,	0x19,	0x1500,	0x79,	0x59,	0x79,		0x59},
{	0x75,	0x55,	0x15,	0x1600,	0x75,	0x55,	0x75,		0x55},
{	0x69,	0x49,	0x09,	0x1700,	0x69,	0x49,	0x69,		0x49},
{	0x6F,	0x4F,	0x0F,	0x1800,	0x6F,	0x4F,	0x6F,		0x4F},
{	0x70,	0x50,	0x10,	0x1900,	0x70,	0x50,	0x70,		0x50},
{	0x5B,	0x7B,	0x1B,	0x0,	0x5B,	0x5B,	0x7B,		0x7B},
{	0x5D,	0x7D,	0x1D,	0,	0x5D,	0x5D,	0x7D,		0x7D},
/* ENTER, CTRL */
{	0x0A,	0x0A,	0x0D,	0,	0x0A,	0x0A,	0x0D,		0x0D},
{	0,	0,	0,	0,	0,	0,	0,		0},
/* ASDFGHJKL;'~ */
{	0x61,	0x41,	0x01,	0x1E00,	0x61,	0x41,	0x61,		0x41},
{	0x73,	0x53,	0x13,	0x1F00,	0x73,	0x53,	0x73,		0x53},
{	0x64,	0x44,	0x04,	0x2000,	0x64,	0x44,	0x64,		0x44},
{	0x66,	0x46,	0x06,	0x2100,	0x66,	0x46,	0x66,		0x46},
{	0x67,	0x47,	0x07,	0x2200,	0x67,	0x47,	0x67,		0x47},
{	0x68,	0x48,	0x08,	0x2300,	0x68,	0x48,	0x68,		0x48},
{	0x6A,	0x4A,	0x0A,	0x2400,	0x6A,	0x4A,	0x6A,		0x4A},
{	0x6B,	0x4B,	0x0B,	0x3500,	0x6B,	0x4B,	0x6B,		0x4B},
{	0x6C,	0x4C,	0x0C,	0x2600,	0x6C,	0x4C,	0x6C,		0x4C},
{	0x3B,	0x3A,	0,	0,	0x3B,	0x3B,	0x3A,		0x3A},
{	0x27,	0x22,	0,	0,	0x27,	0x27,	0x22,		0x22},
{	0x60,	0x7E,	0,	0,	0x60,	0x60,	0x7E,		0x7E},
/* Left Shift*/
{	0x2A,	0,	0,	0,	0,	0,	0,		0},
/* \ZXCVBNM,./ */
{	0x5C,	0x7C,	0x1C,	0,	0x5C,	0x5C,	0x7C,		0x7C},
{	0x7A,	0x5A,	0x1A,	0x2C00,	0x7A,	0x5A,	0x7A,		0x5A},
{	0x78,	0x58,	0x18,	0x2D00,	0x78,	0x58,	0x78,		0x58},
{	0x63,	0x43,	0x03,	0x2E00,	0x63,	0x43,	0x63,		0x43},
{	0x76,	0x56,	0x16,	0x2F00,	0x76,	0x56,	0x76,		0x56},
{	0x62,	0x42,	0x02,	0x3000,	0x62,	0x42,	0x62,		0x42},
{	0x6E,	0x4E,	0x0E,	0x3100,	0x6E,	0x4E,	0x6E,		0x4E},
{	0x6D,	0x4D,	0x0D,	0x3200,	0x6D,	0x4D,	0x6D,		0x4D},
{	0x2C,	0x3C,	0,	0,	0x2C,	0x2C,	0x3C,		0x3C},
{	0x2E,	0x3E,	0,	0,	0x2E,	0x2E,	0x3E,		0x3E},
{	0x2F,	0x3F,	0,	0,	0x2F,	0x2F,	0x3F,		0x3F},
/* Right Shift */
{	0,	0,	0,	0,	0,	0,	0,		0},
/* Print Screen */
{	0,	0,	0,	0,	0,	0,	0,		0},
/* Alt  */
{	0,	0,	0,	0,	0,	0,	0,		0},
/* Space */
{	0x20,	0x20,	0x20,	0,	0x20,	0x20,	0x20,		0x20},
/* Caps */
{	0,	0,	0,	0,	0,	0,	0,		0},
/* F1-F10 */
{	0x3B00,	0x5400,	0x5E00,	0x6800,	0x3B00,	0x3B00,	0x5400,		0x5400},
{	0x3C00,	0x5500,	0x5F00,	0x6900,	0x3C00,	0x3C00,	0x5500,		0x5500},
{	0x3D00,	0x5600,	0x6000,	0x6A00,	0x3D00,	0x3D00,	0x5600,		0x5600},
{	0x3E00,	0x5700,	0x6100,	0x6B00,	0x3E00,	0x3E00,	0x5700,		0x5700},
{	0x3F00,	0x5800,	0x6200,	0x6C00,	0x3F00,	0x3F00,	0x5800,		0x5800},
{	0x4000,	0x5900,	0x6300,	0x6D00,	0x4000,	0x4000,	0x5900,		0x5900},
{	0x4100,	0x5A00,	0x6400,	0x6E00,	0x4100,	0x4100,	0x5A00,		0x5A00},
{	0x4200,	0x5B00,	0x6500,	0x6F00,	0x4200,	0x4200,	0x5B00,		0x5B00},
{	0x4300,	0x5C00,	0x6600,	0x7000,	0x4300,	0x4300,	0x5C00,		0x5C00},
{	0x4400,	0x5D00,	0x6700,	0x7100,	0x4400,	0x4400,	0x5D00,		0x5D00},
/* Num Lock, Scrl Lock */
{	0,	0,	0,	0,	0,	0,	0,		0},
{	0,	0,	0,	0,	0,	0,	0,		0},
/* HOME, Up, Pgup, -kpad, left, center, right, +keypad, end, down, pgdn, ins, del */
{	0x4700,	0x37,	0x7700,	0,	0x37,	0x4700,	0x37,		0x4700},
{	0x4800,	0x38,	0,	0,	0x38,	0x4800,	0x38,		0x4800},
{	0x4900,	0x39,	0x8400,	0,	0x39,	0x4900,	0x39,		0x4900},
{	0x2D,	0x2D,	0,	0,	0x2D,	0x2D,	0x2D,		0x2D},
{	0x4B00,	0x34,	0x7300,	0,	0x34,	0x4B00,	0x34,		0x4B00},
{	0x4C00,	0x35,	0,	0,	0x35,	0x4C00,	0x35,		0x4C00},
{	0x4D00,	0x36,	0x7400,	0,	0x36,	0x4D00,	0x36,		0x4D00},
{	0x2B,	0x2B,	0,	0,	0x2B,	0x2B,	0x2B,		0x2B},
{	0x4F00,	0x31,	0x7500,	0,	0x31,	0x4F00,	0x31,		0x4F00},
{	0x5000,	0x32,	0,	0,	0x32,	0x5000,	0x32,		0x5000},
{	0x5100,	0x33,	0x7600,	0,	0x33,	0x5100,	0x33,		0x5100},
{	0x5200,	0x30,	0,	0,	0x30,	0x5200,	0x30,		0x5200},
{	0x5300,	0x2E,	0,	0,	0x2E,	0x5300,	0x2E,		0x5300}
};

static ZLOX_VOID zlox_keyboard_callback(/*ZLOX_ISR_REGISTERS * regs*/)
{
	ZLOX_UINT32 key = zlox_inb(0x60);
	ZLOX_UINT32 key_ascii = 0;	
	ZLOX_UINT32 key_code = 0;
	ZLOX_UINT32 scanMaxNum = sizeof(scanToAscii_table) / (8 * 4);
	
	//if(!fire_key_interrupt)
		//fire_key_interrupt = ZLOX_TRUE;

	if(press_key == 0xE0)
	{
		switch(key)
		{
		case 0x47:
			key_code = ZLOX_MKK_HOME_PRESS;
			break;
		case 0x48:
			key_code = ZLOX_MKK_CURSOR_UP_PRESS;
			break;
		case 0x49:
			key_code = ZLOX_MKK_PAGE_UP_PRESS;
			break;
		case 0x4B:
			key_code = ZLOX_MKK_CURSOR_LEFT_PRESS;
			break;
		case 0x4D:
			key_code = ZLOX_MKK_CURSOR_RIGHT_PRESS;
			break;
		case 0x4F:
			key_code = ZLOX_MKK_END_PRESS;
			break;
		case 0x50:
			key_code = ZLOX_MKK_CURSOR_DOWN_PRESS;
			break;
		case 0x51:
			key_code = ZLOX_MKK_PAGE_DOWN_PRESS;
			break;
		case 0x52:
			key_code = ZLOX_MKK_INSERT_PRESS;
			break;
		case 0x53:
			key_code = ZLOX_MKK_DELETE_PRESS;
			break;
		default:
			key_code = 0;
			break;
		}
		press_key = 0;
	}
	else if(key == 0xE0)
	{
		press_key = key;
	}

	/* 'LED Keys', ie, Scroll lock, Num lock, and Caps lock */
	if(key == 0x3A)	/* Caps Lock */
	{
		led_status ^= ZLOX_LED_CAPS_LOCK;
		zlox_setleds();
	}
	if(key == 0x45)	/* Num Lock */
	{
		led_status ^= ZLOX_LED_NUM_LOCK;
		zlox_setleds();
	}
	if(key == 0x46) /* Scroll Lock */
	{
		led_status ^= ZLOX_LED_SCROLL_LOCK;
		zlox_setleds();
	}

	if(key == 0x1D && !(control_keys & ZLOX_CK_CTRL))	/* Ctrl key */
		control_keys |= ZLOX_CK_CTRL;
	if(key == 0x80 + 0x1D)	/* Ctrl key depressed */
		control_keys &= (0xFF - ZLOX_CK_CTRL);
	if((key == 0x2A || key == 0x36) && !(control_keys & ZLOX_CK_SHIFT))	/* Shift key */
		control_keys |= ZLOX_CK_SHIFT;
	if((key == 0x80 + 0x2A) || (key == 0x80 + 0x36))	/* Shift key depressed */
		control_keys &= (0xFF - ZLOX_CK_SHIFT);
	if(key == 0x38 && !(control_keys & ZLOX_CK_ALT))
		control_keys |= ZLOX_CK_ALT;
	if(key == 0x80 + 0x38)
		control_keys &= (0xFF - ZLOX_CK_ALT);

	if(control_keys & ZLOX_CK_SHIFT)
	{
		if(led_status & ZLOX_LED_CAPS_LOCK)
		{
			key_ascii = key < scanMaxNum ? scanToAscii_table[key][6] : 0; 
			if((key_ascii == 0 || key_ascii > 0xFF) && (led_status & ZLOX_LED_NUM_LOCK))
				key_ascii = key < scanMaxNum ? scanToAscii_table[key][7] : 0;
		}
		else if(led_status & ZLOX_LED_NUM_LOCK)
			key_ascii = key < scanMaxNum ? scanToAscii_table[key][7] : 0;
		else
			key_ascii = key < scanMaxNum ? scanToAscii_table[key][1] : 0;
	}
	else if(led_status & ZLOX_LED_CAPS_LOCK)
	{
		key_ascii = key < scanMaxNum ? scanToAscii_table[key][5] : 0;
		if((key_ascii == 0 || key_ascii > 0xFF) && (led_status & ZLOX_LED_NUM_LOCK))
			key_ascii = key < scanMaxNum ? scanToAscii_table[key][4] : 0;
	}
	else if(led_status & ZLOX_LED_NUM_LOCK) 
		key_ascii = key < scanMaxNum ? scanToAscii_table[key][4] : 0;
	else 
		key_ascii = key < scanMaxNum ? scanToAscii_table[key][0] : 0;

	if(key_code != 0)
	{
		ZLOX_TASK_MSG ascii_msg = {0};
		ascii_msg.type = ZLOX_MT_KEYBOARD;
		ascii_msg.keyboard.type = ZLOX_MKT_KEY;
		ascii_msg.keyboard.key = key_code;
		//zlox_send_tskmsg(input_focus_task,&ascii_msg);
		zlox_update_for_mykbd(&ascii_msg);
	}
	else if(key_ascii != 0)
	{
		if(key_ascii <= 0xFF)
		{
			ZLOX_TASK_MSG ascii_msg = {0};
			ascii_msg.type = ZLOX_MT_KEYBOARD;
			ascii_msg.keyboard.type = ZLOX_MKT_ASCII;
			ascii_msg.keyboard.ascii = key_ascii;
			//zlox_send_tskmsg(input_focus_task,&ascii_msg);
			zlox_update_for_mykbd(&ascii_msg);
		}
		else
		{
			ZLOX_TASK_MSG ascii_msg = {0};
			switch(key_ascii)
			{
			case ZLOX_MKK_F1_PRESS:
			case ZLOX_MKK_F2_PRESS:
			case ZLOX_MKK_F3_PRESS:
			case ZLOX_MKK_F4_PRESS:
			case ZLOX_MKK_F5_PRESS:
			case ZLOX_MKK_F6_PRESS:
			case ZLOX_MKK_F7_PRESS:
			case ZLOX_MKK_F8_PRESS:
			case ZLOX_MKK_F9_PRESS:
			case ZLOX_MKK_F10_PRESS:
				ascii_msg.type = ZLOX_MT_KEYBOARD;
				ascii_msg.keyboard.type = ZLOX_MKT_KEY;
				ascii_msg.keyboard.key = key_ascii;
				//zlox_send_tskmsg(input_focus_task,&ascii_msg);
				zlox_update_for_mykbd(&ascii_msg);
				break;
			}
		}
	}
}

ZLOX_VOID zlox_setleds()
{
	zlox_outb(0x60, 0xED);
	while(zlox_inb(0x64) & 2);
	zlox_outb(0x60, led_status);
	while(zlox_inb(0x64) & 2);
}

ZLOX_BOOL zlox_initKeyboard()
{
	if(ps2_first_port_status == ZLOX_TRUE)
	{
		zlox_register_interrupt_callback(ZLOX_IRQ1,&zlox_keyboard_callback);

		led_status = 0; /* All leds off */
		zlox_setleds();
		return ZLOX_TRUE;
	}
	return ZLOX_FALSE;
}

ZLOX_UINT8 zlox_get_control_keys()
{
	return control_keys;
}

ZLOX_UINT8 zlox_release_control_keys(ZLOX_UINT8 key)
{
	switch(key)
	{
	case ZLOX_CK_SHIFT:
		control_keys &= (0xFF - ZLOX_CK_SHIFT);
		break;
	case ZLOX_CK_ALT:
		control_keys &= (0xFF - ZLOX_CK_ALT);
		break;
	case ZLOX_CK_CTRL:
		control_keys &= (0xFF - ZLOX_CK_CTRL);
		break;
	}
	return control_keys;
}

