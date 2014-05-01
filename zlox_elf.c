/*zlox_elf.c 和ELF可执行文件相关的函数定义*/

#include "zlox_elf.h"
#include "zlox_monitor.h"
#include "zlox_fs.h"
#include "zlox_paging.h"
#include "zlox_task.h"
#include "zlox_kheap.h"

// 所需的zlox_paging.c里的全局变量
extern ZLOX_PAGE_DIRECTORY * current_directory;

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

	for(i = 0; i < hdr->e_shnum; i++)
	{
		if(shdr[i].sh_type == ZLOX_SHT_PROGBITS)
		{
			tmpSheader = &shdr[i];
			if(tmpSheader->sh_addr == ZLOX_NULL)
				continue;
			for( j = tmpSheader->sh_addr;
				j < (tmpSheader->sh_addr + tmpSheader->sh_size);
				j += 0x1000)
			{
				// General-purpose stack is in user-mode.
				zlox_alloc_frame_do( zlox_get_page(j, 1, current_directory), 0 , 1 );
			}

			zlox_memcpy((ZLOX_UINT8 *)tmpSheader->sh_addr,
					(ZLOX_UINT8 *)(buf + tmpSheader->sh_offset),tmpSheader->sh_size);
		}
	}

	return (ZLOX_UINT32)hdr->e_entry;
}

ZLOX_SINT32 zlox_execve(const ZLOX_CHAR * filename)
{
	ZLOX_FS_NODE *fsnode = zlox_finddir_fs(fs_root, (ZLOX_CHAR *)filename);

	if ((fsnode->flags & 0x7) == ZLOX_FS_FILE)
	{
		ZLOX_SINT32 ret = zlox_fork();
		if(ret == 0)
		{
			ZLOX_CHAR * buf = (ZLOX_CHAR *)zlox_kmalloc(fsnode->length + 10);
			ZLOX_UINT32 sz = zlox_read_fs(fsnode, 0, 1280, (ZLOX_UINT8 *)buf);
			ZLOX_UINT32 addr;

			if((sz > 0) && zlox_elf_check_supported((ZLOX_ELF32_EHDR *)buf))
			{
				if((addr = zlox_load_elf((ZLOX_ELF32_EHDR *)buf,(ZLOX_UINT8 *)buf)) > 0)
				{
					zlox_kfree(buf);
					zlox_monitor_write("execve:\"");
					zlox_monitor_write(filename);
					zlox_monitor_write("\" is a valid ELF file and load success !\n");
					return addr;
				}
				else
				{
					zlox_kfree(buf);
					zlox_monitor_write("execve:\"");
					zlox_monitor_write(filename);
					zlox_monitor_write("\" load failed !\n");
					return -1;
				}				
			}
			else
			{
				zlox_kfree(buf);
				zlox_monitor_write("execve Error:\"");
				zlox_monitor_write(filename);
				zlox_monitor_write("\" is not a valid ELF file!\n");
				return -1;
			}
		}
		return 0;
	}
	else
	{
		zlox_monitor_write("execve Error:\"");
		zlox_monitor_write(filename);
		zlox_monitor_write("\" is a directory, It's must be a file\n");
		return -1;
	}	

	return -1;
}

