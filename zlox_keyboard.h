// zlox_keyboard.h the keyboard header

#ifndef _ZLOX_KEYBOARD_H_
#define _ZLOX_KEYBOARD_H_

#include "zlox_common.h"

ZLOX_BOOL zlox_initKeyboard();

ZLOX_UINT8 zlox_get_control_keys();

ZLOX_UINT8 zlox_release_control_keys(ZLOX_UINT8 key);

#endif // _ZLOX_KEYBOARD_H_

