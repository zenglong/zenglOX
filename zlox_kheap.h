/* zlox_kheap.h Kernel heap functions header file */

#ifndef _ZLOX_KHEAP_H_
#define _ZLOX_KHEAP_H_

#include "zlox_common.h"
#include "zlox_ordered_array.h"

#define ZLOX_KHEAP_START 0xC0000000
#define ZLOX_KHEAP_INITIAL_SIZE	0x100000

#define ZLOX_HEAP_INDEX_SIZE 0x20000
#define ZLOX_HEAP_MAGIC 0x123890AB
#define ZLOX_HEAP_MIN_SIZE 0x70000

typedef struct _ZLOX_KHP_HEADER
{
	ZLOX_UINT32 magic; // Magic number,用于判断是否是有效的ZLOX_KHP_HEADER
	ZLOX_UINT8 is_hole; // 如果是1则说明是未分配的hole，如果是0则说明是已分配的block
	ZLOX_UINT32 size; // hole或block的总大小的字节数(包括头，数据部分及底部)
}ZLOX_KHP_HEADER; // 堆里的hole或block的头部结构

typedef struct _ZLOX_KHP_FOOTER
{
	ZLOX_UINT32 magic; // 和ZLOX_KHP_HEADER里的magic作用一样
	ZLOX_KHP_HEADER * header; // 通过header指针，方便从FOOTER底部结构直接访问HEADER头部信息
}ZLOX_KHP_FOOTER; // 堆里的hole或block的底部结构

typedef struct _ZLOX_HEAP
{
	ZLOX_ORDERED_ARRAY index;
	ZLOX_UINT32 start_address;	// The start of our allocated space.
	ZLOX_UINT32 end_address;	// The end of our allocated space. May be expanded up to max_address.
	ZLOX_UINT32 max_address;	// The maximum address the heap can be expanded to.
	ZLOX_UINT8 supervisor;		// Should extra pages requested by us be mapped as supervisor-only?
	ZLOX_UINT8 readonly;		// Should extra pages requested by us be mapped as read-only?
} ZLOX_HEAP;

/**
   Create a new heap.
**/
ZLOX_HEAP * zlox_create_heap(ZLOX_UINT32 start, ZLOX_UINT32 end_addr, 
							ZLOX_UINT32 max, ZLOX_UINT8 supervisor, ZLOX_UINT8 readonly);

ZLOX_VOID * zlox_alloc(ZLOX_UINT32 size, ZLOX_UINT8 page_align, ZLOX_HEAP * heap);

ZLOX_VOID zlox_free(ZLOX_VOID *p, ZLOX_HEAP *heap);

/**
   Allocate a chunk of memory, sz in size. If align == 1,
   the chunk must be page-aligned. If phys != 0, the physical
   location of the allocated chunk will be stored into phys.

   This is the internal version of zlox_kmalloc. More user-friendly
   parameter representations are available in zlox_kmalloc, zlox_kmalloc_a,
   zlox_kmalloc_ap, zlox_kmalloc_p.
**/
ZLOX_UINT32 zlox_kmalloc_int(ZLOX_UINT32 sz, ZLOX_SINT32 align, ZLOX_UINT32 *phys);

/**
   Allocate a chunk of memory, sz in size. The chunk must be
   page aligned.
**/
ZLOX_UINT32 zlox_kmalloc_a(ZLOX_UINT32 sz);

/**
   Allocate a chunk of memory, sz in size. The physical address
   is returned in phys. Phys MUST be a valid pointer to ZLOX_UINT32!
**/
ZLOX_UINT32 zlox_kmalloc_p(ZLOX_UINT32 sz, ZLOX_UINT32 *phys);

/**
   Allocate a chunk of memory, sz in size. The physical address 
   is returned in phys. It must be page-aligned.
**/
ZLOX_UINT32 zlox_kmalloc_ap(ZLOX_UINT32 sz, ZLOX_UINT32 *phys);

/**
   General allocation function.
**/
ZLOX_UINT32 zlox_kmalloc(ZLOX_UINT32 sz);

ZLOX_VOID zlox_kfree(ZLOX_VOID *p);

#endif //_ZLOX_KHEAP_H_

