/*zlox_elf.h 和ELF可执行文件格式相关的定义*/

#ifndef _ZLOX_ELF_H_
#define _ZLOX_ELF_H_

#include "zlox_common.h"

#define ZLOX_ELF_NIDENT	16
#define ZLOX_ELFMAG0	0x7F // e_ident[ZLOX_EI_MAG0]
#define ZLOX_ELFMAG1	'E'  // e_ident[ZLOX_EI_MAG1]
#define ZLOX_ELFMAG2	'L'  // e_ident[ZLOX_EI_MAG2]
#define ZLOX_ELFMAG3	'F'  // e_ident[ZLOX_EI_MAG3]

#define ZLOX_ELFDATA2LSB	(1)  // Little Endian
#define ZLOX_ELFCLASS32	(1)  // 32-bit Architecture

#define ZLOX_EM_386		(3)  // x86 Machine Type
#define ZLOX_EV_CURRENT	(1)  // ELF Current Version

#define ZLOX_EXECVE_ADDR	0x8048000

#define ZLOX_SHN_UNDEF	(0x00) // Undefined/Not present

#define ZLOX_ELF_LINK_MAP_LIST_SIZE 5
#define ZLOX_ELF_KERNEL_MAP_LIST_SIZE 5

#define ZLOX_ELF_LD_SO_VADDR	0x80000000
#define ZLOX_ELF_LD_RESOLVE_NAME	"_dl_runtime_resolve"

#define ZLOX_ELF32_R_SYM(val)		((val) >> 8)
#define ZLOX_ELF32_R_TYPE(val)		((val) & 0xff)

#define ZLOX_R_386_32	   1		/* Direct 32 bit  */
#define ZLOX_R_386_PC32	   2		/* PC relative 32 bit */
#define ZLOX_R_386_GOT32	   3		/* 32 bit GOT entry */
#define ZLOX_R_386_PLT32	   4		/* 32 bit PLT address */
#define ZLOX_R_386_COPY	   5		/* Copy symbol at runtime */
#define ZLOX_R_386_GLOB_DAT	   6		/* Create GOT entry */
#define ZLOX_R_386_JMP_SLOT	   7		/* Create PLT entry */
#define ZLOX_R_386_RELATIVE	   8		/* Adjust by program base */
#define ZLOX_R_386_GOTOFF	   9		/* 32 bit offset to GOT */
#define ZLOX_R_386_GOTPC	   10		/* 32 bit PC relative offset to GOT */

typedef ZLOX_UINT16 ZLOX_ELF32_HALF;
typedef ZLOX_UINT32 ZLOX_ELF32_OFF;
typedef ZLOX_UINT32 ZLOX_ELF32_ADDR;
typedef ZLOX_UINT32 ZLOX_ELF32_WORD;
typedef ZLOX_SINT32 ZLOX_ELF32_SWORD;

typedef enum _ZLOX_ELF_IDENT {
	ZLOX_EI_MAG0		= 0, // 0x7F
	ZLOX_EI_MAG1		= 1, // 'E'
	ZLOX_EI_MAG2		= 2, // 'L'
	ZLOX_EI_MAG3		= 3, // 'F'
	ZLOX_EI_CLASS		= 4, // Architecture (32/64)
	ZLOX_EI_DATA		= 5, // Byte Order
	ZLOX_EI_VERSION	= 6, // ELF Version
	ZLOX_EI_OSABI		= 7, // OS Specific
	ZLOX_EI_ABIVERSION	= 8, // OS Specific
	ZLOX_EI_PAD		= 9  // Padding
} ZLOX_ELF_IDENT;

typedef enum _ZLOX_ELF_TYPE {
	ZLOX_ET_NONE		= 0, // Unkown Type
	ZLOX_ET_REL		= 1, // Relocatable File
	ZLOX_ET_EXEC		= 2, // Executable File
	ZLOX_ET_DYN		= 3, // Shared object file
} ZLOX_ELF_TYPE;

typedef enum _ZLOX_SHT_TYPES {
	ZLOX_SHT_NULL		= 0,   // Null section
	ZLOX_SHT_PROGBITS	= 1,   // Program information
	ZLOX_SHT_SYMTAB	= 2,   // Symbol table
	ZLOX_SHT_STRTAB	= 3,   // String table
	ZLOX_SHT_RELA		= 4,   // Relocation (w/ addend)
	ZLOX_SHT_NOBITS	= 8,   // Not present in file
	ZLOX_SHT_REL		= 9,   // Relocation (no addend)
} ZLOX_SHT_TYPES;

typedef enum _ZLOX_PHT_TYPES{
	ZLOX_PT_LOAD		= 1,	// Loadable program segment
	ZLOX_PT_DYNAMIC	= 2,	// Dynamic linking information
	ZLOX_PT_INTERP	= 3,	// Program interpreter
	ZLOX_PT_PHDR		= 6,	// Entry for header table itself
} ZLOX_PHT_TYPES;

typedef enum _ZLOX_DYN_TYPES{
	ZLOX_DT_NEEDED	= 1,	// Name of needed library
	ZLOX_DT_PLTRELSZ	= 2,	// Size in bytes of PLT relocs
	ZLOX_DT_PLTGOT	= 3,	// Processor defined value
	ZLOX_DT_HASH		= 4,	// Address of symbol hash table
	ZLOX_DT_STRTAB	= 5,	// Address of string table
	ZLOX_DT_SYMTAB	= 6,	// Address of symbol table
	ZLOX_DT_REL		= 17,	// Address of Rel relocs 
	ZLOX_DT_RELSZ		= 18,	// Total size of Rel relocs 
	ZLOX_DT_PLTREL	= 20,	// Type of reloc in PLT 
	ZLOX_DT_JMPREL	= 23,	// Address of PLT relocs 
} ZLOX_DYN_TYPES;

