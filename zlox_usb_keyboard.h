// zlox_usb_keyboard.h -- the header file relate to USB Keyboard

#ifndef _ZLOX_USB_KEYBOARD_H_
#define _ZLOX_USB_KEYBOARD_H_

#include "zlox_common.h"
#include "zlox_usb.h"

#define ZLOX_USB_KBD_CH_ESCAPE                       0x1B
#define ZLOX_USB_KBD_CH_SHIFT_TAB                    0xF0

#define ZLOX_USB_KBD_KEY_A                           0x04
#define ZLOX_USB_KBD_KEY_B                           0x05
#define ZLOX_USB_KBD_KEY_C                           0x06
#define ZLOX_USB_KBD_KEY_D                           0x07
#define ZLOX_USB_KBD_KEY_E                           0x08
#define ZLOX_USB_KBD_KEY_F                           0x09
#define ZLOX_USB_KBD_KEY_G                           0x0a
#define ZLOX_USB_KBD_KEY_H                           0x0b
#define ZLOX_USB_KBD_KEY_I                           0x0c
#define ZLOX_USB_KBD_KEY_J                           0x0d
#define ZLOX_USB_KBD_KEY_K                           0x0e
#define ZLOX_USB_KBD_KEY_L                           0x0f
#define ZLOX_USB_KBD_KEY_M                           0x10
#define ZLOX_USB_KBD_KEY_N                           0x11
#define ZLOX_USB_KBD_KEY_O                           0x12
#define ZLOX_USB_KBD_KEY_P                           0x13
#define ZLOX_USB_KBD_KEY_Q                           0x14
#define ZLOX_USB_KBD_KEY_R                           0x15
#define ZLOX_USB_KBD_KEY_S                           0x16
#define ZLOX_USB_KBD_KEY_T                           0x17
#define ZLOX_USB_KBD_KEY_U                           0x18
#define ZLOX_USB_KBD_KEY_V                           0x19
#define ZLOX_USB_KBD_KEY_W                           0x1a
#define ZLOX_USB_KBD_KEY_X                           0x1b
#define ZLOX_USB_KBD_KEY_Y                           0x1c
#define ZLOX_USB_KBD_KEY_Z                           0x1d
#define ZLOX_USB_KBD_KEY_1                           0x1e
#define ZLOX_USB_KBD_KEY_2                           0x1f
#define ZLOX_USB_KBD_KEY_3                           0x20
#define ZLOX_USB_KBD_KEY_4                           0x21
#define ZLOX_USB_KBD_KEY_5                           0x22
#define ZLOX_USB_KBD_KEY_6                           0x23
#define ZLOX_USB_KBD_KEY_7                           0x24
#define ZLOX_USB_KBD_KEY_8                           0x25
#define ZLOX_USB_KBD_KEY_9                           0x26
#define ZLOX_USB_KBD_KEY_0                           0x27
#define ZLOX_USB_KBD_KEY_RETURN                      0x28
#define ZLOX_USB_KBD_KEY_ESCAPE                      0x29
#define ZLOX_USB_KBD_KEY_BACKSPACE                   0x2a
#define ZLOX_USB_KBD_KEY_TAB                         0x2b
#define ZLOX_USB_KBD_KEY_SPACE                       0x2c
#define ZLOX_USB_KBD_KEY_MINUS                       0x2d
#define ZLOX_USB_KBD_KEY_EQUALS                      0x2e
#define ZLOX_USB_KBD_KEY_LBRACKET                    0x2f
#define ZLOX_USB_KBD_KEY_RBRACKET                    0x30
#define ZLOX_USB_KBD_KEY_BACKSLASH                   0x31
#define ZLOX_USB_KBD_KEY_CAPS_LOCK2                  0x32
#define ZLOX_USB_KBD_KEY_SEMICOLON                   0x33
#define ZLOX_USB_KBD_KEY_APOSTROPHE                  0x34
#define ZLOX_USB_KBD_KEY_GRAVE                       0x35
#define ZLOX_USB_KBD_KEY_COMMA                       0x36
#define ZLOX_USB_KBD_KEY_PERIOD                      0x37
#define ZLOX_USB_KBD_KEY_SLASH                       0x38
#define ZLOX_USB_KBD_KEY_CAPS_LOCK                   0x39
#define ZLOX_USB_KBD_KEY_F1                          0x3a
#define ZLOX_USB_KBD_KEY_F2                          0x3b
#define ZLOX_USB_KBD_KEY_F3                          0x3c
#define ZLOX_USB_KBD_KEY_F4                          0x3d
#define ZLOX_USB_KBD_KEY_F5                          0x3e
#define ZLOX_USB_KBD_KEY_F6                          0x3f
#define ZLOX_USB_KBD_KEY_F7                          0x40
#define ZLOX_USB_KBD_KEY_F8                          0x41
#define ZLOX_USB_KBD_KEY_F9                          0x42
#define ZLOX_USB_KBD_KEY_F10                         0x43
#define ZLOX_USB_KBD_KEY_F11                         0x44
#define ZLOX_USB_KBD_KEY_F12                         0x45
#define ZLOX_USB_KBD_KEY_PRINT                       0x46
#define ZLOX_USB_KBD_KEY_SCROLL_LOCK                 0x47
#define ZLOX_USB_KBD_KEY_PAUSE                       0x48
#define ZLOX_USB_KBD_KEY_INSERT                      0x49
#define ZLOX_USB_KBD_KEY_HOME                        0x4a
#define ZLOX_USB_KBD_KEY_PAGE_UP                     0x4b
#define ZLOX_USB_KBD_KEY_DELETE                      0x4c
#define ZLOX_USB_KBD_KEY_END                         0x4d
#define ZLOX_USB_KBD_KEY_PAGE_DOWN                   0x4e
#define ZLOX_USB_KBD_KEY_RIGHT                       0x4f
#define ZLOX_USB_KBD_KEY_LEFT                        0x50
#define ZLOX_USB_KBD_KEY_DOWN                        0x51
#define ZLOX_USB_KBD_KEY_UP                          0x52
#define ZLOX_USB_KBD_KEY_NUM_LOCK                    0x53
#define ZLOX_USB_KBD_KEY_KP_DIV                      0x54
#define ZLOX_USB_KBD_KEY_KP_MUL                      0x55
#define ZLOX_USB_KBD_KEY_KP_SUB                      0x56
#define ZLOX_USB_KBD_KEY_KP_ADD                      0x57
#define ZLOX_USB_KBD_KEY_ENTER                       0x58
#define ZLOX_USB_KBD_KEY_KP1                         0x59
#define ZLOX_USB_KBD_KEY_KP2                         0x5a
#define ZLOX_USB_KBD_KEY_KP3                         0x5b
#define ZLOX_USB_KBD_KEY_KP4                         0x5c
#define ZLOX_USB_KBD_KEY_KP5                         0x5d
#define ZLOX_USB_KBD_KEY_KP6                         0x5e
#define ZLOX_USB_KBD_KEY_KP7                         0x5f
#define ZLOX_USB_KBD_KEY_KP8                         0x60
#define ZLOX_USB_KBD_KEY_KP9                         0x61
#define ZLOX_USB_KBD_KEY_KP0                         0x62
#define ZLOX_USB_KBD_KEY_KP_DEC                      0x63

#define ZLOX_USB_KBD_KEY_LCONTROL                    0x01
#define ZLOX_USB_KBD_KEY_LSHIFT                      0x02
#define ZLOX_USB_KBD_KEY_LALT                        0x04
#define ZLOX_USB_KBD_KEY_LWIN                        0x08
#define ZLOX_USB_KBD_KEY_RCONTROL                    0x10
#define ZLOX_USB_KBD_KEY_RSHIFT                      0x20
#define ZLOX_USB_KBD_KEY_RALT                        0x40
#define ZLOX_USB_KBD_KEY_RWIN                        0x80

ZLOX_BOOL zlox_usb_kbd_init(ZLOX_USB_DEVICE * dev);

#endif // _ZLOX_USB_KEYBOARD_H_

