/* zlox_uheap.h user heap functions header file */

#ifndef _ZLOX_UHEAP_H_
#define _ZLOX_UHEAP_H_

#include "zlox_kheap.h"

#define ZLOX_UHEAP_START 0xF0001000
#define ZLOX_UHEAP_INITIAL_SIZE 0x50000
#define ZLOX_UHEAP_MAX 0xFFFFF000
#define ZLOX_UHEAP_MIN_SIZE 0x23000

#define ZLOX_UHEAP_INDEX_SIZE 0xA000

// 给用户态程式使用的分配堆函数
ZLOX_UINT32 zlox_umalloc(ZLOX_UINT32 sz);

// 给用户态程式使用的释放堆函数
ZLOX_VOID zlox_ufree(ZLOX_VOID *p);

ZLOX_HEAP * zlox_create_uheap(ZLOX_UINT32 start, ZLOX_UINT32 end_addr, 
				ZLOX_UINT32 max, ZLOX_UINT8 supervisor, ZLOX_UINT8 readonly);

#endif //_ZLOX_UHEAP_H_

