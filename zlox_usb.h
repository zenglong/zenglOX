// zlox_usb.h -- the header file relate to USB

#ifndef _ZLOX_USB_H_
#define _ZLOX_USB_H_

#include "zlox_common.h"
#include "zlox_pci.h"

// USB Limits
#define ZLOX_USB_STRING_SIZE                 127
#define ZLOX_USB_CONF_BUF_SIZE               256

// USB Speeds
#define ZLOX_USB_FULL_SPEED 0x00
#define ZLOX_USB_LOW_SPEED 0x01
#define ZLOX_USB_HIGH_SPEED 0x02

// USB Request Type
// USB Revision 1.1 spec --- 9.3 USB Device Requests
// <Table 9-2. Format of Setup Data> Offset 0 Field <bmRequestType> 
// at page 183 (PDF top page is 199)
#define ZLOX_USB_RT_TRANSFER_MASK                0x80
#define ZLOX_USB_RT_DEV_TO_HOST                  0x80
#define ZLOX_USB_RT_HOST_TO_DEV                  0x00

#define ZLOX_USB_RT_TYPE_MASK                    0x60
#define ZLOX_USB_RT_STANDARD                     0x00
#define ZLOX_USB_RT_CLASS                        0x20
#define ZLOX_USB_RT_VENDOR                       0x40

#define ZLOX_USB_RT_RECIPIENT_MASK               0x1f
#define ZLOX_USB_RT_DEV                          0x00
#define ZLOX_USB_RT_INTF                         0x01
#define ZLOX_USB_RT_ENDP                         0x02
#define ZLOX_USB_RT_OTHER                        0x03
// USB Request Type end

// USB Device Requests
// USB Revision 1.1 spec --- Table 9-4. Standard Request Codes
// at page 187 (PDF top page is 203)
#define ZLOX_USB_REQ_GET_STATUS                  0x00
#define ZLOX_USB_REQ_CLEAR_FEATURE               0x01
#define ZLOX_USB_REQ_SET_FEATURE                 0x03
#define ZLOX_USB_REQ_SET_ADDR                    0x05
#define ZLOX_USB_REQ_GET_DESC                    0x06
#define ZLOX_USB_REQ_SET_DESC                    0x07
#define ZLOX_USB_REQ_GET_CONF                    0x08
#define ZLOX_USB_REQ_SET_CONF                    0x09
#define ZLOX_USB_REQ_GET_INTF                    0x0a
#define ZLOX_USB_REQ_SET_INTF                    0x0b
#define ZLOX_USB_REQ_SYNC_FRAME                  0x0c
// USB Device Requests end

// USB HID Interface Requests
#define ZLOX_USB_REQ_GET_REPORT                  0x01
#define ZLOX_USB_REQ_GET_IDLE                    0x02
#define ZLOX_USB_REQ_GET_PROTOCOL                0x03
#define ZLOX_USB_REQ_SET_REPORT                  0x09
#define ZLOX_USB_REQ_SET_IDLE                    0x0a
#define ZLOX_USB_REQ_SET_PROTOCOL                0x0b

// USB Base Descriptor Types
// USB Revision 1.1 spec --- Table 9-5. Descriptor Types
// at page 187 (PDF top page is 203)
#define ZLOX_USB_DESC_TYPE_DEVICE                 0x01
#define ZLOX_USB_DESC_TYPE_CONF                   0x02
#define ZLOX_USB_DESC_TYPE_STRING                 0x03
#define ZLOX_USB_DESC_TYPE_INTF                   0x04
#define ZLOX_USB_DESC_TYPE_ENDP                   0x05
// USB Base Descriptor Types end

// USB HUB Descriptor Types
#define ZLOX_USB_DESC_TYPE_HUB                    0x29

