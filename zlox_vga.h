/*zlox_vga.h the vga header*/

#ifndef _ZLOX_VGA_H_
#define _ZLOX_VGA_H_

#include "zlox_common.h"

#define ZLOX_VGA_MODE_80X25_TEXT 0
#define ZLOX_VGA_MODE_320X200X256 1
#define ZLOX_VGA_MODE_640X480X16 2

#define ZLOX_VGA_AC_INDEX 0x3C0
#define ZLOX_VGA_AC_WRITE 0x3C0
#define ZLOX_VGA_AC_READ 0x3C1
#define ZLOX_VGA_MISC_WRITE 0x3C2
#define ZLOX_VGA_SEQ_INDEX 0x3C4
#define ZLOX_VGA_SEQ_DATA 0x3C5
#define ZLOX_VGA_DAC_READ_INDEX 0x3C7
#define ZLOX_VGA_DAC_WRITE_INDEX 0x3C8
#define ZLOX_VGA_DAC_DATA 0x3C9
#define ZLOX_VGA_MISC_READ 0x3CC
#define ZLOX_VGA_GC_INDEX 0x3CE
#define ZLOX_VGA_GC_DATA 0x3CF
/* COLOR emulation MONO emulation */
#define ZLOX_VGA_CRTC_INDEX 0x3D4 /* 0x3B4 */
#define ZLOX_VGA_CRTC_DATA 0x3D5 /* 0x3B5 */
#define ZLOX_VGA_INSTAT_READ 0x3DA

#define ZLOX_VGA_NUM_SEQ_REGS 5
#define ZLOX_VGA_NUM_CRTC_REGS 25
#define ZLOX_VGA_NUM_GC_REGS 9
#define ZLOX_VGA_NUM_AC_REGS 21
#define ZLOX_VGA_NUM_REGS (1 + ZLOX_VGA_NUM_SEQ_REGS + ZLOX_VGA_NUM_CRTC_REGS + \
				ZLOX_VGA_NUM_GC_REGS + ZLOX_VGA_NUM_AC_REGS)

ZLOX_SINT32 zlox_vga_set_mode(ZLOX_UINT32 mode);

ZLOX_SINT32 zlox_vga_update_screen(ZLOX_UINT8 * buffer, ZLOX_UINT32 buffer_size);

ZLOX_UINT32 zlox_vga_get_text_font();

ZLOX_SINT32 zlox_vga_exit_graphic_mode();

#endif // _ZLOX_VGA_H_

