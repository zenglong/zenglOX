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
 
ZLOX_VOID zlox_ps2_wait(ZLOX_UINT8 a_type);
ZLOX_VOID zlox_ps2_flush_output_buffer();

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
	zlox_ps2_flush_output_buffer();
	zlox_ps2_wait(1);
	zlox_outb(0x64, 0x20);
	zlox_ps2_wait(0);
	ZLOX_UINT8 config_byte = zlox_inb(0x60); // in VMware maybe 0xfa or 0x47, other emulator may be 0x43
	//ZLOX_UNUSED(config_byte);
	zlox_monitor_write("[debug info] config_byte: ");
	zlox_monitor_write_hex(config_byte);
	zlox_monitor_write("\n");
}

ZLOX_VOID zlox_ps2_wait(ZLOX_UINT8 a_type) //unsigned char
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

ZLOX_VOID zlox_ps2_flush_output_buffer()
{
	// Flush The Output Buffer 
	while(zlox_inb(0x64) & 0x1)
		zlox_inb(0x60);
}

ZLOX_BOOL zlox_ps2_init()
{
	ZLOX_UINT8 config_byte = 0x47;	//unsigned char
	while(zlox_inb(0x64) & 0x2);
	zlox_outb(0x64, 0x60);
	while(zlox_inb(0x64) & 0x2);
	zlox_outb(0x60, config_byte);

	ps2_has_secport = ZLOX_TRUE;
	ps2_first_port_status = ZLOX_TRUE;
	ps2_sec_port_status = ZLOX_TRUE;
	ps2_init_status = ZLOX_TRUE;
	return ZLOX_TRUE;
}

// 用于系统调用, 获取PS2的初始化状态
ZLOX_SINT32 zlox_ps2_get_status(ZLOX_BOOL * init_status, ZLOX_BOOL * first_port_status, ZLOX_BOOL * sec_port_status)
{
	*init_status = ps2_init_status;
	*first_port_status = ps2_first_port_status;
	*sec_port_status = ps2_sec_port_status;
	return 0;
}

