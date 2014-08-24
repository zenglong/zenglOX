// zlox_ps2.c -- 和ps2控制器相关的函数定义

#include "zlox_ps2.h"
#include "zlox_monitor.h"

ZLOX_BOOL ps2_has_secport = ZLOX_TRUE;
ZLOX_BOOL ps2_first_port_status = ZLOX_FALSE;
ZLOX_BOOL ps2_sec_port_status = ZLOX_FALSE;
ZLOX_BOOL ps2_init_status = ZLOX_FALSE;

#define ZLOX_PIC1_CMD                    0x20
#define ZLOX_PIC1_DATA                   0x21
#define ZLOX_PIC2_CMD                    0xA0
#define ZLOX_PIC2_DATA                   0xA1
#define ZLOX_PIC_READ_IRR                0x0a    /* OCW3 irq ready next CMD read */
#define ZLOX_PIC_READ_ISR                0x0b    /* OCW3 irq service next CMD read */
 
/* Helper func */
static ZLOX_UINT16 zlox_pic_get_irq_reg(ZLOX_UINT32 ocw3)
{
    /* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
     * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain */
    zlox_outb(ZLOX_PIC1_CMD, ocw3);
    zlox_outb(ZLOX_PIC2_CMD, ocw3);
    return (zlox_inb(ZLOX_PIC2_CMD) << 8) | zlox_inb(ZLOX_PIC1_CMD);
}
 
/* Returns the combined value of the cascaded PICs irq request register */
ZLOX_UINT16 zlox_pic_get_irr(ZLOX_VOID)
{
    return zlox_pic_get_irq_reg(ZLOX_PIC_READ_IRR);
}
 
/* Returns the combined value of the cascaded PICs in-service register */
ZLOX_UINT16 zlox_pic_get_isr(ZLOX_VOID)
{
    return zlox_pic_get_irq_reg(ZLOX_PIC_READ_ISR);
}

ZLOX_VOID zlox_test_ps2_keyboard() // debug function
{
	while(zlox_inb(0x64) & 0x2);
	zlox_outb(0x64, 0x20);
	while(!(zlox_inb(0x64) & 0x1));
	ZLOX_UINT8 config_byte = zlox_inb(0x60);
	ZLOX_UINT8 mask1 = zlox_inb(0x21);
	ZLOX_UINT8 mask2 = zlox_inb(0xA1);
	ZLOX_UINT16 irr = zlox_pic_get_irr();
	ZLOX_UINT16 isr = zlox_pic_get_isr();
	ZLOX_UNUSED(config_byte);
	ZLOX_UNUSED(mask1);
	ZLOX_UNUSED(mask2);
	ZLOX_UNUSED(irr);
	ZLOX_UNUSED(isr);
}

