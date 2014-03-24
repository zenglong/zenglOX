/* zlox_ordered_array.h Interface for creating, inserting and deleting
	from ordered arrays. */

#ifndef _ZLOX_ORDERED_ARRAY_H_
#define _ZLOX_ORDERED_ARRAY_H_

#include "zlox_common.h"

typedef void * ZLOX_VPTR;

typedef ZLOX_SINT8 (*ZLOX_LESSTHEN_FUNC)(ZLOX_VPTR,ZLOX_VPTR);

typedef struct _ZLOX_ORDERED_ARRAY
{
	ZLOX_VPTR * array; // 指针数组的起始线性地址
	ZLOX_UINT32 size; // 指针数组里当前已存储的hole指针数目
	ZLOX_UINT32 max_size; // 指针数组最多能容纳的指针数目
	ZLOX_LESSTHEN_FUNC less_than; // 指针数组用于进行排序比较的函数
} ZLOX_ORDERED_ARRAY;

ZLOX_ORDERED_ARRAY zlox_place_ordered_array(ZLOX_VOID *addr, ZLOX_UINT32 max_size, ZLOX_LESSTHEN_FUNC less_than);

ZLOX_VOID zlox_insert_ordered_array(ZLOX_VPTR item, ZLOX_ORDERED_ARRAY *array);

/**
   Lookup the item at index i.
**/
ZLOX_VPTR zlox_lookup_ordered_array(ZLOX_UINT32 i, ZLOX_ORDERED_ARRAY *array);

ZLOX_VOID zlox_remove_ordered_array(ZLOX_UINT32 i, ZLOX_ORDERED_ARRAY *array);

#endif //_ZLOX_ORDERED_ARRAY_H_

