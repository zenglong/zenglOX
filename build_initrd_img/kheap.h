/* kheap.h -- 堆相关的结构定义 */

#ifndef _KHEAP_H_
#define _KHEAP_H_

#include "common.h"

#define HEAP_MAGIC 0x123890AB

typedef struct _ORDERED_ARRAY
{
	VOID * array; // 指针数组的起始线性地址
	UINT32 size; // 指针数组里当前已存储的hole指针数目
	UINT32 max_size; // 指针数组最多能容纳的指针数目
	VOID * less_than; // 指针数组用于进行排序比较的函数
} ORDERED_ARRAY;

typedef struct _KHP_HEADER
{
	UINT32 magic; // Magic number,用于判断是否是有效的ZLOX_KHP_HEADER
	UINT8 is_hole; // 如果是1则说明是未分配的hole，如果是0则说明是已分配的block
	UINT32 size; // hole或block的总大小的字节数(包括头，数据部分及底部)
} KHP_HEADER; // 堆里的hole或block的头部结构

typedef struct _KHP_FOOTER
{
	UINT32 magic; // 和ZLOX_KHP_HEADER里的magic作用一样
	KHP_HEADER * header; // 通过header指针，方便从FOOTER底部结构直接访问HEADER头部信息
} KHP_FOOTER; // 堆里的hole或block的底部结构

typedef struct _HEAP
{
	ORDERED_ARRAY index;
	ORDERED_ARRAY blk_index; // for debug
	UINT32 start_address;	// The start of our allocated space.
	UINT32 end_address;	// The end of our allocated space. May be expanded up to max_address.
	UINT32 max_address;	// The maximum address the heap can be expanded to.
	UINT8 supervisor;		// Should extra pages requested by us be mapped as supervisor-only?
	UINT8 readonly;		// Should extra pages requested by us be mapped as read-only?
} HEAP;

#endif // _KHEAP_H_

