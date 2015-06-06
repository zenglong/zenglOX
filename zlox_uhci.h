// zlox_uhci.h -- the header file relate to uhci

#ifndef _ZLOX_UHCI_H_
#define _ZLOX_UHCI_H_

#include "zlox_common.h"
#include "zlox_pci.h"
#include "zlox_usb.h"

// Limits
#define ZLOX_UHCI_MAX_QH 8
#define ZLOX_UHCI_MAX_TD 32

// TD Link Pointer
// uhci Revision 1.1 spec --- 3.2.1 TD LINK POINTER (DWORD 0: 00-03h)
// <Link Pointer (LP)> at page 21 (PDF top page is 27)
#define ZLOX_UHCI_TD_PTR_TERMINATE (1 << 0)
#define ZLOX_UHCI_TD_PTR_QH (1 << 1)
#define ZLOX_UHCI_TD_PTR_DEPTH (1 << 2)

// TD Control and Status
// uhci Revision 1.1 spec --- 3.2.2 TD CONTROL AND STATUS (DWORD 1: 04-07h)
// at page 22 (PDF top page is 28)
#define ZLOX_UHCI_TD_CS_ACTLEN 0x000007ff
#define ZLOX_UHCI_TD_CS_BITSTUFF (1 << 17)     // Bitstuff Error
#define ZLOX_UHCI_TD_CS_CRC_TIMEOUT (1 << 18)     // CRC/Time Out Error
#define ZLOX_UHCI_TD_CS_NAK (1 << 19)     // NAK Received
#define ZLOX_UHCI_TD_CS_BABBLE (1 << 20)     // Babble Detected
#define ZLOX_UHCI_TD_CS_DATABUFFER (1 << 21)     // Data Buffer Error
#define ZLOX_UHCI_TD_CS_STALLED (1 << 22)     // Stalled
#define ZLOX_UHCI_TD_CS_ACTIVE (1 << 23)     // Active
#define ZLOX_UHCI_TD_CS_IOC (1 << 24)     // Interrupt on Complete
#define ZLOX_UHCI_TD_CS_IOS (1 << 25)     // Isochronous Select
#define ZLOX_UHCI_TD_CS_LOW_SPEED (1 << 26)     // Low Speed Device
#define ZLOX_UHCI_TD_CS_ERROR_MASK (3 << 27)     // Error counter
#define ZLOX_UHCI_TD_CS_ERROR_SHIFT 27
#define ZLOX_UHCI_TD_CS_SPD (1 << 29)     // Short Packet Detect

// TD Token
// uhci Revision 1.1 spec --- 3.2.3 TD TOKEN (DWORD 2: 08-0Bh)
// at page 24 (PDF top page is 30)
#define ZLOX_UHCI_TD_TOK_PID_MASK 0x000000ff    // Packet Identification
#define ZLOX_UHCI_TD_TOK_DEVADDR_MASK 0x00007f00    // Device Address
#define ZLOX_UHCI_TD_TOK_DEVADDR_SHIFT 8
#define ZLOX_UHCI_TD_TOK_ENDP_MASK 00x0078000    // Endpoint
#define ZLOX_UHCI_TD_TOK_ENDP_SHIFT 15
#define ZLOX_UHCI_TD_TOK_D 0x00080000    // Data Toggle
#define ZLOX_UHCI_TD_TOK_D_SHIFT 19
#define ZLOX_UHCI_TD_TOK_MAXLEN_MASK 0xffe00000    // Maximum Length
#define ZLOX_UHCI_TD_TOK_MAXLEN_SHIFT 21

#define ZLOX_UHCI_RH_MAXCHILD 7

// UHCI Controller I/O Registers
#define ZLOX_UHCI_REG_CMD 0x00 // USB Command
#define ZLOX_UHCI_REG_STS 0x02 // USB Status
#define ZLOX_UHCI_REG_INTR 0x04 // USB Interrupt Enable
#define ZLOX_UHCI_REG_FRNUM 0x06 // Frame Number
#define ZLOX_UHCI_REG_FRBASEADD 0x08 // Frame List Base Address
#define ZLOX_UHCI_REG_SOFMOD 0x0C // Start of Frame Modify
#define ZLOX_UHCI_REG_PORT1 0x10 // Port 1 Status/Control
#define ZLOX_UHCI_REG_PORT2 0x12 // Port 2 Status/Control

// UHCI PCI Registers in PCI config space
#define ZLOX_UHCI_PCI_REG_LEGSUP (0xC0 / 4) // Legacy Support
#define ZLOX_UHCI_PCI_LEGSUP_RWC 0x8f00 /* the R/WC bits */

