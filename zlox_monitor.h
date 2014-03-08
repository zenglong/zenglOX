/*zlox_monitor.h the monitor header*/

#ifndef _ZLOX_MONITOR_H_
#define _ZLOX_MONITOR_H_

#include "zlox_common.h"

// Writes a single character out to the screen.
ZLOX_VOID zlox_monitor_put(ZLOX_CHAR c);

// Clears the screen, by copying lots of spaces to the framebuffer.
ZLOX_VOID zlox_monitor_clear();

// Outputs a null-terminated ASCII string to the monitor.
ZLOX_VOID zlox_monitor_write(ZLOX_CHAR * c);

// Outputs an integer as Hex
ZLOX_VOID zlox_monitor_write_hex(ZLOX_UINT32 n);

// Outputs an integer to the monitor.
ZLOX_VOID zlox_monitor_write_dec(ZLOX_UINT32 n);

#endif //_ZLOX_MONITOR_H_