// USB Class Codes
#define ZLOX_USB_CLASS_INTF                  0x00
#define ZLOX_USB_CLASS_AUDIO                 0x01
#define ZLOX_USB_CLASS_COMM                  0x02
#define ZLOX_USB_CLASS_HID                   0x03
#define ZLOX_USB_CLASS_PHYSICAL              0x05
#define ZLOX_USB_CLASS_IMAGE                 0x06
#define ZLOX_USB_CLASS_PRINTER               0x07
#define ZLOX_USB_CLASS_STORAGE               0x08
#define ZLOX_USB_CLASS_HUB                   0x09
#define ZLOX_USB_CLASS_CDC_DATA              0x0a
#define ZLOX_USB_CLASS_SMART_CARD            0x0b
#define ZLOX_USB_CLASS_SECURITY              0x0d
#define ZLOX_USB_CLASS_VIDEO                 0x0e
#define ZLOX_USB_CLASS_HEALTHCARE            0x0f
#define ZLOX_USB_CLASS_DIAGNOSTIC            0xdc
#define ZLOX_USB_CLASS_WIRELESS              0xe0
#define ZLOX_USB_CLASS_MISC                  0xef
#define ZLOX_USB_CLASS_APP                   0xfe
#define ZLOX_USB_CLASS_VENDOR                0xff
// USB Class Codes end

// USB HID Subclass Codes
#define ZLOX_USB_SUBCLASS_BOOT               0x01

// USB HID Protocol Codes
#define ZLOX_USB_PROTOCOL_KBD                0x01
#define ZLOX_USB_PROTOCOL_MOUSE              0x02

// USB Hub Feature Seletcors
#define ZLOX_USB_F_C_HUB_LOCAL_POWER             0   // Hub
#define ZLOX_USB_F_C_HUB_OVER_CURRENT            1   // Hub
#define ZLOX_USB_F_PORT_CONNECTION               0   // Port
#define ZLOX_USB_F_PORT_ENABLE                   1   // Port
#define ZLOX_USB_F_PORT_SUSPEND                  2   // Port
#define ZLOX_USB_F_PORT_OVER_CURRENT             3   // Port
#define ZLOX_USB_F_PORT_RESET                    4   // Port
#define ZLOX_USB_F_PORT_POWER                    8   // Port
#define ZLOX_USB_F_PORT_LOW_SPEED                9   // Port
#define ZLOX_USB_F_C_PORT_CONNECTION             16  // Port
#define ZLOX_USB_F_C_PORT_ENABLE                 17  // Port
#define ZLOX_USB_F_C_PORT_SUSPEND                18  // Port
#define ZLOX_USB_F_C_PORT_OVER_CURRENT           19  // Port
#define ZLOX_USB_F_C_PORT_RESET                  20  // Port
#define ZLOX_USB_F_PORT_TEST                     21  // Port
#define ZLOX_USB_F_PORT_INDICATOR                22  // Port
// USB Hub Feature Seletcors end

#define ZLOX_USB_GET_CFG_HDR(usb_hcd) (&(usb_hcd->pciconf_lst-> \
					ptr[usb_hcd->pciconf_lst_idx].cfg_hdr))
#define ZLOX_USB_GET_CFG_ADDR(usb_hcd) (usb_hcd->pciconf_lst-> \
					ptr[usb_hcd->pciconf_lst_idx].cfg_addr)

#define ZLOX_USB_LINK_DATA(link,T,m) \
    (T *)((ZLOX_UINT8 *)(link) - (ZLOX_UINT32)(&(((T*)0)->m)))

#define ZLOX_USB_LIST_FOR_EACH_SAFE(it, n, list, m) \
    for (it = ZLOX_USB_LINK_DATA((list).next, typeof(*it), m), \
        n = ZLOX_USB_LINK_DATA(it->m.next, typeof(*it), m); \
        &it->m != &(list); \
        it = n, \
        n = ZLOX_USB_LINK_DATA(n->m.next, typeof(*it), m))

typedef enum _ZLOX_USB_HCD_TYPE
{
	ZLOX_USB_HCD_TYPE_NONE,
	ZLOX_USB_HCD_TYPE_UHCI,
	ZLOX_USB_HCD_TYPE_OHCI,
	ZLOX_USB_HCD_TYPE_EHCI,
	ZLOX_USB_HCD_TYPE_XHCI,
}ZLOX_USB_HCD_TYPE;

typedef struct _ZLOX_USB_HCD ZLOX_USB_HCD;

