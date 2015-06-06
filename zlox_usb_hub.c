// zlox_usb_hub.c -- the C file relate to USB HUB

#include "zlox_usb_hub.h"
#include "zlox_monitor.h"
#include "zlox_time.h"

// Port Status
#define ZLOX_HUB_PORT_CONNECTION                 (1 << 0)    // Current Connect Status
#define ZLOX_HUB_PORT_ENABLE                     (1 << 1)    // Port Enabled
#define ZLOX_HUB_PORT_SUSPEND                    (1 << 2)    // Suspend
#define ZLOX_HUB_PORT_OVER_CURRENT               (1 << 3)    // Over-current
#define ZLOX_HUB_PORT_RESET                      (1 << 4)    // Port Reset
#define ZLOX_HUB_PORT_POWER                      (1 << 8)    // Port Power
#define ZLOX_HUB_PORT_SPEED_MASK                 (3 << 9)    // Port Speed
#define ZLOX_HUB_PORT_SPEED_SHIFT                9
#define ZLOX_HUB_PORT_TEST                       (1 << 11)   // Port Test Control
#define ZLOX_HUB_PORT_INDICATOR                  (1 << 12)   // Port Indicator Control
#define ZLOX_HUB_PORT_CONNECTION_CHANGE          (1 << 16)   // Connect Status Change
#define ZLOX_HUB_PORT_ENABLE_CHANGE              (1 << 17)   // Port Enable Change
#define ZLOX_HUB_PORT_OVER_CURRENT_CHANGE        (1 << 19)   // Over-current Change

// Hub Characteristics
#define ZLOX_HUB_POWER_MASK                  0x03        // Logical Power Switching Mode
#define ZLOX_HUB_POWER_GLOBAL                0x00
#define ZLOX_HUB_POWER_INDIVIDUAL            0x01
#define ZLOX_HUB_COMPOUND                    0x04        // Part of a Compound Device
#define ZLOX_HUB_CURRENT_MASK                0x18        // Over-current Protection Mode
#define ZLOX_HUB_TT_TTI_MASK                 0x60        // TT Think Time
#define ZLOX_HUB_PORT_INDICATORS             0x80        // Port Indicators

typedef struct _ZLOX_USB_HUB
{
	ZLOX_USB_DEVICE * dev;
	ZLOX_USB_HUB_DESC desc;
} ZLOX_USB_HUB;

ZLOX_VOID zlox_usb_print_hub_desc(ZLOX_USB_HUB_DESC * desc)
{
	zlox_monitor_write(" Hub: port count=");
	zlox_monitor_write_hex(desc->portCount);
	zlox_monitor_write(" characteristics=");
	zlox_monitor_write_hex(desc->chars);
	zlox_monitor_write(" power time=");
	zlox_monitor_write_dec(desc->portPowerTime);
	zlox_monitor_write(" current=");
	zlox_monitor_write_dec(desc->current);
	zlox_monitor_write("\n");
}

static ZLOX_UINT32 zlox_usb_hub_reset_port(ZLOX_USB_HUB * hub, ZLOX_UINT32 port)
{
	ZLOX_USB_DEVICE *dev = hub->dev;

	// Reset the port
	if (!zlox_usb_dev_request(dev,
		ZLOX_USB_RT_HOST_TO_DEV | ZLOX_USB_RT_CLASS | ZLOX_USB_RT_OTHER,
		ZLOX_USB_REQ_SET_FEATURE, ZLOX_USB_F_PORT_RESET, port + 1,
		0, 0))
	{
		return 0;
	}

	// Wait 100ms for port to enable (TODO - remove after dynamic port detection)
	ZLOX_UINT32 * status = (ZLOX_UINT32 *)zlox_usb_get_cont_phys(sizeof(ZLOX_UINT32));
	ZLOX_UINT32 ret = 0;
	if(status == ZLOX_NULL)
		return 0;
	for (ZLOX_UINT32 i = 0; i < 5; ++i)
	{
		// Delay
		zlox_timer_sleep(1, ZLOX_FALSE);

		// Get current status
		if (!zlox_usb_dev_request(dev,
			ZLOX_USB_RT_DEV_TO_HOST | ZLOX_USB_RT_CLASS | ZLOX_USB_RT_OTHER,
			ZLOX_USB_REQ_GET_STATUS, 0, port + 1,
			sizeof(ZLOX_UINT32), status))
		{
			if(status != ZLOX_NULL)
				zlox_usb_free_cont_phys(status);
			return ret;
		}

		ret = (*status);

		// Check if device is attached to port
		if (~ret & ZLOX_HUB_PORT_CONNECTION)
		{
			break;
		}

		// Check if device is enabled
		if (ret & ZLOX_HUB_PORT_ENABLE)
		{
			break;
		}
	}

	if(status != ZLOX_NULL)
		zlox_usb_free_cont_phys(status);
	return ret;
}

