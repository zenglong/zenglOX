/*zlox_ide.h the ide header*/

#ifndef _ZLOX_IDE_H_
#define _ZLOX_IDE_H_

#include "zlox_common.h"

ZLOX_SINT32 zlox_ide_probe();

ZLOX_SINT32 zlox_ide_before_send_command(ZLOX_UINT8 direction, ZLOX_UINT16 bus);

ZLOX_SINT32 zlox_ide_start_dma(ZLOX_UINT16 bus);

ZLOX_SINT32 zlox_ide_after_send_command(ZLOX_UINT16 bus);

ZLOX_SINT32 zlox_ide_get_buffer_data(ZLOX_UINT8 * buffer, ZLOX_UINT32 len, ZLOX_UINT16 bus);

ZLOX_SINT32 zlox_ide_set_buffer_data(ZLOX_UINT8 * buffer, ZLOX_UINT32 len, ZLOX_UINT16 bus);

#endif // _ZLOX_IDE_H_