ZLOX_BOOL zlox_ps2_init(ZLOX_BOOL need_openMouse)
{
	// Disable Devices 
	while(zlox_inb(0x64) & 0x2);
	zlox_outb(0x64, 0xAD);
	while(zlox_inb(0x64) & 0x2);
	zlox_outb(0x64, 0xA7);
	// Flush The Output Buffer 
	while(zlox_inb(0x64) & 0x1)
		zlox_inb(0x60);
	// Set the Controller Configuration Byte 
	while(zlox_inb(0x64) & 0x2);
	zlox_outb(0x64, 0x20);
	while(!(zlox_inb(0x64) & 0x1));
	ZLOX_UINT8 config_byte = zlox_inb(0x60);
	while(zlox_inb(0x64) & 0x2);
	zlox_outb(0x64, 0x60);
	while(zlox_inb(0x64) & 0x2);
	zlox_outb(0x60, config_byte & (~0x43));
	if((config_byte & 0x20) == 0)
		ps2_has_secport = ZLOX_FALSE;
	else
		ps2_has_secport = ZLOX_TRUE;
	// Perform Controller Self Test 
	while(zlox_inb(0x64) & 0x2);
	zlox_outb(0x64, 0xAA);
	while(!(zlox_inb(0x64) & 0x1));
	ZLOX_UINT8 response = zlox_inb(0x60);
	if(response != 0x55)
	{
		zlox_monitor_write("PS/2 Controller self test failed ! , you can't use keyboard and mouse... \n");
		return ps2_init_status;
	}
	// Determine If There Are 2 Channels 
	if(ps2_has_secport == ZLOX_TRUE)
	{
		while(zlox_inb(0x64) & 0x2);
		zlox_outb(0x64, 0xA8);
		while(zlox_inb(0x64) & 0x2);
		zlox_outb(0x64, 0x20);
		while(!(zlox_inb(0x64) & 0x1));
		config_byte = zlox_inb(0x60);
		if((config_byte & 0x20) != 0)
		{
			zlox_monitor_write("PS/2 Controller is single channel !\n");
			ps2_has_secport = ZLOX_FALSE;
		}
		else
			ps2_has_secport = ZLOX_TRUE;
		while(zlox_inb(0x64) & 0x2);
		zlox_outb(0x64, 0xA7);
	}
	// Perform Interface Tests 
	while(zlox_inb(0x64) & 0x2);
	zlox_outb(0x64, 0xAB);
	while(!(zlox_inb(0x64) & 0x1));
	response = zlox_inb(0x60);
	if(response != 0x00)
	{
		zlox_monitor_write("first PS/2 port test failed, you can't use keyboard");
	}
	else
		ps2_first_port_status = ZLOX_TRUE;
	if(ps2_has_secport == ZLOX_TRUE)
	{
		while(zlox_inb(0x64) & 0x2);
		zlox_outb(0x64, 0xA9);
		while(!(zlox_inb(0x64) & 0x1));
		response = zlox_inb(0x60);
		if(response != 0x00)
		{
			zlox_monitor_write("second PS/2 port test failed, you can't use mouse");
		}
		else
			ps2_sec_port_status = ZLOX_TRUE;
	}
	if(ps2_first_port_status == ZLOX_FALSE && 
		ps2_sec_port_status == ZLOX_FALSE)
	{
		return ps2_init_status;
	}
	// Enable Devices 
	if(ps2_first_port_status == ZLOX_TRUE)
	{
		while(zlox_inb(0x64) & 0x2);
		zlox_outb(0x64, 0xAE);
	}
	if(need_openMouse == ZLOX_TRUE && ps2_sec_port_status == ZLOX_TRUE)
	{
		while(zlox_inb(0x64) & 0x2);
		zlox_outb(0x64, 0xA8);
	}
	while(zlox_inb(0x64) & 0x2);
	zlox_outb(0x64, 0x20);
	while(!(zlox_inb(0x64) & 0x1));
	config_byte = zlox_inb(0x60);
	if(ps2_first_port_status == ZLOX_TRUE)
		config_byte = config_byte | 0x41;
	if(need_openMouse == ZLOX_TRUE && ps2_sec_port_status == ZLOX_TRUE)
		config_byte = config_byte | 0x2;
	while(zlox_inb(0x64) & 0x2);
	zlox_outb(0x64, 0x60);
	while(zlox_inb(0x64) & 0x2);
	zlox_outb(0x60, config_byte);
	// Reset Devices 
	if(ps2_first_port_status == ZLOX_TRUE)
	{
		while(zlox_inb(0x64) & 0x2);
		zlox_outb(0x60, 0xFF);
		while(!(zlox_inb(0x64) & 0x1));
		response = zlox_inb(0x60);
		if(response != 0xAA && response != 0xFA)
		{
			zlox_monitor_write("first PS/2 device reset failed, you can't use keyboard");
			ps2_first_port_status = ZLOX_FALSE;
		}
	}
	if(need_openMouse == ZLOX_TRUE && ps2_sec_port_status == ZLOX_TRUE)
	{
		while(zlox_inb(0x64) & 0x2);
		zlox_outb(0x64, 0xD4);
		while(zlox_inb(0x64) & 0x2);
		zlox_outb(0x60, 0xFF);
		while(!(zlox_inb(0x64) & 0x1));
		response = zlox_inb(0x60);
		if(response != 0xAA && response != 0xFA)
		{
			zlox_monitor_write("second PS/2 device reset failed, you can't use mouse");
			ps2_first_port_status = ZLOX_FALSE;
		}
	}
	ps2_init_status = ZLOX_TRUE;
	return ps2_init_status;
}

