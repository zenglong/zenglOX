/*elf.h 和ELF可执行文件格式相关的定义*/

#ifndef _ELF_H_
#define _ELF_H_

#include "common.h"

#define EXECVE_ADDR	0x8048000
#define ELF_LD_SO_VADDR	0x80000000

#define ELF32_R_SYM(val)		((val) >> 8)
#define ELF32_R_TYPE(val)		((val) & 0xff)

typedef UINT16 ELF32_HALF;
typedef UINT32 ELF32_OFF;
typedef UINT32 ELF32_ADDR;
typedef UINT32 ELF32_WORD;
typedef SINT32 ELF32_SWORD;

typedef struct _ELF32_DYN{
	ELF32_SWORD    d_tag;
	union {
		ELF32_WORD d_val;
		ELF32_ADDR d_ptr;
	} d_un;
} ELF32_DYN;

typedef struct _ELF_DYN_MAP{
	ELF32_DYN * Dynamic;
	VOID * dl_runtime_resolve;
	UINT32 hash;
	UINT32 strtab;
	UINT32 symtab;
	UINT32 plt_got;
	UINT32 jmprel_type;
	UINT32 jmprel;
	UINT32 jmprelsz;
	UINT32 rel;
	UINT32 relsz;
} ELF_DYN_MAP;

typedef struct _ELF_LINK_MAP_LIST ELF_LINK_MAP_LIST;

typedef struct _ELF_LINK_MAP{
	ELF_LINK_MAP_LIST * maplst;
	UINT32 kmap_index;
	UINT32 index;
	CHAR * soname;
	UINT32 entry;
	UINT32 vaddr;
	UINT32 msize;
	ELF_DYN_MAP dyn;
} ELF_LINK_MAP;

struct _ELF_LINK_MAP_LIST{
	BOOL isInit;
	SINT32 count;
	SINT32 size;
	ELF_LINK_MAP * ptr;
};

typedef struct _ELF32_SYM{
	UINT32 st_name;
	ELF32_ADDR st_value;
	UINT32 st_size;
	UINT8 st_info;
	UINT8 st_other;
	UINT16 st_shndx;
} ELF32_SYM;

typedef struct _ELF32_REL{
	ELF32_ADDR r_offset;
	UINT32   r_info;
} ELF32_REL;

#endif // _ELF_H_

