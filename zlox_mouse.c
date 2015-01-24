// zlox_mouse.c -- something relate to the mouse driver

#include "zlox_mouse.h"
#include "zlox_isr.h"
#include "zlox_task.h"
#include "zlox_my_windows.h"
#include "zlox_ata.h"

#define MOUSE_PORT   0x60
#define MOUSE_STATUS 0x64
#define MOUSE_ABIT   0x02
#define MOUSE_BBIT   0x01
#define MOUSE_WRITE  0xD4
#define MOUSE_F_BIT  0x20
#define MOUSE_V_BIT  0x08

// zlox_my_windows.c
extern ZLOX_MY_WINDOW * mywin_list_header;

ZLOX_UINT8 mouse_cycle=0; //unsigned char
ZLOX_UINT8 mouse_byte[3]; //signed char

ZLOX_FLOAT mouse_scale_factor = 1.0;

//Mouse functions
ZLOX_VOID zlox_mouse_handler(/*ZLOX_ISR_REGISTERS * regs*/)
{
	ZLOX_UINT8 status = zlox_inb(MOUSE_STATUS);
	while (status & MOUSE_BBIT) {
		ZLOX_UINT8 mouse_in = zlox_inb(MOUSE_PORT);
		if (status & MOUSE_F_BIT) {
			switch(mouse_cycle)
			{
			case 0:
				mouse_byte[0] = mouse_in;
				if (!(mouse_in & MOUSE_V_BIT)) return;
				mouse_cycle++;
				break;
			case 1:
				mouse_byte[1] = mouse_in;
				mouse_cycle++;
				break;
			case 2:
				mouse_byte[2] = mouse_in;
				/* We now have a full mouse packet ready to use */
				if (mouse_byte[0] & 0x80 || mouse_byte[0] & 0x40) {
					/* x/y overflow? bad packet! */
					break;
				}
				ZLOX_TASK_MSG msg = {0};
				msg.type = ZLOX_MT_MOUSE;
				msg.mouse.state = mouse_byte[0];
				if((msg.mouse.state & 0x10) != 0)
					msg.mouse.rel_x = (ZLOX_SINT32)((ZLOX_UINT32)(mouse_byte[1] & 0x0FF) | (0xFFFFFF00));
				else
					msg.mouse.rel_x = (ZLOX_SINT32)((ZLOX_UINT32)(mouse_byte[1] & 0x0FF));
				if((msg.mouse.state & 0x20) != 0)
					msg.mouse.rel_y = (ZLOX_SINT32)((ZLOX_UINT32)(mouse_byte[2] & 0x0FF) | (0xFFFFFF00));
				else
					msg.mouse.rel_y = (ZLOX_SINT32)((ZLOX_UINT32)(mouse_byte[2] & 0x0FF));
				if(mouse_scale_factor > 1.0)
				{
					if(msg.mouse.rel_x > 10 || msg.mouse.rel_x < -10)
						msg.mouse.rel_x = msg.mouse.rel_x * mouse_scale_factor;
					if(msg.mouse.rel_y > 10 || msg.mouse.rel_y < -10)
						msg.mouse.rel_y = msg.mouse.rel_y * mouse_scale_factor;
				}
				if(mywin_list_header != ZLOX_NULL)
				{
					zlox_update_for_mymouse(&msg);
				}
				mouse_cycle = 0;
				break;
			default:
				break;
			}
		}
		status = zlox_inb(MOUSE_STATUS);
	}
}

ZLOX_VOID zlox_mouse_wait(ZLOX_UINT8 a_type) //unsigned char
{
	ZLOX_UINT32 _time_out=100000; //unsigned int
	if(a_type==0)
	{
		while(_time_out--) //Data
		{
			if((zlox_inb(0x64) & 1)==1)
			{
				return;
			}
		}
		return;
	}
	else
	{
		while(_time_out--) //Signal
		{
			if((zlox_inb(0x64) & 2)==0)
			{
				return;
			}
		}
		return;
	}
}

ZLOX_VOID zlox_mouse_write(ZLOX_UINT8 a_write) //unsigned char
{
	//Wait to be able to send a command
	zlox_mouse_wait(1);
	//Tell the mouse we are sending a command
	zlox_outb(0x64, 0xD4);
	//Wait for the final part
	zlox_mouse_wait(1);
	//Finally write
	zlox_outb(0x60, a_write);
}

ZLOX_UINT8 zlox_mouse_read()
{
	//Get's response from mouse
	zlox_mouse_wait(0);
	return zlox_inb(0x60);
}

ZLOX_VOID zlox_mouse_install()
{
	ZLOX_UINT8 _status;	//unsigned char

	//Enable the auxiliary mouse device
	zlox_mouse_wait(1);
	zlox_outb(0x64, 0xA8);
 
	//Enable the interrupts
	zlox_mouse_wait(1);
	zlox_outb(0x64, 0x20);
	zlox_mouse_wait(0);
	_status=(zlox_inb(0x60) | 2);
	zlox_mouse_wait(1);
	zlox_outb(0x64, 0x60);
	zlox_mouse_wait(1);
	zlox_outb(0x60, _status);
 
	//Tell the mouse to use default settings
	zlox_mouse_write(0xF6);
	zlox_mouse_read();	//Acknowledge

	//Enable Packet Streaming
	zlox_mouse_write(0xF4);
	zlox_mouse_read();	//Acknowledge

	//Setup the mouse handler
	zlox_register_interrupt_callback(ZLOX_IRQ12, zlox_mouse_handler);
}

ZLOX_SINT32 zlox_mouse_set_scale(ZLOX_FLOAT * scale)
{
	if((*scale) >= 1.0 && (*scale) <= 4.0)
	{
		mouse_scale_factor = (*scale);
		return 0;
	}
	return -1;
}