typedef enum _ZLOX_SHT_ATTRIBUTES {
	ZLOX_SHF_WRITE	= 0x01, // Writable section
	ZLOX_SHF_ALLOC	= 0x02  // Exists in memory
} ZLOX_SHT_ATTRIBUTES;

typedef struct _ZLOX_ELF32_EHDR{
	ZLOX_UINT8		e_ident[ZLOX_ELF_NIDENT];
	ZLOX_ELF32_HALF	e_type;
	ZLOX_ELF32_HALF	e_machine;
	ZLOX_ELF32_WORD	e_version;
	ZLOX_ELF32_ADDR	e_entry;
	ZLOX_ELF32_OFF	e_phoff;
	ZLOX_ELF32_OFF	e_shoff;
	ZLOX_ELF32_WORD	e_flags;
	ZLOX_ELF32_HALF	e_ehsize;
	ZLOX_ELF32_HALF	e_phentsize;
	ZLOX_ELF32_HALF	e_phnum;
	ZLOX_ELF32_HALF	e_shentsize;
	ZLOX_ELF32_HALF	e_shnum;
	ZLOX_ELF32_HALF	e_shstrndx;
} ZLOX_ELF32_EHDR;

typedef struct _ZLOX_ELF32_SHDR{
	ZLOX_ELF32_WORD	sh_name;
	ZLOX_ELF32_WORD	sh_type;
	ZLOX_ELF32_WORD	sh_flags;
	ZLOX_ELF32_ADDR	sh_addr;
	ZLOX_ELF32_OFF	sh_offset;
	ZLOX_ELF32_WORD	sh_size;
	ZLOX_ELF32_WORD	sh_link;
	ZLOX_ELF32_WORD	sh_info;
	ZLOX_ELF32_WORD	sh_addralign;
	ZLOX_ELF32_WORD	sh_entsize;
} ZLOX_ELF32_SHDR;

typedef struct _ZLOX_ELF32_PHDR{
	ZLOX_UINT32		p_type;
	ZLOX_ELF32_OFF	p_offset;
	ZLOX_ELF32_ADDR	p_vaddr;
	ZLOX_ELF32_ADDR	p_paddr;
	ZLOX_UINT32		p_filesz;
	ZLOX_UINT32		p_memsz;
	ZLOX_UINT32		p_flags;
	ZLOX_UINT32		p_align;
} ZLOX_ELF32_PHDR;

typedef struct _ZLOX_ELF32_DYN{
	ZLOX_ELF32_SWORD    d_tag;
	union {
		ZLOX_ELF32_WORD d_val;
		ZLOX_ELF32_ADDR d_ptr;
	} d_un;
} ZLOX_ELF32_DYN;

typedef struct _ZLOX_ELF_KERNEL_MAP{
	ZLOX_BOOL isValid;
	ZLOX_UINT32 index;
	ZLOX_CHAR soname[128];
	ZLOX_VOID * heap;
	ZLOX_UINT32 npage;
	ZLOX_UINT32 msize;
	ZLOX_UINT32 ref_count;
} ZLOX_ELF_KERNEL_MAP;

typedef struct _ZLOX_ELF_KERNEL_MAP_LIST{
	ZLOX_BOOL isInit;
	ZLOX_SINT32 count;
	ZLOX_SINT32 size;
	ZLOX_ELF_KERNEL_MAP * ptr;
} ZLOX_ELF_KERNEL_MAP_LIST;

typedef struct _ZLOX_ELF_DYN_MAP{
	ZLOX_ELF32_DYN * Dynamic;
	ZLOX_VOID * dl_runtime_resolve;
	ZLOX_UINT32 hash;
	ZLOX_UINT32 strtab;
	ZLOX_UINT32 symtab;
	ZLOX_UINT32 plt_got;
	ZLOX_UINT32 jmprel_type;
	ZLOX_UINT32 jmprel;
	ZLOX_UINT32 jmprelsz;
	ZLOX_UINT32 rel;
	ZLOX_UINT32 relsz;
} ZLOX_ELF_DYN_MAP;

typedef struct _ZLOX_ELF_LINK_MAP_LIST ZLOX_ELF_LINK_MAP_LIST;

typedef struct _ZLOX_ELF_LINK_MAP{
	ZLOX_ELF_LINK_MAP_LIST * maplst;
	ZLOX_UINT32 kmap_index;
	ZLOX_UINT32 index;
	ZLOX_CHAR * soname;
	ZLOX_UINT32 entry;
	ZLOX_UINT32 vaddr;
	ZLOX_UINT32 msize;
	ZLOX_ELF_DYN_MAP dyn;
} ZLOX_ELF_LINK_MAP;

struct _ZLOX_ELF_LINK_MAP_LIST{
	ZLOX_BOOL isInit;
	ZLOX_SINT32 count;
	ZLOX_SINT32 size;
	ZLOX_ELF_LINK_MAP * ptr;
};

typedef struct _ZLOX_ELF32_SYM{
	ZLOX_UINT32 st_name;
	ZLOX_ELF32_ADDR st_value;
	ZLOX_UINT32 st_size;
	ZLOX_UINT8 st_info;
	ZLOX_UINT8 st_other;
	ZLOX_UINT16 st_shndx;
} ZLOX_ELF32_SYM;

typedef struct _ZLOX_ELF32_REL{
	ZLOX_ELF32_ADDR r_offset;
	ZLOX_UINT32   r_info;
} ZLOX_ELF32_REL;

ZLOX_SINT32 zlox_elf_free_lnk_maplst(ZLOX_ELF_LINK_MAP_LIST * maplst);

ZLOX_SINT32 zlox_execve(const ZLOX_CHAR * filename);

#endif // _ZLOX_ELF_H_

