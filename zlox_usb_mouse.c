// zlox_usb_mouse.c -- the C file relate to USB mouse

#include "zlox_usb_mouse.h"
#include "zlox_monitor.h"
#include "zlox_task.h"
#include "zlox_my_windows.h"

typedef struct _ZLOX_USB_MOUSE
{
	ZLOX_USB_TRANSFER dataTransfer;
	ZLOX_UINT8 data[4];
} ZLOX_USB_MOUSE;

// zlox_my_windows.c
extern ZLOX_MY_WINDOW * mywin_list_header;
extern ZLOX_FLOAT mouse_scale_factor;

static ZLOX_VOID zlox_usb_mouse_process(ZLOX_USB_MOUSE * mouse)
{
	ZLOX_UINT8 * data = mouse->data;

	/*zlox_monitor_write("mouse in: ");
	zlox_monitor_write_hex(data[0]);
	zlox_monitor_write(" dx=");
	zlox_monitor_write_dec((ZLOX_SINT8)data[1]);
	zlox_monitor_write(" dy=");
	zlox_monitor_write_dec((ZLOX_SINT8)data[2]);
	zlox_monitor_write("\n");*/

	ZLOX_TASK_MSG msg = {0};
	msg.type = ZLOX_MT_MOUSE;
	msg.mouse.state = data[0];
	msg.mouse.rel_x = (ZLOX_SINT32)((ZLOX_SINT8)data[1]);
	msg.mouse.rel_y = (ZLOX_SINT32)((ZLOX_SINT8)data[2]);
	msg.mouse.rel_y = -(msg.mouse.rel_y);

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
}

static ZLOX_VOID zlox_usb_mouse_poll(ZLOX_USB_DEVICE * dev)
{
	ZLOX_USB_MOUSE * mouse = dev->drv;
	ZLOX_USB_TRANSFER * t = &mouse->dataTransfer;

	if (t->complete)
	{
		if (t->success)
		{
			zlox_usb_mouse_process(mouse);
		}

		t->complete = ZLOX_FALSE;
		dev->hcIntr(dev, t);
	}
}

ZLOX_BOOL zlox_usb_mouse_init(ZLOX_USB_DEVICE * dev)
{
	if (dev->intfDesc.intfClass == ZLOX_USB_CLASS_HID &&
		dev->intfDesc.intfSubClass == ZLOX_USB_SUBCLASS_BOOT &&
		dev->intfDesc.intfProtocol == ZLOX_USB_PROTOCOL_MOUSE)
	{
		zlox_monitor_write("Initializing Mouse\n");

		ZLOX_USB_MOUSE * mouse = zlox_usb_get_cont_phys(sizeof(ZLOX_USB_MOUSE));

		dev->drv = mouse;
		dev->drvPoll = zlox_usb_mouse_poll;

		// Prepare transfer
		ZLOX_USB_TRANSFER * t = &mouse->dataTransfer;
		zlox_memset((ZLOX_UINT8 *)t, 0, sizeof(ZLOX_USB_TRANSFER));
		t->endp = &dev->endp;
		t->req = ZLOX_NULL;
		t->data = mouse->data;
		t->len = 4;
		t->complete = ZLOX_FALSE;
		t->success = ZLOX_FALSE;

		dev->hcIntr(dev, t);
		return ZLOX_TRUE;
	}

	return ZLOX_FALSE;
}

