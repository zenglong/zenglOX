/* zlox_kheap.h Kernel heap functions header file */

#ifndef _ZLOX_KHEAP_H_
#define _ZLOX_KHEAP_H_

#include "zlox_common.h"

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

#endif //_ZLOX_KHEAP_H_