struct _ZLOX_USB_HCD
{
	ZLOX_PCI_DEVCONF_LST * pciconf_lst;
	ZLOX_UINT32 pciconf_lst_idx;
	ZLOX_USB_HCD * next;
	ZLOX_USB_HCD_TYPE type;
	ZLOX_UINT32 irq; /*PCI irq*/
	ZLOX_UINT32 irq_pin; /*PCI interrupt pin*/
	ZLOX_VOID * hcd_priv;
	ZLOX_VOID (*poll)(struct _ZLOX_USB_HCD * usb_hcd);
};

typedef struct _ZLOX_USB_GET_EXT_INFO
{
	ZLOX_PCI_CONF_HDR * cfg_hdr;
	ZLOX_UNI_PCI_CFG_ADDR cfg_addr;
}ZLOX_USB_GET_EXT_INFO;

// USB Device Request
// usb Revision 1.1 spec --- 9.3 USB Device Requests
// at page 183 (PDF top page is 199)
typedef struct _ZLOX_USB_DEV_REQ
{
	ZLOX_UINT8 type;
	ZLOX_UINT8 req;
	ZLOX_UINT16 value;
	ZLOX_UINT16 index;
	ZLOX_UINT16 len;
} ZLOX_PACKED ZLOX_USB_DEV_REQ;

// USB Device Descriptor
// USB Revision 1.1 spec --- 9.6.1 Device
// at page 196 (PDF top page is 212)
typedef struct  _ZLOX_USB_DEVICE_DESC
{
	ZLOX_UINT8 len;
	ZLOX_UINT8 type;
	ZLOX_UINT16 usbVer;
	ZLOX_UINT8 devClass;
	ZLOX_UINT8 devSubClass;
	ZLOX_UINT8 devProtocol;
	ZLOX_UINT8 maxPacketSize;
	ZLOX_UINT16 vendorId;
	ZLOX_UINT16 productId;
	ZLOX_UINT16 deviceVer;
	ZLOX_UINT8 vendorStr;
	ZLOX_UINT8 productStr;
	ZLOX_UINT8 serialStr;
	ZLOX_UINT8 confCount;
} ZLOX_PACKED ZLOX_USB_DEVICE_DESC;

// USB Configuration Descriptor
// USB Revision 1.1 spec --- 9.6.2 Configuration
// at page 199 (PDF top page is 215)
typedef struct _ZLOX_USB_CONF_DESC
{
	ZLOX_UINT8 len;
	ZLOX_UINT8 type;
	ZLOX_UINT16 totalLen;
	ZLOX_UINT8 intfCount;
	ZLOX_UINT8 confValue;
	ZLOX_UINT8 confStr;
	ZLOX_UINT8 attributes;
	ZLOX_UINT8 maxPower;
} ZLOX_PACKED ZLOX_USB_CONF_DESC;

// USB Interface Descriptor
// USB Revision 1.1 spec --- 9.6.3 Interface
// at page 201 (PDF top page is 217)
typedef struct _ZLOX_USB_INTF_DESC
{
	ZLOX_UINT8 len;
	ZLOX_UINT8 type;
	ZLOX_UINT8 intfIndex;
	ZLOX_UINT8 altSetting;
	ZLOX_UINT8 endpCount;
	ZLOX_UINT8 intfClass;
	ZLOX_UINT8 intfSubClass;
	ZLOX_UINT8 intfProtocol;
	ZLOX_UINT8 intfStr;
} ZLOX_PACKED ZLOX_USB_INTF_DESC;

// USB Endpoint Descriptor
// USB Revision 1.1 spec --- 9.6.4 Endpoint
// at page 203 (PDF top page is 219)
typedef struct _ZLOX_USB_ENDP_DESC
{
	ZLOX_UINT8 len;
	ZLOX_UINT8 type;
	ZLOX_UINT8 addr;
	ZLOX_UINT8 attributes;
	ZLOX_UINT16 maxPacketSize;
	ZLOX_UINT8 interval;
} ZLOX_PACKED ZLOX_USB_ENDP_DESC;

// USB String Descriptor
// USB Revision 1.1 spec --- 9.6.5 String
// at page 204 (PDF top page is 220)
typedef struct _ZLOX_USB_STRING_DESC
{
	ZLOX_UINT8 len;
	ZLOX_UINT8 type;
	ZLOX_UINT16 str[];
} ZLOX_PACKED ZLOX_USB_STRING_DESC;

