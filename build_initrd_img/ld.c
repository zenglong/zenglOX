// ld.c -- dynamic loader

#include "common.h"
#include "syscall.h"
#include "elf.h"
#include "task.h"

extern UINT32 _commontest_gl; // debug global value of other dynamic library

VOID _dl_runtime_resolve();

int ret;

// Compare two strings. return 0 if they are equal or 1 otherwise.
static SINT32 dl_strcmp(CHAR * str1, CHAR * str2)
{
	/*SINT32 i = 0;
	SINT32 failed = 0;
	while(str1[i] != '\0' && str2[i] != '\0')
	{
		if(str1[i] != str2[i])
		{
			failed = 1;
			break;
		}
		i++;
	}*/
	// why did the loop exit?
	/*if( (str1[i] == '\0' && str2[i] != '\0') || (str1[i] != '\0' && str2[i] == '\0') )
		failed = 1;
  	*/

	SINT32 retval = 0;

	asm volatile (
		"pushf\n\t"
		"cld\n"
	"start:\n\t"		
		"lodsb\n\t"		// 将esi的第一个字节装入寄存器AL，同时esi的值加1
		"scasb\n\t"		// 将edi的第一个字节和AL相减，同时edi的值加1
		"jne not_eq\n\t"	// 不相等则跳转到not_eq
		"testb %b0,%b0\n\t"	// 判断是否到达字符串的最后一个NULL字符
		"jne start\n\t"		// 如果到了最后一个NULL字符,则说明两个字符串完全相等
		"xorl %0,%0\n\t"	// 字符串相等则将结果设置为0
		"jmp end\n"		
	"not_eq:\n\t"
		"movl $1,%0\n"		// 两字符串不相等,则将结果设置为1
	"end:\n\t"
		"popf"
		:"=a"(retval):"S"(str1),"D"(str2));
	return retval;
}

static UINT32 dl_elf_hash(const CHAR *name)
{
	UINT32 h = 0, g;
	while (*name)
	{
		h = (h << 4) + *name++;
		if (g = h & 0xf0000000)
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}

static ELF32_SYM * dl_elf_find_symbol(UINT32 * hashptr, const CHAR * name, 
						ELF32_SYM * symtab, CHAR * strtab)
{
	UINT32 * bucket = hashptr + 2;
	UINT32 * chain = bucket + hashptr[0];
	UINT32 y = bucket[dl_elf_hash(name) % hashptr[0]];
	CHAR * str;
	while(symtab[y].st_name != 0)
	{
		str = strtab + symtab[y].st_name;
		if(dl_strcmp(str, (CHAR *)name) == 0)
			return &symtab[y];
		y = chain[y];
	}
	return NULL;
}

static BOOL dl_elf_findsym_fromlst(CHAR * name, ELF_LINK_MAP * map, 
				ELF32_SYM ** retsym, ELF_LINK_MAP ** retmap)
{
	UINT32 count = map->maplst->count;
	ELF_LINK_MAP * tmp_map = map->maplst->ptr + 1;
	ELF32_SYM * sym = NULL;
	UINT32 i;
	for(i = 1; i < count ;i++)
	{
		if(tmp_map != map)
		{
			sym = dl_elf_find_symbol((UINT32 *)tmp_map->dyn.hash, name, 
							(ELF32_SYM *)tmp_map->dyn.symtab, (CHAR *)tmp_map->dyn.strtab);
			if(sym != NULL && sym->st_value != 0)
			{
				(*retsym) = sym;
				(*retmap) = tmp_map;
				return TRUE;
			}
		}
		tmp_map++;
	}
	return FALSE;
}

UINT32 dl_fixup(ELF_LINK_MAP * map, UINT32 jmp_off)
{
	ELF32_REL * jmprel = (ELF32_REL *)(map->dyn.jmprel + jmp_off);
	ELF32_SYM * symtab = (ELF32_SYM *)map->dyn.symtab;
	UINT32 * offset = (UINT32 *)(jmprel->r_offset + (map->index != 0 ? map->vaddr : 0));
	UINT32 symindex = ELF32_R_SYM(jmprel->r_info);
	CHAR * name = (CHAR *)(symtab[symindex].st_name + map->dyn.strtab);
	UINT32 value;
	ELF32_SYM * retsym;
	ELF_LINK_MAP * retmap;
	if(dl_elf_findsym_fromlst(name, map, &retsym, &retmap) == FALSE)
	{
		syscall_monitor_write("shared library Error:can't find symbol \"");
		syscall_monitor_write(name);
		syscall_monitor_write("\" \n");
		syscall_exit(-1);
		return FALSE;
	}
	if(retsym->st_value > EXECVE_ADDR && 
		retsym->st_value < ELF_LD_SO_VADDR)
		value = retsym->st_value;
	else
		value = retsym->st_value + (retmap->index != 0 ? retmap->vaddr : 0);
	(*offset) = value;
	return value;
}

int dl_test(int arg)
{
	arg = 2;
	_dl_runtime_resolve(); // must use _dl_runtime_resolve once, or you can't find it in the symbol table !
	return 0;
}

int dl_main(VOID * entry)
{
	ret = 1;
	_commontest_gl = 5; // debug global value of other dynamic library
	if(FALSE)
		dl_test(ret); // we never reach here

	VOID * task = (VOID *)syscall_get_currentTask();
	VOID * location = (VOID *)(((TASK *)task)->link_maps.ptr[0].entry);
	char * args = ((char *)syscall_get_init_esp(task)) - 1;
	char * orig_args = (char *)syscall_get_args(task);
	int arg_num = 0;
	int ret;
	int length = strlen(orig_args);
	int i = length - 1;

	if(args >= orig_args)
		reverse_memcpy((UINT8 *)args,(UINT8 *)(orig_args + length),length + 1);
	else
		memcpy((UINT8 *)(args - length),(UINT8 *)orig_args,length + 1);

	args -= length;
	asm volatile("movl %0,%%esp"::"r"((UINT32)args));

	for(;i>=0;i--)
	{
		if(args[i] == ' ' && args[i+1] != ' ' && args[i+1] != '\0')
		{
			arg_num++;
			asm volatile("push %0" :: "r"((UINT32)(args + i + 1)));
		}
	}

	if(args[0] != ' ' && args[0] != '\0')
	{
		arg_num++;
		asm volatile("push %0" :: "r"((UINT32)args));
	}
	
	for(i=0;i < length;i++)
	{
		if(args[i] == ' ')
		{
			args[i] = '\0';
		}
	}

	asm volatile(" \
		movl %esp,%eax; \
		push %eax; \
		");
	
	asm volatile("push %1; \
			push %2; \
			call *%3; \
			" : "=a" (ret) : "d"(arg_num), "c"((UINT32)task), "0" (location));
	
	syscall_exit(ret);
	return 0;
}

