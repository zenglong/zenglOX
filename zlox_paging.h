/* zlox_paging.h Defines the interface for and structures relating to paging.*/

#ifndef _ZLOX_PAGING_H_
#define _ZLOX_PAGING_H_

#include "zlox_common.h"
#include "zlox_isr.h"

struct _ZLOX_PAGE
{
	ZLOX_UINT32 present    : 1;   // Page present in memory
	ZLOX_UINT32 rw         : 1;   // Read-only if clear, readwrite if set
    ZLOX_UINT32 user       : 1;   // Supervisor level only if clear
    ZLOX_UINT32 accessed   : 1;   // Has the page been accessed since last refresh?
    ZLOX_UINT32 dirty      : 1;   // Has the page been written to since last refresh?
    ZLOX_UINT32 unused     : 7;   // Amalgamation of unused and reserved bits
    ZLOX_UINT32 frame      : 20;  // Frame address (shifted right 12 bits)
}__attribute__((packed));

typedef struct _ZLOX_PAGE ZLOX_PAGE;

struct _ZLOX_PAGE_TABLE
{
	ZLOX_PAGE pages[1024];
}__attribute__((packed));

typedef struct _ZLOX_PAGE_TABLE ZLOX_PAGE_TABLE;

typedef struct _ZLOX_PAGE_DIRECTORY
{
    /**
       Array of pointers to pagetables.
    **/
    ZLOX_PAGE_TABLE *tables[1024];
    /**
       Array of pointers to the pagetables above, but gives their *physical*
       location, for loading into the CR3 register.
    **/
    ZLOX_UINT32 tablesPhysical[1024];

    /**
       The physical address of tablesPhysical. This comes into play
       when we get our kernel heap allocated and the directory
       may be in a different location in virtual memory.
    **/
    ZLOX_UINT32 physicalAddr;
}ZLOX_PAGE_DIRECTORY;

ZLOX_VOID zlox_init_paging();

#endif //_ZLOX_PAGING_H_