// USB Command Register
#define ZLOX_UHCI_CMD_RS (1 << 0) // Run/Stop

// Port Status and Control Registers
#define ZLOX_UHCI_PORT_CONNECTION (1 << 0) // Current Connect Status
#define ZLOX_UHCI_PORT_CONNECTION_CHANGE (1 << 1) // Connect Status Change
#define ZLOX_UHCI_PORT_ENABLE (1 << 2) // Port Enabled
#define ZLOX_UHCI_PORT_ENABLE_CHANGE (1 << 3) // Port Enable Change
#define ZLOX_UHCI_PORT_LS (3 << 4) // Line Status
#define ZLOX_UHCI_PORT_RD (1 << 6) // Resume Detect
#define ZLOX_UHCI_PORT_LSDA (1 << 8) // Low Speed Device Attached
#define ZLOX_UHCI_PORT_RESET (1 << 9) // Port Reset
#define ZLOX_UHCI_PORT_SUSP (1 << 12) // Suspend
#define ZLOX_UHCI_PORT_RWC (ZLOX_UHCI_PORT_CONNECTION_CHANGE | ZLOX_UHCI_PORT_ENABLE_CHANGE)

// USB Revision 1.1 spec --- 8.3.1 Packet Identifier Field
// at page 155 (PDF top page is 171)
#define ZLOX_UHCI_TD_PACKET_IN 0x69
#define ZLOX_UHCI_TD_PACKET_OUT 0xe1
#define ZLOX_UHCI_TD_PACKET_SETUP 0x2d

#define ZLOX_UHCI_CLASS 0xc
#define ZLOX_UHCI_SUB_CLASS 0x3
#define ZLOX_UHCI_PROG_IF 0x0

#define ZLOX_UHCI_BAR_IDX 0x4

// uhci Revision 1.1 spec --- 3.3 Queue Head (QH)
// at page 25 (PDF top page is 31)
typedef struct _ZLOX_UHCI_QH
{
    volatile ZLOX_UINT32 head;
    volatile ZLOX_UINT32 element;

    // internal fields
    ZLOX_USB_TRANSFER * transfer;
    ZLOX_USB_LINK qhLink;
    ZLOX_UINT32 tdHead;
    ZLOX_UINT32 active;
    ZLOX_UINT8 pad[4];
} ZLOX_PACKED ZLOX_UHCI_QH;

// Transfer Descriptor
// uhci Revision 1.1 spec --- 3.2 Transfer Descriptor (TD)
// at page 20 (PDF top page is 26)
typedef struct _ZLOX_UHCI_TD
{
    volatile ZLOX_UINT32 link;
    volatile ZLOX_UINT32 cs;
    volatile ZLOX_UINT32 token;
    volatile ZLOX_UINT32 buffer;

    // internal fields
    ZLOX_UINT32 tdNext;
    ZLOX_UINT8 active;
    ZLOX_UINT8 pad[11];
} ZLOX_PACKED ZLOX_UHCI_TD;

/*
 * The full UHCI controller information:
 */
typedef struct _ZLOX_UHCI_HCD {
	ZLOX_UINT32 io_addr; /* Grabbed from PCI */
	ZLOX_UINT32 io_len;
	ZLOX_UINT32 irq; /*PCI irq*/
	ZLOX_UINT32 irq_pin; /*PCI interrupt pin*/
	ZLOX_SINT32 rh_numports; /* Number of root-hub ports */
	ZLOX_UINT32 * frameList;
	ZLOX_UINT32 frameListPhys; // Physical Address of frameList
	ZLOX_UHCI_QH * qhPool;
	ZLOX_UINT32 qhPoolPhys; // Physical Address of qhPool
	ZLOX_UHCI_TD * tdPool;
	ZLOX_UINT32 tdPoolPhys; // Physical Address of tdPool
	ZLOX_UHCI_QH * asyncQH;
	ZLOX_UINT32 asyncQH_Phys; // Physical Address of asyncQH
	ZLOX_VOID * usb_hcd;
}ZLOX_UHCI_HCD;

ZLOX_BOOL zlox_uhci_detect(ZLOX_PCI_CONF_HDR * cfg_hdr);
ZLOX_SINT32 zlox_uhci_print_devinfo(ZLOX_UHCI_HCD * uhci_hcd);
ZLOX_VOID * zlox_uhci_init(ZLOX_USB_HCD * usb_hcd);

#endif // _ZLOX_UHCI_H_