static ZLOX_VOID zlox_usb_hub_probe(ZLOX_USB_HUB * hub)
{
	ZLOX_USB_DEVICE * dev = hub->dev;
	ZLOX_UINT32 portCount = hub->desc.portCount;
	ZLOX_UINT32 time_sleep_ticks = 0;

	// Enable power if needed
	if ((hub->desc.chars & ZLOX_HUB_POWER_MASK) == ZLOX_HUB_POWER_INDIVIDUAL)
	{
		for (ZLOX_UINT32 port = 0; port < portCount; ++port)
		{
			if (!zlox_usb_dev_request(dev,
				ZLOX_USB_RT_HOST_TO_DEV | 
				ZLOX_USB_RT_CLASS | 
				ZLOX_USB_RT_OTHER,
				ZLOX_USB_REQ_SET_FEATURE, 
				ZLOX_USB_F_PORT_POWER, port + 1,
				0, 0))
			{
				return;
			}
		}

		time_sleep_ticks = hub->desc.portPowerTime * 2;
		time_sleep_ticks = time_sleep_ticks / 20;
		time_sleep_ticks += (time_sleep_ticks % 20) ? 1 : 0;
		zlox_timer_sleep(time_sleep_ticks, ZLOX_FALSE);
	}

	// Reset ports
	for (ZLOX_UINT32 port = 0; port < portCount; ++port)
	{
		ZLOX_UINT32 status = zlox_usb_hub_reset_port(hub, port);

		if (~status & ZLOX_HUB_PORT_ENABLE)
			continue;

		ZLOX_UINT32 speed = (status & ZLOX_HUB_PORT_SPEED_MASK) >> 
					ZLOX_HUB_PORT_SPEED_SHIFT;

		ZLOX_USB_DEVICE * dev = zlox_usb_dev_create();
		if(dev == ZLOX_NULL)
			continue;

		dev->parent = hub->dev;
		dev->hcd = hub->dev->hcd;
		dev->port = port;
		dev->speed = speed;
		dev->maxPacketSize = 8;

		dev->hcControl = hub->dev->hcControl;
		dev->hcIntr = hub->dev->hcIntr;

		if (!zlox_usb_dev_init(dev))
		{
			// TODO - cleanup
		}
	}
}

ZLOX_BOOL zlox_usb_hub_init(ZLOX_USB_DEVICE * dev)
{
	if (dev->intfDesc.intfClass != ZLOX_USB_CLASS_HUB)
		return ZLOX_FALSE;

	zlox_monitor_write("Initializing Hub\n");

	ZLOX_BOOL ret = ZLOX_FALSE;
	ZLOX_USB_HUB * hub = ZLOX_NULL;
	hub = (ZLOX_USB_HUB *)zlox_usb_get_cont_phys(sizeof(ZLOX_USB_HUB));

	if (!zlox_usb_dev_request(dev,
		ZLOX_USB_RT_DEV_TO_HOST | ZLOX_USB_RT_CLASS | ZLOX_USB_RT_DEV,
		ZLOX_USB_REQ_GET_DESC, (ZLOX_USB_DESC_TYPE_HUB << 8) | 0, 0,
		sizeof(ZLOX_USB_HUB_DESC), &(hub->desc)))
	{
		goto failed;
	}

	zlox_usb_print_hub_desc(&(hub->desc));

	hub->dev = dev;

	dev->drv = hub;
	dev->drvPoll = ZLOX_NULL;

	zlox_usb_hub_probe(hub);

	ret = ZLOX_TRUE;

failed:
	if(hub != ZLOX_NULL)
	{
		zlox_usb_free_cont_phys((ZLOX_VOID *)hub);
	}
	return ret;
}

