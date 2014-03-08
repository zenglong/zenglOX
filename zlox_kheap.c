/* zlox_kheap.c Kernel heap functions, also provides
            a placement malloc() for use before the heap is 
            initialised. */

#include "zlox_kheap.h"

// end is defined in the linker script.
extern ZLOX_UINT32 _end;
ZLOX_UINT32 placement_address = (ZLOX_UINT32)&_end;

ZLOX_UINT32 zlox_kmalloc_int(ZLOX_UINT32 sz, ZLOX_SINT32 align, ZLOX_UINT32 *phys)
{
	ZLOX_UINT32 tmp;
    // This will eventually call malloc() on the kernel heap.
    // For now, though, we just assign memory at placement_address
    // and increment it by sz. Even when we've coded our kernel
    // heap, this will be useful for use before the heap is initialised.
    if (align == 1 && (placement_address & 0x00000FFF) )
    {
        // Align the placement address;
        placement_address &= 0xFFFFF000;
        placement_address += 0x1000;
    }
    if (phys)
    {
        *phys = placement_address;
    }
    tmp = placement_address;
    placement_address += sz;
    return tmp;
}

ZLOX_UINT32 zlox_kmalloc_a(ZLOX_UINT32 sz)
{
    return zlox_kmalloc_int(sz, 1, 0);
}

ZLOX_UINT32 zlox_kmalloc_p(ZLOX_UINT32 sz, ZLOX_UINT32 *phys)
{
    return zlox_kmalloc_int(sz, 0, phys);
}

ZLOX_UINT32 zlox_kmalloc_ap(ZLOX_UINT32 sz, ZLOX_UINT32 *phys)
{
    return zlox_kmalloc_int(sz, 1, phys);
}

ZLOX_UINT32 zlox_kmalloc(ZLOX_UINT32 sz)
{
    return zlox_kmalloc_int(sz, 0, 0);
}

