/*zlox_elf.c 和ELF可执行文件相关的函数定义*/

#include "zlox_elf.h"
#include "zlox_monitor.h"
#include "zlox_fs.h"
#include "zlox_paging.h"
#include "zlox_task.h"
#include "zlox_kheap.h"
#include "zlox_isr.h"

// 所需的zlox_paging.c里的全局变量
extern ZLOX_PAGE_DIRECTORY * current_directory;
extern ZLOX_TASK * current_task;

ZLOX_ELF_KERNEL_MAP_LIST elf_kmaplst = {0};

static ZLOX_SINT32 zlox_elf_print_msg(const ZLOX_CHAR * output)
{
	if(zlox_cmd_window_write(output) == -1)
		zlox_monitor_write(output);
	return 0;
}

ZLOX_BOOL zlox_elf_check_file(ZLOX_ELF32_EHDR * hdr)
{
	if(!hdr) 
		return ZLOX_FALSE;
	if(hdr->e_ident[ZLOX_EI_MAG0] != ZLOX_ELFMAG0) {
		zlox_elf_print_msg("ELF Error:ELF Header EI_MAG0 incorrect.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_ident[ZLOX_EI_MAG1] != ZLOX_ELFMAG1) {
		zlox_elf_print_msg("ELF Error:ELF Header EI_MAG1 incorrect.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_ident[ZLOX_EI_MAG2] != ZLOX_ELFMAG2) {
		zlox_elf_print_msg("ELF Error:ELF Header EI_MAG2 incorrect.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_ident[ZLOX_EI_MAG3] != ZLOX_ELFMAG3) {
		zlox_elf_print_msg("ELF Error:ELF Header EI_MAG3 incorrect.\n");
		return ZLOX_FALSE;
	}
	return ZLOX_TRUE;
}

ZLOX_BOOL zlox_elf_check_supported(ZLOX_ELF32_EHDR *hdr, ZLOX_ELF_TYPE type) {
	if(!zlox_elf_check_file(hdr)) {
		zlox_elf_print_msg("ELF Error:Invalid ELF File.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_ident[ZLOX_EI_CLASS] != ZLOX_ELFCLASS32) {
		zlox_elf_print_msg("ELF Error:Unsupported ELF File Class.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_ident[ZLOX_EI_DATA] != ZLOX_ELFDATA2LSB) {
		zlox_elf_print_msg("ELF Error:Unsupported ELF File byte order.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_machine != ZLOX_EM_386) {
		zlox_elf_print_msg("ELF Error:Unsupported ELF File target.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_ident[ZLOX_EI_VERSION] != ZLOX_EV_CURRENT) {
		zlox_elf_print_msg("ELF Error:Unsupported ELF File version.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_shnum == ZLOX_SHN_UNDEF)
	{
		zlox_elf_print_msg("ELF Error:No section header.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_shstrndx == ZLOX_SHN_UNDEF)
	{
		zlox_elf_print_msg("ELF Error:No section header string table.\n");
		return ZLOX_FALSE;
	}
	if(hdr->e_type != type) {
		zlox_elf_print_msg("ELF Error:Unsupported ELF File type.\n");
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
	// 当动态数组里的元素个数等于可用容量时，
	// 就对数组进行动态扩容
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
			// 通过zlox_pages_map_to_heap函数，将动态链接库
			// 的虚拟内存页表映射到heap堆里，这样，所有依赖该
			// 动态链接库的任务都可以直接从该heap堆里将动态库的页表项
			// 映射到自己的页表项中，除了heap堆里的页表项的读写位为1外，
			// 其他所有依赖该动态链接库的任务的页表项的读写位都为0，这样，
			// 当对该动态库进行写入操作时，将会触发写时复制。
			maps->ptr[i].heap = zlox_pages_map_to_heap(task, vaddr, msize, ZLOX_TRUE, &maps->ptr[i].npage);
			maps->ptr[i].msize = msize;
			maps->ptr[i].index = i;
			maps->ptr[i].ref_count = 1;
			maps->ptr[i].isValid = ZLOX_TRUE;
			maps->count++;
			return &maps->ptr[i];
		}
	}
	
	zlox_elf_print_msg("shared library Error: when load \"");
	zlox_elf_print_msg(soname);
	zlox_elf_print_msg("\" can't find empty kernel map!\n");
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
	//ZLOX_FS_NODE * fsnode = zlox_finddir_fs(fs_root, soname);
	ZLOX_FS_NODE fsnode;
	zlox_memset(((ZLOX_UINT8 *)&fsnode), 0, sizeof(ZLOX_FS_NODE));
	zlox_finddir_fs_safe(fs_root, soname, &fsnode);

	if ((fsnode.flags & 0x7) != ZLOX_FS_FILE)
	{
		goto not_valid_shlib;
	}
	ZLOX_UINT8 * buf = (ZLOX_UINT8 *)zlox_kmalloc(fsnode.length + 10);
	ZLOX_UINT32 sz = zlox_read_fs(&fsnode, 0, fsnode.length, buf);
	if((sz > 0) && zlox_elf_check_supported((ZLOX_ELF32_EHDR *)buf, ZLOX_ET_DYN) )
	{
		return buf;
	}
	else
	{
		zlox_kfree(buf);
not_valid_shlib:
		zlox_elf_print_msg("shared library Error:\"");
		zlox_elf_print_msg(soname);
		zlox_elf_print_msg("\" is not a valid shared library!\n");
		return ZLOX_NULL;
	}
}

// 通过zlox_shlib_loadToVAddr函数将buf文件缓冲里需要加载的数据
// 给加载到vaddr指定的起始虚拟内存位置
ZLOX_UINT32 zlox_shlib_loadToVAddr(ZLOX_ELF32_EHDR *hdr, ZLOX_UINT8 * buf, ZLOX_UINT32 vaddr)
{
	ZLOX_ELF32_PHDR * phdr = (ZLOX_ELF32_PHDR *)((ZLOX_UINT32)hdr + hdr->e_phoff);
	ZLOX_UINT32 i, first_load = 0, last_load = 0;
	ZLOX_BOOL isFirst = ZLOX_TRUE;
	// 循环遍历动态链接库的每个program header结构
	for(i = 0; i < hdr->e_phnum; i++)
	{
		switch(phdr[i].p_type)
		{
		case ZLOX_PT_LOAD:
			// 将动态链接库LOAD类型指定的p_offset偏移处的p_filesz大小的数据
			// 加载到p_vaddr + vaddr对应的虚拟内存位置，
			// 该虚拟内存的尺寸为p_memsz字节大小
			zlox_pages_alloc((phdr[i].p_vaddr + vaddr), phdr[i].p_memsz);
			zlox_memcpy((ZLOX_UINT8 *)(phdr[i].p_vaddr + vaddr),
					(ZLOX_UINT8 *)(buf + phdr[i].p_offset),phdr[i].p_filesz);
			// 如果虚拟内存的大小p_memsz大于文件里的数据大小p_filesz时，
			// 则将虚拟内存里多出来的空间给清空为0
			// (当包含bss段时，p_memsz就会大于p_filesz)
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
	// 计算出加载到虚拟内存里的数据的总字节数
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
	// 通过zlox_push_elf_link_map函数向动态数组中添加一个
	// ZLOX_ELF_LINK_MAP结构
	tmp_map = zlox_push_elf_link_map(vaddr, msize);
	tmp_map->maplst = &current_task->link_maps;
	tmp_map->kmap_index = kmap_index;
	tmp_map->soname = soname;
	tmp_map->entry = hdr->e_entry + (tmp_map->index != 0 ? vaddr : 0);
	// 循环遍历program header table，找到Dynamic节的实际虚拟内存地址
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
	// 循环遍历Dynamic节，将节里所需项目的值缓存起来
	while(Dynamic->d_tag != 0)
	{
		switch(Dynamic->d_tag)
		{
		case ZLOX_DT_HASH:
			// tmp_map->index == 0时，表示是ELF可执行文件，
			// ELF可执行文件的虚拟内存地址都是固定的值。
			// tmp_map->index != 0则表示是动态链接库，
			// 动态链接库里的地址信息都是非固定的值，需要
			// 加上vaddr(即动态链接库被加载的起始虚拟地址)
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
			zlox_elf_print_msg("shared library Error:no _dl_runtime_resolve function in \"");
			zlox_elf_print_msg(soname);
			zlox_elf_print_msg("\" \n");
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
						zlox_elf_print_msg("shared library Error:can't find symbol \"");
						zlox_elf_print_msg(name);
						zlox_elf_print_msg("\" \n");
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
					zlox_elf_print_msg("shared library Error:can't find symbol \"");
					zlox_elf_print_msg(name);
					zlox_elf_print_msg("\" \n");
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

// 将soname对应的动态链接库加载到vaddr对应的起始虚拟内存位置处
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
		// 如果是系统里第一次加载该动态链接库的话，
		// 就通过上面定义的zlox_shlib_loadToVAddr函数来完成加载工作
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
	// 循环遍历Dynamic节，将NEEDED类型指定的动态库
	// 给加载到虚拟内存中
	while(Dynamic->d_tag != 0)
	{
		if(Dynamic->d_tag == ZLOX_DT_NEEDED)
		{
			// NEEDED类型的d_un.d_val值是strtab(字符串表)里的                
			// 偏移值，通过Dynamic->d_un.d_val + map->dyn.strtab
			// 就可以得到动态库的名称字符串
			ZLOX_CHAR * tmp_soname = (ZLOX_CHAR *)(Dynamic->d_un.d_val + map->dyn.strtab);
			// tmp_vaddr里存储着动态库需要被加载到的虚拟内存地址
			// 新的动态库需要被放置在之前加载的最后一个动态库的结束
			// 地址的下一页
			ZLOX_UINT32 tmp_vaddr = map->maplst->ptr[map->maplst->count - 1].vaddr;
			tmp_vaddr += map->maplst->ptr[map->maplst->count - 1].msize;
			tmp_vaddr = ZLOX_NEXT_PAGE_START(tmp_vaddr);
			// 通过递归调用zlox_load_shared_library函数来将
			// 依赖的动态链接库给加载到虚拟内存中
			if(zlox_load_shared_library(tmp_soname, tmp_vaddr) == 0)
			{
				zlox_elf_print_msg("shared library Error:load \"");
				zlox_elf_print_msg(tmp_soname);
				zlox_elf_print_msg("\" failed \n");
				return 0;
			}
		}
		Dynamic++;
	}
	return map->entry;
}

// 通过zlox_load_elf函数将ELF可执行文件及其依赖的
// 动态链接库文件给加载到虚拟内存空间里
ZLOX_UINT32 zlox_load_elf(ZLOX_ELF32_EHDR *hdr,ZLOX_UINT8 * buf)
{
	ZLOX_ELF32_PHDR * phdr = (ZLOX_ELF32_PHDR *)((ZLOX_UINT32)hdr + hdr->e_phoff);
	ZLOX_CHAR * interp = ZLOX_NULL;
	ZLOX_UINT32 i,vaddr = 0,last_load,ret;
	// 循环遍历动态链接库的每个program header结构
	for(i = 0; i < hdr->e_phnum; i++)
	{
		switch(phdr[i].p_type)
		{
		case ZLOX_PT_LOAD:
			// 将ELF可执行文件中LOAD类型指定的p_offset偏移处的p_filesz大小的数据
			// 加载到p_vaddr对应的虚拟内存位置，该虚拟内存的
			// 尺寸为p_memsz字节大小
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
		// 得到动态链接库解释器的字符串名称，如：ld.so
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
		// 通过上面定义的zlox_load_shared_library函数将ld.so之类的
		// 动态库解释器加载到ZLOX_ELF_LD_SO_VADDR对应的
		// 虚拟内存位置
		ret = zlox_load_shared_library(interp, ZLOX_ELF_LD_SO_VADDR);
		if(ret == 0)
			return 0;
		ZLOX_ELF32_DYN * Dynamic = map->dyn.Dynamic;
		// 循环遍历Dynamic节，将NEEDED类型指定的动态库
		// 给加载到虚拟内存中
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
						zlox_elf_print_msg("shared library Error:load \"");
						zlox_elf_print_msg(tmp_soname);
						zlox_elf_print_msg("\" failed \n");
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

ZLOX_BOOL g_elf_lock = ZLOX_FALSE;

static ZLOX_VOID zlox_elf_lock(ZLOX_TASK * task)
{
	while(g_elf_lock == ZLOX_TRUE)
	{
		zlox_isr_detect_proc_irq();
		asm("pause");
	}

	g_elf_lock = ZLOX_TRUE;
	if((task != ZLOX_NULL) && 
	   (task->elf_lock == ZLOX_FALSE))
	{
		task->elf_lock = ZLOX_TRUE;
	}
	return;
}

ZLOX_VOID zlox_elf_unlock(ZLOX_TASK * task)
{
	if(g_elf_lock == ZLOX_TRUE)
	{
		g_elf_lock = ZLOX_FALSE;
	}

	if((task != ZLOX_NULL) && 
	   (task->elf_lock == ZLOX_TRUE))
	{
		task->elf_lock = ZLOX_FALSE;
	}
	return;
}

ZLOX_SINT32 zlox_execve(const ZLOX_CHAR * filename)
{
	zlox_elf_lock(ZLOX_NULL);

	ZLOX_SINT32 ret_pos = zlox_set_filename((ZLOX_CHAR **)&filename);
	//ZLOX_FS_NODE *fsnode = zlox_finddir_fs(fs_root, (ZLOX_CHAR *)filename);
	ZLOX_FS_NODE fsnode;
	zlox_memset(((ZLOX_UINT8 *)&fsnode), 0, sizeof(ZLOX_FS_NODE));
	zlox_finddir_fs_safe(fs_root, (ZLOX_CHAR *)filename, &fsnode);

	if ((fsnode.flags & 0x7) == ZLOX_FS_FILE)
	{
		ZLOX_SINT32 ret = zlox_fork();
		if(ret == 0)
		{
			current_task->elf_lock = ZLOX_TRUE;

			ZLOX_CHAR * buf = (ZLOX_CHAR *)zlox_kmalloc(fsnode.length + 10);
			ZLOX_UINT32 sz = zlox_read_fs(&fsnode, 0, fsnode.length, (ZLOX_UINT8 *)buf);
			ZLOX_UINT32 addr;

			if((sz > 0) && zlox_elf_check_supported((ZLOX_ELF32_EHDR *)buf, ZLOX_ET_EXEC) )
			{
				// zlox_execve会通过zlox_load_elf函数将ELF格式
				// 的可执行文件及其依赖的动态链接库给加载到内存中
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
					zlox_elf_unlock(current_task);
					return addr;
				}
				else
				{
					zlox_kfree(buf);
					zlox_elf_print_msg("execve:\"");
					zlox_elf_print_msg(filename);
					zlox_elf_print_msg("\" load failed !\n");
					zlox_elf_unlock(current_task);
					zlox_exit(-1);
					return -1;
				}				
			}
			else
			{
				zlox_kfree(buf);
				zlox_elf_print_msg("execve Error:\"");
				zlox_elf_print_msg(filename);
				zlox_elf_print_msg("\" is not a valid ELF file!\n");
				zlox_elf_unlock(current_task);
				zlox_exit(-1);
				return -1;
			}
		}
		zlox_restore_filename((ZLOX_CHAR *)filename,ret_pos);
		return 0;
	}
	else if((fsnode.flags & 0x7) == ZLOX_FS_DIRECTORY)
	{
		zlox_elf_print_msg("execve Error:\"");
		zlox_elf_print_msg(filename);
		zlox_elf_print_msg("\" is a directory , it's must be an ELF file \n");
		zlox_restore_filename((ZLOX_CHAR *)filename,ret_pos);
		zlox_elf_unlock(ZLOX_NULL);
		return -1;
	}
	else
	{
		zlox_elf_print_msg("execve Error:\"");
		zlox_elf_print_msg(filename);
		zlox_elf_print_msg("\" is not exist in your file system! \n");
		zlox_restore_filename((ZLOX_CHAR *)filename,ret_pos);
		zlox_elf_unlock(ZLOX_NULL);
		return -1;
	}

	zlox_elf_unlock(ZLOX_NULL);
	return -1;
}

