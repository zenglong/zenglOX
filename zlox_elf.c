/*zlox_elf.c 和ELF可执行文件相关的函数定义*/

#include "zlox_elf.h"
#include "zlox_monitor.h"
#include "zlox_fs.h"
#include "zlox_paging.h"
#include "zlox_task.h"
#include "zlox_kheap.h"

// 所需的zlox_paging.c里的全局变量
extern ZLOX_PAGE_DIRECTORY * current_directory;
extern ZLOX_TASK * current_task;

ZLOX_ELF_KERNEL_MAP_LIST elf_kmaplst = {0};

ZLOX_BOOL zlox_elf_check_file(ZLOX_ELF32_EHDR * hdr)
{
	if(!hdr) 
		return ZLOX_FALSE;
	if(hdr->e_ident[ZLOX_EI_MAG0] != ZLOX_ELFMAG0) {
		zlox_monitor_write("ELF Error:ELF Header EI_MAG0 incorrect.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_ident[ZLOX_EI_MAG1] != ZLOX_ELFMAG1) {
		zlox_monitor_write("ELF Error:ELF Header EI_MAG1 incorrect.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_ident[ZLOX_EI_MAG2] != ZLOX_ELFMAG2) {
		zlox_monitor_write("ELF Error:ELF Header EI_MAG2 incorrect.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_ident[ZLOX_EI_MAG3] != ZLOX_ELFMAG3) {
		zlox_monitor_write("ELF Error:ELF Header EI_MAG3 incorrect.\n");
		return ZLOX_FALSE;
	}
	return ZLOX_TRUE;
}

ZLOX_BOOL zlox_elf_check_supported(ZLOX_ELF32_EHDR *hdr, ZLOX_ELF_TYPE type) {
	if(!zlox_elf_check_file(hdr)) {
		zlox_monitor_write("ELF Error:Invalid ELF File.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_ident[ZLOX_EI_CLASS] != ZLOX_ELFCLASS32) {
		zlox_monitor_write("ELF Error:Unsupported ELF File Class.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_ident[ZLOX_EI_DATA] != ZLOX_ELFDATA2LSB) {
		zlox_monitor_write("ELF Error:Unsupported ELF File byte order.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_machine != ZLOX_EM_386) {
		zlox_monitor_write("ELF Error:Unsupported ELF File target.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_ident[ZLOX_EI_VERSION] != ZLOX_EV_CURRENT) {
		zlox_monitor_write("ELF Error:Unsupported ELF File version.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_shnum == ZLOX_SHN_UNDEF)
	{
		zlox_monitor_write("ELF Error:No section header.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_shstrndx == ZLOX_SHN_UNDEF)
	{
		zlox_monitor_write("ELF Error:No section header string table.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_type != type) {
		zlox_monitor_write("ELF Error:Unsupported ELF File type.\n");
		return ZLOX_FALSE;
	}
	return ZLOX_TRUE;
}

ZLOX_ELF_LINK_MAP * zlox_push_elf_link_map(ZLOX_UINT32 vaddr, ZLOX_UINT32 msize)
{
	ZLOX_ELF_LINK_MAP_LIST * maps = &(current_task->link_maps);
	if(!maps->isInit) // 如果没进行过初始化，则初始化link map list
	{
		maps->size = ZLOX_ELF_LINK_MAP_LIST_SIZE;
		maps->ptr = (ZLOX_ELF_LINK_MAP *)zlox_kmalloc(maps->size * sizeof(ZLOX_ELF_LINK_MAP));
		zlox_memset((ZLOX_UINT8 *)maps->ptr,0,maps->size * sizeof(ZLOX_ELF_LINK_MAP));		
		maps->count = 0;
		maps->isInit = ZLOX_TRUE;
	}
	else if(maps->count == maps->size)
	{
		ZLOX_ELF_LINK_MAP * tmp_ptr = maps->ptr;
		maps->size += ZLOX_ELF_LINK_MAP_LIST_SIZE;
		maps->ptr = (ZLOX_ELF_LINK_MAP *)zlox_kmalloc(maps->size * sizeof(ZLOX_ELF_LINK_MAP));
		zlox_memcpy((ZLOX_UINT8 *)maps->ptr,(ZLOX_UINT8 *)tmp_ptr, maps->count * sizeof(ZLOX_ELF_LINK_MAP));
		zlox_memset((ZLOX_UINT8 *)(maps->ptr + maps->count), 0,
				ZLOX_ELF_LINK_MAP_LIST_SIZE * sizeof(ZLOX_ELF_LINK_MAP));
	}
	
	maps->ptr[maps->count].vaddr = vaddr;
	maps->ptr[maps->count].msize = msize;
	maps->ptr[maps->count].index = maps->count;
	maps->count++;
	return &maps->ptr[maps->count - 1];
}

ZLOX_ELF_KERNEL_MAP * zlox_add_elf_kernel_map(ZLOX_CHAR * soname, ZLOX_VOID * task, ZLOX_UINT32 vaddr, ZLOX_UINT32 msize)
{
	ZLOX_ELF_KERNEL_MAP_LIST * maps = &elf_kmaplst;
	if(!maps->isInit) // 如果没进行过初始化，则初始化kernel map list
	{
		maps->size = ZLOX_ELF_KERNEL_MAP_LIST_SIZE;
		maps->ptr = (ZLOX_ELF_KERNEL_MAP *)zlox_kmalloc(maps->size * sizeof(ZLOX_ELF_KERNEL_MAP));
		zlox_memset((ZLOX_UINT8 *)maps->ptr,0,maps->size * sizeof(ZLOX_ELF_KERNEL_MAP));	
		maps->count = 0;
		maps->isInit = ZLOX_TRUE;
	}
	else if(maps->count == maps->size)
	{
		ZLOX_ELF_KERNEL_MAP * tmp_ptr = maps->ptr;
		maps->size += ZLOX_ELF_KERNEL_MAP_LIST_SIZE;
		maps->ptr = (ZLOX_ELF_KERNEL_MAP *)zlox_kmalloc(maps->size * sizeof(ZLOX_ELF_KERNEL_MAP));
		zlox_memcpy((ZLOX_UINT8 *)maps->ptr,(ZLOX_UINT8 *)tmp_ptr, maps->count * sizeof(ZLOX_ELF_KERNEL_MAP));
		zlox_memset((ZLOX_UINT8 *)(maps->ptr + maps->count), 0,
				ZLOX_ELF_KERNEL_MAP_LIST_SIZE * sizeof(ZLOX_ELF_KERNEL_MAP));
	}
	
	ZLOX_SINT32 i;
	for(i = 0; i < maps->size ;i++)
	{
		if(maps->ptr[i].isValid == ZLOX_FALSE)
		{
			zlox_strcpy(maps->ptr[i].soname, soname);
			maps->ptr[i].heap = zlox_pages_map_to_heap(task, vaddr, msize, ZLOX_TRUE, &maps->ptr[i].npage);
			maps->ptr[i].msize = msize;
			maps->ptr[i].index = i;
			maps->ptr[i].ref_count = 1;
			maps->ptr[i].isValid = ZLOX_TRUE;
			maps->count++;
			return &maps->ptr[i];
		}
	}
	
	zlox_monitor_write("shared library Error: when load \"");
	zlox_monitor_write(soname);
	zlox_monitor_write("\" can't find empty kernel map!\n");
	return ZLOX_NULL;
}

ZLOX_ELF_LINK_MAP * zlox_exists_link_map(ZLOX_CHAR * soname)
{
	ZLOX_ELF_LINK_MAP_LIST * maps = &current_task->link_maps;
	if(!maps->isInit)
		return ZLOX_NULL;

	ZLOX_SINT32 i;
	for(i = 0; i < maps->count; i++)
	{
		if(maps->ptr[i].soname == ZLOX_NULL)
			continue;
		else if(zlox_strcmp(maps->ptr[i].soname, soname) == 0)
			return &maps->ptr[i];
	}

	return ZLOX_NULL;
}

ZLOX_ELF_KERNEL_MAP * zlox_exists_kernel_map(ZLOX_CHAR * soname)
{
	if(!elf_kmaplst.isInit)
		return ZLOX_NULL;

	ZLOX_SINT32 i;
	for(i = 0; i < elf_kmaplst.count; i++)
	{
		if(elf_kmaplst.ptr[i].isValid == ZLOX_FALSE)
			continue;
		else if(zlox_strcmp(elf_kmaplst.ptr[i].soname, soname) == 0)
			return &elf_kmaplst.ptr[i];
	}

	return ZLOX_NULL;
}

ZLOX_UINT8 * zlox_shlib_readfile(ZLOX_CHAR * soname)
{
	ZLOX_FS_NODE * fsnode = zlox_finddir_fs(fs_root, soname);
	if ((fsnode->flags & 0x7) != ZLOX_FS_FILE)
	{
		goto not_valid_shlib;
	}
	ZLOX_UINT8 * buf = (ZLOX_UINT8 *)zlox_kmalloc(fsnode->length + 10);
	ZLOX_UINT32 sz = zlox_read_fs(fsnode, 0, fsnode->length, buf);
	if((sz > 0) && zlox_elf_check_supported((ZLOX_ELF32_EHDR *)buf, ZLOX_ET_DYN) )
	{
		return buf;
	}
	else
	{
		zlox_kfree(buf);
not_valid_shlib:
		zlox_monitor_write("shared library Error:\"");
		zlox_monitor_write(soname);
		zlox_monitor_write("\" is not a valid shared library!\n");
		return ZLOX_NULL;
	}
}

ZLOX_UINT32 zlox_shlib_loadToVAddr(ZLOX_ELF32_EHDR *hdr, ZLOX_UINT8 * buf, ZLOX_UINT32 vaddr)
{
	ZLOX_ELF32_PHDR * phdr = (ZLOX_ELF32_PHDR *)((ZLOX_UINT32)hdr + hdr->e_phoff);
	ZLOX_UINT32 i, first_load = 0, last_load = 0;
	ZLOX_BOOL isFirst = ZLOX_TRUE;
	for(i = 0; i < hdr->e_phnum; i++)
	{
		switch(phdr[i].p_type)
		{
		case ZLOX_PT_LOAD:
			zlox_pages_alloc((phdr[i].p_vaddr + vaddr), phdr[i].p_memsz);
			zlox_memcpy((ZLOX_UINT8 *)(phdr[i].p_vaddr + vaddr),
					(ZLOX_UINT8 *)(buf + phdr[i].p_offset),phdr[i].p_filesz);
			if(phdr[i].p_memsz > phdr[i].p_filesz)
			{
				zlox_memset((ZLOX_UINT8 *)(phdr[i].p_vaddr + vaddr + phdr[i].p_filesz), 0 , 
						(phdr[i].p_memsz - phdr[i].p_filesz));
			}
			if(isFirst)
			{
				first_load = i;
				isFirst = ZLOX_FALSE;
			}
			last_load = i;
			break;
		}
	}
	ZLOX_UINT32 msize = (phdr[last_load].p_vaddr + phdr[last_load].p_memsz) - phdr[first_load].p_vaddr;
	return msize;
}

ZLOX_UINT32 zlox_elf_hash(const ZLOX_CHAR *name)
{
	ZLOX_UINT32 h = 0, g;
	while (*name)
	{
		h = (h << 4) + *name++;
		if ((g = h & 0xf0000000))
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}

ZLOX_ELF32_SYM * zlox_elf_find_symbol(ZLOX_UINT32 * hashptr, const ZLOX_CHAR * name, ZLOX_ELF32_SYM * symtab, ZLOX_CHAR * strtab)
{
	ZLOX_UINT32 * bucket = hashptr + 2;
	ZLOX_UINT32 * chain = bucket + hashptr[0];
	ZLOX_UINT32 y = bucket[zlox_elf_hash(name) % hashptr[0]];
	ZLOX_CHAR * str;
	while(symtab[y].st_name != 0)
	{
		str = strtab + symtab[y].st_name;
		if(zlox_strcmp(str, (ZLOX_CHAR *)name) == 0)
			return &symtab[y];
		y = chain[y];
	}
	return ZLOX_NULL;
}

ZLOX_ELF_LINK_MAP * zlox_elf_make_link_map(ZLOX_UINT32 kmap_index, ZLOX_CHAR * soname, ZLOX_UINT32 vaddr, ZLOX_UINT32 msize)
{
	ZLOX_ELF32_EHDR * hdr = (ZLOX_ELF32_EHDR *)vaddr;
	ZLOX_ELF32_PHDR * phdr = (ZLOX_ELF32_PHDR *)((ZLOX_UINT32)hdr + hdr->e_phoff);
	ZLOX_ELF32_DYN * Dynamic = ZLOX_NULL;
	ZLOX_ELF_LINK_MAP * tmp_map;
	tmp_map = zlox_push_elf_link_map(vaddr, msize);
	tmp_map->maplst = &current_task->link_maps;
	tmp_map->kmap_index = kmap_index;
	tmp_map->soname = soname;
	tmp_map->entry = hdr->e_entry + (tmp_map->index != 0 ? vaddr : 0);
	for(ZLOX_UINT32 i = 0; i < hdr->e_phnum; i++)
	{
		if(phdr[i].p_type == ZLOX_PT_DYNAMIC)
		{
			tmp_map->dyn.Dynamic = Dynamic = (ZLOX_ELF32_DYN *)((tmp_map->index != 0 ? vaddr : 0) + phdr[i].p_vaddr);
			break;
		}
	}
	if(Dynamic == ZLOX_NULL)
		return tmp_map;
	while(Dynamic->d_tag != 0)
	{
		switch(Dynamic->d_tag)
		{
		case ZLOX_DT_HASH:
			tmp_map->dyn.hash = Dynamic->d_un.d_ptr + (tmp_map->index != 0 ? vaddr : 0);
			break;
		case ZLOX_DT_STRTAB:
			tmp_map->dyn.strtab = Dynamic->d_un.d_ptr + (tmp_map->index != 0 ? vaddr : 0);
			break;
		case ZLOX_DT_SYMTAB:
			tmp_map->dyn.symtab = Dynamic->d_un.d_ptr + (tmp_map->index != 0 ? vaddr : 0);
			break;
		case ZLOX_DT_PLTGOT:
			tmp_map->dyn.plt_got = Dynamic->d_un.d_ptr + (tmp_map->index != 0 ? vaddr : 0);
			break;
		case ZLOX_DT_PLTREL:
			tmp_map->dyn.jmprel_type = Dynamic->d_un.d_val;
			break;
		case ZLOX_DT_JMPREL:
			tmp_map->dyn.jmprel = Dynamic->d_un.d_ptr + (tmp_map->index != 0 ? vaddr : 0);
			break;
		case ZLOX_DT_PLTRELSZ:
			tmp_map->dyn.jmprelsz = Dynamic->d_un.d_val;
			break;
		case ZLOX_DT_REL:
			tmp_map->dyn.rel = Dynamic->d_un.d_ptr + (tmp_map->index != 0 ? vaddr : 0);
			break;
		case ZLOX_DT_RELSZ:
			tmp_map->dyn.relsz = Dynamic->d_un.d_val;
			break;
		}
		Dynamic++;
	}
	if(tmp_map->index == 1)
	{
		ZLOX_ELF32_SYM * retsym = zlox_elf_find_symbol((ZLOX_UINT32 *)tmp_map->dyn.hash, ZLOX_ELF_LD_RESOLVE_NAME, 
								(ZLOX_ELF32_SYM *)tmp_map->dyn.symtab, (ZLOX_CHAR *)tmp_map->dyn.strtab);
		if(retsym == ZLOX_NULL)
		{
			zlox_monitor_write("shared library Error:no _dl_runtime_resolve function in \"");
			zlox_monitor_write(soname);
			zlox_monitor_write("\" \n");
			return ZLOX_NULL;
		}
		tmp_map->dyn.dl_runtime_resolve = (ZLOX_VOID *)(retsym->st_value + vaddr);
	}
	else if(tmp_map->index > 1)
	{
		tmp_map->dyn.dl_runtime_resolve = tmp_map->maplst->ptr[1].dyn.dl_runtime_resolve;
	}
	return tmp_map;
}

ZLOX_BOOL zlox_elf_findsym_fromlst(ZLOX_CHAR * name, ZLOX_ELF_LINK_MAP * map, 
				ZLOX_ELF32_SYM ** retsym, ZLOX_ELF_LINK_MAP ** retmap)
{
	ZLOX_SINT32 count = map->maplst->count;
	ZLOX_ELF_LINK_MAP * tmp_map = map->maplst->ptr + 1;
	ZLOX_ELF32_SYM * sym = ZLOX_NULL;
	for(ZLOX_SINT32 i = 1; i < count ;i++)
	{
		if(tmp_map != map)
		{
			sym = zlox_elf_find_symbol((ZLOX_UINT32 *)tmp_map->dyn.hash, name, 
							(ZLOX_ELF32_SYM *)tmp_map->dyn.symtab, (ZLOX_CHAR *)tmp_map->dyn.strtab);
			if(sym != ZLOX_NULL && sym->st_value != 0)
			{
				(*retsym) = sym;
				(*retmap) = tmp_map;
				return ZLOX_TRUE;
			}
		}
		tmp_map++;
	}
	return ZLOX_FALSE;
}

ZLOX_BOOL zlox_elf_reloc(ZLOX_ELF32_REL * rel, ZLOX_UINT32 relsz, ZLOX_ELF32_SYM * symtab, ZLOX_ELF_LINK_MAP * map)
{
	relsz = relsz / sizeof(ZLOX_ELF32_REL);
	for(ZLOX_UINT32 i = 0;i < relsz;i++)
	{
		ZLOX_UINT32 symindex = ZLOX_ELF32_R_SYM(rel[i].r_info);
		ZLOX_UINT8 type = ZLOX_ELF32_R_TYPE(rel[i].r_info);
		switch(type)
		{
		case ZLOX_R_386_32:
		case ZLOX_R_386_PC32:
		case ZLOX_R_386_GLOB_DAT:
			{
				ZLOX_UINT32 * offset = (ZLOX_UINT32 *)(rel[i].r_offset + (map->index != 0 ? map->vaddr : 0));
				ZLOX_UINT32 value;
				if(symtab[symindex].st_value != 0)
				{
					if(symtab[symindex].st_value > ZLOX_EXECVE_ADDR && 
						symtab[symindex].st_value < ZLOX_ELF_LD_SO_VADDR)
						value = symtab[symindex].st_value;
					else
						value = symtab[symindex].st_value + (map->index != 0 ? map->vaddr : 0);
					zlox_page_copy((ZLOX_UINT32)offset);
					if(type == ZLOX_R_386_PC32)
						value = (value - ((ZLOX_UINT32)offset + 4)) & 0xFFFFFFFF;
					(*offset) = value;
				}
				else
				{
					ZLOX_CHAR * name = (ZLOX_CHAR *)(symtab[symindex].st_name + map->dyn.strtab);
					ZLOX_ELF32_SYM * retsym;
					ZLOX_ELF_LINK_MAP * retmap;
					if(zlox_elf_findsym_fromlst(name, map, &retsym, &retmap) == ZLOX_FALSE)
					{
						zlox_monitor_write("shared library Error:can't find symbol \"");
						zlox_monitor_write(name);
						zlox_monitor_write("\" \n");
						return ZLOX_FALSE;
					}
					if(retsym->st_value > ZLOX_EXECVE_ADDR && 
						retsym->st_value < ZLOX_ELF_LD_SO_VADDR)
						value = retsym->st_value;
					else
						value = retsym->st_value + (retmap->index != 0 ? retmap->vaddr : 0);
					zlox_page_copy((ZLOX_UINT32)offset);
					if(type == ZLOX_R_386_PC32)
						value = (value - ((ZLOX_UINT32)offset + 4)) & 0xFFFFFFFF;
					(*offset) = value;
				}
			}
			break;
		case ZLOX_R_386_COPY:
			{
				ZLOX_UINT32 value = symtab[symindex].st_value + (map->index != 0 ? map->vaddr : 0);
				ZLOX_CHAR * name = (ZLOX_CHAR *)(symtab[symindex].st_name + map->dyn.strtab);
				ZLOX_ELF32_SYM * retsym;
				ZLOX_ELF_LINK_MAP * retmap;
				if(zlox_elf_findsym_fromlst(name, map, &retsym, &retmap) == ZLOX_FALSE)
				{
					zlox_monitor_write("shared library Error:can't find symbol \"");
					zlox_monitor_write(name);
					zlox_monitor_write("\" \n");
					return ZLOX_FALSE;
				}
				ZLOX_ELF32_ADDR * destaddr =  &retsym->st_value;
				zlox_page_copy((ZLOX_UINT32)destaddr);
				(*destaddr) = value;
			}
			break;
		case ZLOX_R_386_JMP_SLOT:
			{
				ZLOX_UINT32 * offset = (ZLOX_UINT32 *)(rel[i].r_offset + (map->index != 0 ? map->vaddr : 0));
				ZLOX_UINT32 value;
				if(symtab[symindex].st_value != 0)
				{
					if(symtab[symindex].st_value > ZLOX_EXECVE_ADDR && 
						symtab[symindex].st_value < ZLOX_ELF_LD_SO_VADDR)
						value = symtab[symindex].st_value;
					else
						value = symtab[symindex].st_value + (map->index != 0 ? map->vaddr : 0);
					zlox_page_copy((ZLOX_UINT32)offset);
					(*offset) = value;
				}
				else
				{
					value = (*offset) + (map->index != 0 ? map->vaddr : 0);
					zlox_page_copy((ZLOX_UINT32)offset);
					(*offset) = value;
				}
			}
			break;
		}
	}
	return ZLOX_TRUE;
}

ZLOX_BOOL zlox_elf_relocation(ZLOX_ELF_LINK_MAP * map)
{
	if(map->dyn.relsz > 0)
	{
		if(zlox_elf_reloc((ZLOX_ELF32_REL *)map->dyn.rel, map->dyn.relsz, 
				(ZLOX_ELF32_SYM *)map->dyn.symtab, map) == ZLOX_FALSE)
		{
			return ZLOX_FALSE;
		}
	}
	
	if(map->dyn.jmprelsz > 0)
	{
		if(map->dyn.jmprel_type == ZLOX_DT_REL)
			if(zlox_elf_reloc((ZLOX_ELF32_REL *)map->dyn.jmprel, map->dyn.jmprelsz, 
					(ZLOX_ELF32_SYM *)map->dyn.symtab, map) == ZLOX_FALSE)
			{
				return ZLOX_FALSE;
			}
	}

	return ZLOX_TRUE;
}

ZLOX_BOOL zlox_elf_set_plt_got(ZLOX_ELF_LINK_MAP * map)
{
	if(map->dyn.plt_got != 0)
	{
		ZLOX_UINT32 * plt_got = (ZLOX_UINT32 *)map->dyn.plt_got;
		zlox_page_copy((ZLOX_UINT32)plt_got);
		plt_got[1] = (ZLOX_UINT32)map;
		plt_got[2] = (ZLOX_UINT32)map->dyn.dl_runtime_resolve;
	}
	return ZLOX_TRUE;
}

ZLOX_UINT32 zlox_load_shared_library(ZLOX_CHAR * soname, ZLOX_UINT32 vaddr)
{
	ZLOX_ELF_LINK_MAP * map = zlox_exists_link_map(soname);
	if(map != ZLOX_NULL)
		return (ZLOX_UINT32)map->entry;

	ZLOX_ELF_KERNEL_MAP * kmap = zlox_exists_kernel_map(soname);
	if(kmap != ZLOX_NULL)
	{
		zlox_pages_map(vaddr, (ZLOX_PAGE *)kmap->heap, kmap->npage);
		kmap->ref_count++;
	}
	else
	{
		ZLOX_UINT8 * buf = zlox_shlib_readfile(soname);
		if(buf == ZLOX_NULL)
			return 0;
		ZLOX_UINT32 msize = zlox_shlib_loadToVAddr((ZLOX_ELF32_EHDR *)buf, (ZLOX_UINT8 *)buf, vaddr);
		kmap = zlox_add_elf_kernel_map(soname, current_task, vaddr, msize);
		if(kmap == ZLOX_NULL)
		{
			zlox_kfree(buf);
			return 0;
		}
		zlox_kfree(buf);
	}

	map = zlox_elf_make_link_map(kmap->index, kmap->soname, vaddr, kmap->msize);
	if(map == ZLOX_NULL)
		return 0;
	
	ZLOX_ELF32_DYN * Dynamic = map->dyn.Dynamic;
	while(Dynamic->d_tag != 0)
	{
		if(Dynamic->d_tag == ZLOX_DT_NEEDED)
		{
			ZLOX_CHAR * tmp_soname = (ZLOX_CHAR *)(Dynamic->d_un.d_val + map->dyn.strtab);
			ZLOX_UINT32 tmp_vaddr = map->maplst->ptr[map->maplst->count - 1].vaddr;
			tmp_vaddr += map->maplst->ptr[map->maplst->count - 1].msize;
			tmp_vaddr = ZLOX_NEXT_PAGE_START(tmp_vaddr);
			if(zlox_load_shared_library(tmp_soname, tmp_vaddr) == 0)
			{
				zlox_monitor_write("shared library Error:load \"");
				zlox_monitor_write(tmp_soname);
				zlox_monitor_write("\" failed \n");
				return 0;
			}
		}
		Dynamic++;
	}
	return map->entry;
}

ZLOX_UINT32 zlox_load_elf(ZLOX_ELF32_EHDR *hdr,ZLOX_UINT8 * buf)
{
	ZLOX_ELF32_PHDR * phdr = (ZLOX_ELF32_PHDR *)((ZLOX_UINT32)hdr + hdr->e_phoff);
	ZLOX_CHAR * interp = ZLOX_NULL;
	ZLOX_UINT32 i,vaddr = 0,last_load,ret;
	for(i = 0; i < hdr->e_phnum; i++)
	{
		switch(phdr[i].p_type)
		{
		case ZLOX_PT_LOAD:
			zlox_pages_alloc(phdr[i].p_vaddr, phdr[i].p_memsz);
			zlox_memcpy((ZLOX_UINT8 *)phdr[i].p_vaddr,
					(ZLOX_UINT8 *)(buf + phdr[i].p_offset),phdr[i].p_filesz);
			if(phdr[i].p_memsz > phdr[i].p_filesz)
			{
				zlox_memset((ZLOX_UINT8 *)(phdr[i].p_vaddr + phdr[i].p_filesz), 0 , 
						(phdr[i].p_memsz - phdr[i].p_filesz));
			}
			if(vaddr == 0)
				vaddr = phdr[i].p_vaddr;
			last_load = i;
			break;
		case ZLOX_PT_INTERP:
			interp = (ZLOX_CHAR *)(buf + phdr[i].p_offset);
			break;
		}
	}
	
	ZLOX_UINT32 msize = (phdr[last_load].p_vaddr + phdr[last_load].p_memsz) - vaddr;
	ZLOX_ELF_LINK_MAP * map = zlox_elf_make_link_map(0, ZLOX_NULL, vaddr, msize);

	ret = hdr->e_entry;

	if(interp != ZLOX_NULL)
	{
		ret = zlox_load_shared_library(interp, ZLOX_ELF_LD_SO_VADDR);
		if(ret == 0)
			return 0;
		ZLOX_ELF32_DYN * Dynamic = map->dyn.Dynamic;
		if(Dynamic != 0)
			while(Dynamic->d_tag != 0)
			{
				if(Dynamic->d_tag == ZLOX_DT_NEEDED)
				{
					ZLOX_CHAR * tmp_soname = (ZLOX_CHAR *)(Dynamic->d_un.d_val + map->dyn.strtab);
					ZLOX_UINT32 tmp_vaddr = map->maplst->ptr[map->maplst->count - 1].vaddr;
					tmp_vaddr += map->maplst->ptr[map->maplst->count - 1].msize;
					tmp_vaddr = ZLOX_NEXT_PAGE_START(tmp_vaddr);
					if(zlox_load_shared_library(tmp_soname, tmp_vaddr) == 0)
					{
						zlox_monitor_write("shared library Error:load \"");
						zlox_monitor_write(tmp_soname);
						zlox_monitor_write("\" failed \n");
						return 0;
					}
				}
				Dynamic++;
			}
		map->dyn.dl_runtime_resolve = map->maplst->ptr[1].dyn.dl_runtime_resolve;
	}

	ZLOX_ELF_LINK_MAP * tmp_map = map->maplst->ptr;
	for(i = 0; (ZLOX_SINT32)i < map->maplst->count; i++)
	{
		if(zlox_elf_relocation(tmp_map) == ZLOX_FALSE)
			return 0;
		zlox_elf_set_plt_got(tmp_map);
		tmp_map++;
	}
	return ret;
}

ZLOX_SINT32 zlox_elf_free_lnk_maplst(ZLOX_ELF_LINK_MAP_LIST * maplst)
{
	ZLOX_ELF_LINK_MAP * tmp_map;
	for(ZLOX_SINT32 i = 1; i < maplst->count; i++)
	{
		tmp_map = maplst->ptr + i;
		ZLOX_UINT32 kmap_index = tmp_map->kmap_index;
		elf_kmaplst.ptr[kmap_index].ref_count = (elf_kmaplst.ptr[kmap_index].ref_count > 0 ? 
						(elf_kmaplst.ptr[kmap_index].ref_count - 1) : 0);
		if(elf_kmaplst.ptr[kmap_index].ref_count == 0)
		{
			ZLOX_PAGE * page = (ZLOX_PAGE *)elf_kmaplst.ptr[kmap_index].heap;
			ZLOX_UINT32 npage = elf_kmaplst.ptr[kmap_index].npage;
			for(ZLOX_UINT32 j=0; j < npage ;j++)
			{
				zlox_free_frame(page);
				page++;
			}
			zlox_kfree(elf_kmaplst.ptr[kmap_index].heap);
			zlox_memset((ZLOX_UINT8 *)&elf_kmaplst.ptr[kmap_index], 0, sizeof(ZLOX_ELF_KERNEL_MAP));
		}
	}
	return 0;
}

static ZLOX_SINT32 zlox_set_filename(ZLOX_CHAR ** filename)
{
	ZLOX_SINT32 i;
	for(i=0;(*filename)[i]!='\0';i++)
	{
		if((*filename)[i] != ' ')
			break;
	}
	(*filename) = (*filename) + i;
	ZLOX_CHAR * tmp_filename = (*filename);
	for(i=0;tmp_filename[i]!='\0';i++)
	{
		if(tmp_filename[i] == ' ')
		{
			tmp_filename[i] = '\0';
			return i;
		}
	}
	return -1;
}

static ZLOX_SINT32 zlox_restore_filename(ZLOX_CHAR * filename,ZLOX_SINT32 pos)
{
	if(pos > 0)
		filename[pos] = ' ';
	return 0;
}

ZLOX_SINT32 zlox_execve(const ZLOX_CHAR * filename)
{
	ZLOX_SINT32 ret_pos = zlox_set_filename((ZLOX_CHAR **)&filename);
	ZLOX_FS_NODE *fsnode = zlox_finddir_fs(fs_root, (ZLOX_CHAR *)filename);

	if ((fsnode->flags & 0x7) == ZLOX_FS_FILE)
	{
		ZLOX_SINT32 ret = zlox_fork();
		if(ret == 0)
		{
			ZLOX_CHAR * buf = (ZLOX_CHAR *)zlox_kmalloc(fsnode->length + 10);
			ZLOX_UINT32 sz = zlox_read_fs(fsnode, 0, fsnode->length, (ZLOX_UINT8 *)buf);
			ZLOX_UINT32 addr;

			if((sz > 0) && zlox_elf_check_supported((ZLOX_ELF32_EHDR *)buf, ZLOX_ET_EXEC) )
			{
				if((addr = zlox_load_elf((ZLOX_ELF32_EHDR *)buf,(ZLOX_UINT8 *)buf)) > 0)
				{
					zlox_kfree(buf);
					//zlox_monitor_write("execve:\"");
					//zlox_monitor_write(filename);
					//zlox_monitor_write("\" is a valid ELF file and load success !\n");
					zlox_restore_filename((ZLOX_CHAR *)filename,ret_pos);
					ZLOX_UINT32 filename_len = zlox_strlen((ZLOX_CHAR *)filename);
					current_task->args = (ZLOX_CHAR *)zlox_kmalloc(filename_len + 1);
					zlox_strcpy(current_task->args,filename);
					return addr;
				}
				else
				{
					zlox_kfree(buf);
					zlox_monitor_write("execve:\"");
					zlox_monitor_write(filename);
					zlox_monitor_write("\" load failed !\n");
					zlox_exit(-1);
					return -1;
				}				
			}
			else
			{
				zlox_kfree(buf);
				zlox_monitor_write("execve Error:\"");
				zlox_monitor_write(filename);
				zlox_monitor_write("\" is not a valid ELF file!\n");
				zlox_exit(-1);
				return -1;
			}
		}
		zlox_restore_filename((ZLOX_CHAR *)filename,ret_pos);
		return 0;
	}
	else if((fsnode->flags & 0x7) == ZLOX_FS_DIRECTORY)
	{
		zlox_monitor_write("execve Error:\"");
		zlox_monitor_write(filename);
		zlox_monitor_write("\" is a directory , it's must be an ELF file \n");
		zlox_restore_filename((ZLOX_CHAR *)filename,ret_pos);
		return -1;
	}
	else
	{
		zlox_monitor_write("execve Error:\"");
		zlox_monitor_write(filename);
		zlox_monitor_write("\" is not exist in your file system! \n");
		zlox_restore_filename((ZLOX_CHAR *)filename,ret_pos);
		return -1;
	}

	return -1;
}

