/* kheap.h -- 堆相关的结构定义 */

#ifndef _KHEAP_H_
#define _KHEAP_H_

#include "common.h"

typedef struct _ORDERED_ARRAY
{
	VOID * array; // 指针数组的起始线性地址
	UINT32 size; // 指针数组里当前已存储的hole指针数目
	UINT32 max_size; // 指针数组最多能容纳的指针数目
	VOID * less_than; // 指针数组用于进行排序比较的函数
} ORDERED_ARRAY;

typedef struct _HEAP
{
	ORDERED_ARRAY index;
	UINT32 start_address;	// The start of our allocated space.
	UINT32 end_address;	// The end of our allocated space. May be expanded up to max_address.
	UINT32 max_address;	// The maximum address the heap can be expanded to.
	UINT8 supervisor;		// Should extra pages requested by us be mapped as supervisor-only?
	UINT8 readonly;		// Should extra pages requested by us be mapped as read-only?
} HEAP;

#endif // _KHEAP_H_