// USB Hub Descriptor
// USB Revision 1.1 spec --- 11.15.2.1 Hub Descriptor
// at page 264 (PDF top page is 280)
typedef struct _ZLOX_USB_HUB_DESC
{
	ZLOX_UINT8 len;
	ZLOX_UINT8 type;
	ZLOX_UINT8 portCount;
	ZLOX_UINT16 chars;
	ZLOX_UINT8 portPowerTime;
	ZLOX_UINT8 current;
	// removable/power control bits vary in size
} ZLOX_PACKED ZLOX_USB_HUB_DESC;

// USB Endpoint
typedef struct _ZLOX_USB_ENDPOINT
{
	ZLOX_USB_ENDP_DESC desc;
	ZLOX_UINT32 toggle;
} ZLOX_USB_ENDPOINT;

// USB Transfer
typedef struct _ZLOX_USB_TRANSFER
{
	ZLOX_USB_ENDPOINT * endp;
	ZLOX_USB_DEV_REQ * req;
	ZLOX_VOID * data;
	ZLOX_UINT32 len;
	ZLOX_BOOL complete;
	ZLOX_BOOL success;
} ZLOX_USB_TRANSFER;

typedef struct _ZLOX_USB_LINK
{
	struct _ZLOX_USB_LINK * prev;
	struct _ZLOX_USB_LINK * next;
} ZLOX_USB_LINK;

// USB Device
typedef struct _ZLOX_USB_DEVICE
{
	struct _ZLOX_USB_DEVICE * parent;
	struct _ZLOX_USB_DEVICE * next;
	ZLOX_VOID * hcd;
	ZLOX_VOID * drv;

	ZLOX_UINT32 port;
	ZLOX_UINT32 speed;
	ZLOX_UINT32 addr;
	ZLOX_UINT32 maxPacketSize;

	ZLOX_USB_ENDPOINT endp;

	ZLOX_USB_INTF_DESC intfDesc;

	ZLOX_VOID (*hcControl)(struct _ZLOX_USB_DEVICE * dev, ZLOX_USB_TRANSFER * t);
	ZLOX_VOID (*hcIntr)(struct _ZLOX_USB_DEVICE * dev, ZLOX_USB_TRANSFER * t);

	ZLOX_VOID (*drvPoll)(struct _ZLOX_USB_DEVICE * dev);
} ZLOX_USB_DEVICE;

typedef struct _ZLOX_USB_DRIVER
{
	ZLOX_BOOL (*init)(ZLOX_USB_DEVICE * dev);
} ZLOX_USB_DRIVER;

ZLOX_SINT32 zlox_usb_get_ext_info(ZLOX_USB_HCD * usb_hcd, 
				ZLOX_USB_GET_EXT_INFO * ext_info);

ZLOX_USB_DEVICE * zlox_usb_dev_create();

ZLOX_BOOL zlox_usb_detect_phys_continuous(ZLOX_UINT32 data_addr, 
				ZLOX_UINT32 data_len);

ZLOX_VOID * zlox_usb_get_cont_phys(ZLOX_UINT32 data_len);

ZLOX_VOID zlox_usb_free_cont_phys(ZLOX_VOID * p);

ZLOX_UINT32 zlox_usb_get_phys(ZLOX_UINT32 addr);

ZLOX_VOID zlox_usb_link_before(ZLOX_USB_LINK * a, ZLOX_USB_LINK * x);

ZLOX_VOID zlox_usb_link_remove(ZLOX_USB_LINK * x);

ZLOX_BOOL zlox_usb_dev_request(ZLOX_USB_DEVICE * dev,
    ZLOX_UINT32 type, ZLOX_UINT32 request,
    ZLOX_UINT32 value, ZLOX_UINT32 index,
    ZLOX_UINT32 len, ZLOX_VOID * data);

ZLOX_BOOL zlox_usb_dev_init(ZLOX_USB_DEVICE * dev);

ZLOX_VOID zlox_usb_poll();

ZLOX_SINT32 zlox_usb_init();

#endif // _ZLOX_USB_H_

