/* zlox_timer.h Defines the interface for all PIT-related functions.*/

#ifndef _ZLOX_TIME_H_
#define _ZLOX_TIME_H_

#include "zlox_common.h"

ZLOX_VOID zlox_timer_sleep(ZLOX_UINT32 Ticks, ZLOX_BOOL need_switch);

ZLOX_UINT32 zlox_timer_get_tick();

ZLOX_UINT32 zlox_timer_get_frequency();

ZLOX_VOID zlox_init_timer(ZLOX_UINT32 frequency);

#endif //_ZLOX_TIME_H_

