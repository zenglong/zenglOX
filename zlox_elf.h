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
	ZLOX_ET_EXEC		= 2  // Executable File
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

ZLOX_SINT32 zlox_execve(const ZLOX_CHAR * filename);

#endif // _ZLOX_ELF_H_

