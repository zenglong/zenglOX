// zlox_ps2.h -- 和ps2控制器相关的定义

#ifndef _ZLOX_PS2_H_
#define _ZLOX_PS2_H_

#include "zlox_common.h"

ZLOX_BOOL zlox_ps2_init(ZLOX_BOOL need_openMouse);

// 用于系统调用, 获取PS2的初始化状态
ZLOX_SINT32 zlox_ps2_get_status(ZLOX_BOOL * init_status, ZLOX_BOOL * first_port_status, ZLOX_BOOL * sec_port_status);

#endif // _ZLOX_PS2_H_

