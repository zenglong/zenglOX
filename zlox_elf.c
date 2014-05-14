/*zlox_elf.c 和ELF可执行文件相关的函数定义*/

#include "zlox_elf.h"
#include "zlox_monitor.h"
#include "zlox_fs.h"
#include "zlox_paging.h"
#include "zlox_task.h"
#include "zlox_kheap.h"

// 所需的zlox_paging.c里的全局变量
extern ZLOX_PAGE_DIRECTORY * current_directory;
extern volatile ZLOX_TASK * current_task;

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

ZLOX_BOOL zlox_elf_check_supported(ZLOX_ELF32_EHDR *hdr) {
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
	if(hdr->e_type != ZLOX_ET_EXEC) {
		zlox_monitor_write("ELF Error:Unsupported ELF File type.\n");
		return ZLOX_FALSE;
	}
	return ZLOX_TRUE;
}

ZLOX_UINT32 zlox_load_elf(ZLOX_ELF32_EHDR *hdr,ZLOX_UINT8 * buf)
{
	ZLOX_ELF32_SHDR * shdr = (ZLOX_ELF32_SHDR *)((ZLOX_UINT32)hdr + hdr->e_shoff);
	ZLOX_ELF32_SHDR * shstr = &shdr[hdr->e_shstrndx];
	ZLOX_CHAR * strtab = (ZLOX_CHAR *)hdr + shstr->sh_offset;

	if(hdr->e_entry == ZLOX_NULL)
		return ZLOX_NULL;

	if(strtab == ZLOX_NULL)
		return ZLOX_NULL;

	ZLOX_UINT32 i,j;
	ZLOX_CHAR * nameOffset;
	ZLOX_ELF32_SHDR * codeSheader = ZLOX_NULL;
	ZLOX_ELF32_SHDR * tmpSheader = ZLOX_NULL;
	ZLOX_CHAR * codeName = ".text";
	for(i = 0; i < hdr->e_shnum; i++)
	{
		nameOffset = strtab + shdr[i].sh_name;
		if(shdr[i].sh_type == ZLOX_SHT_PROGBITS)
		{
			if(zlox_strcmp(codeName,nameOffset)==0)
			{
				codeSheader = &shdr[i];
				break;
			}
		}
	}

	if(codeSheader == ZLOX_NULL)
		return ZLOX_NULL;

	ZLOX_PAGE * tmp_page;
	ZLOX_UINT32 table_idx;
	for(i = 0; i < hdr->e_shnum; i++)
	{
		if(shdr[i].sh_type == ZLOX_SHT_PROGBITS)
		{
			tmpSheader = &shdr[i];
			if(tmpSheader->sh_addr == ZLOX_NULL)
				continue;
			for( j = tmpSheader->sh_addr;
				j < (tmpSheader->sh_addr + tmpSheader->sh_size);
				(j = ((j + 0x1000) & 0xFFFFF000)))
			{
				tmp_page = zlox_get_page(j, 1, current_directory);
				table_idx = (j/0x1000)/ 1024;
				if(tmp_page->frame == 0)
					zlox_alloc_frame( tmp_page, 0 , 1 );
				else if((current_directory->tablesPhysical[table_idx] & 0x2) == 0)
					zlox_page_copy(j);
			}
		}
	}

	// Flush the TLB(translation lookaside buffer) by reading and writing the page directory address again.
	ZLOX_UINT32 pd_addr;
	asm volatile("mov %%cr3, %0" : "=r" (pd_addr));
	asm volatile("mov %0, %%cr3" : : "r" (pd_addr));

	for(i = 0; i < hdr->e_shnum; i++)
	{
		if(shdr[i].sh_type == ZLOX_SHT_PROGBITS)
		{
			tmpSheader = &shdr[i];
			if(tmpSheader->sh_addr == ZLOX_NULL)
				continue;

			zlox_memcpy((ZLOX_UINT8 *)tmpSheader->sh_addr,
					(ZLOX_UINT8 *)(buf + tmpSheader->sh_offset),tmpSheader->sh_size);
		}
	}

	return (ZLOX_UINT32)hdr->e_entry;
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

			if((sz > 0) && zlox_elf_check_supported((ZLOX_ELF32_EHDR *)buf))
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

