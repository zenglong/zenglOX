/* zlox_iso.c -- 包含和iso 9660文件系统相关的函数定义 */

// 有关ISO 9660文件系统的结构，可以参考 http://wiki.osdev.org/ISO_9660 和 
// http://en.wikipedia.org/wiki/ISO_9660 这两个链接里的文章

#include "zlox_kheap.h"
#include "zlox_iso.h"
#include "zlox_ata.h"
#include "zlox_monitor.h"

extern ZLOX_IDE_DEVICE ide_devices[4];

ZLOX_SINT32 iso_ide_index;
ZLOX_FS_NODE * iso_root;
ZLOX_DIRENT iso_dirent;
ZLOX_FS_NODE iso_fsnode;
ZLOX_UINT8 * iso_path_table;
ZLOX_ISO_PATH_TABLE_EXTDATA * iso_path_table_extdata; 
ZLOX_UINT32 iso_path_table_sz;
ZLOX_ISO_CACHE iso_cache = {0};
ZLOX_CHAR iso_tmp_name[128];

// 根据node文件节点来读取某个文件的内容到用户提供的buffer缓冲区域
static ZLOX_UINT32 zlox_iso_read(ZLOX_FS_NODE * node, ZLOX_UINT32 arg_offset, ZLOX_UINT32 size, ZLOX_UINT8 * buffer)
{
	if((node->flags & 0x7) != ZLOX_FS_FILE)
		return 0;
	ZLOX_UINT32 tmp_lba = node->inode;
	ZLOX_UINT32 cur_datasize = node->length;
	if(tmp_lba != iso_cache.lba)
	{
		if((cur_datasize > iso_cache.size) || (iso_cache.size > cur_datasize * 3))
		{
			iso_cache.size = cur_datasize / ZLOX_ATAPI_SECTOR_SIZE;
			iso_cache.size *= ZLOX_ATAPI_SECTOR_SIZE;
			if((cur_datasize % ZLOX_ATAPI_SECTOR_SIZE) > 0)
				iso_cache.size += ZLOX_ATAPI_SECTOR_SIZE;
			if(iso_cache.ptr != ZLOX_NULL)
				zlox_kfree(iso_cache.ptr);
			iso_cache.ptr = (ZLOX_UINT8 *)zlox_kmalloc(iso_cache.size);
		}
		zlox_atapi_drive_read_sectors(iso_ide_index, tmp_lba, (iso_cache.size/ZLOX_ATAPI_SECTOR_SIZE),
						iso_cache.ptr);
		iso_cache.lba = tmp_lba;
	}
	if (arg_offset >= node->length)
		return 0;
	if(size == 0)
		return 0;
	if(buffer == 0)
		return 0;
	if (arg_offset + size > node->length)
		size = node->length - arg_offset;
	zlox_memcpy(buffer, (ZLOX_UINT8 *)(iso_cache.ptr + arg_offset), size);
	return size;
}

// 从node目录节点中搜索index索引对应的文件或目录的基本信息
static ZLOX_DIRENT * zlox_iso_readdir(ZLOX_FS_NODE *node, ZLOX_UINT32 index)
{	
	ZLOX_UINT32 i;
	if((node->flags & 0x7) != ZLOX_FS_DIRECTORY)
		return 0;

	ZLOX_UINT32 tmp_lba = node->inode;
	ZLOX_UINT32 cur_datasize = node->length;
	if(tmp_lba != iso_cache.lba)
	{
		if((cur_datasize > iso_cache.size) || (iso_cache.size > cur_datasize * 3))
		{
			iso_cache.size = cur_datasize / ZLOX_ATAPI_SECTOR_SIZE;
			iso_cache.size *= ZLOX_ATAPI_SECTOR_SIZE;
			if((cur_datasize % ZLOX_ATAPI_SECTOR_SIZE) > 0)
				iso_cache.size += ZLOX_ATAPI_SECTOR_SIZE;
			if(iso_cache.ptr != ZLOX_NULL)
				zlox_kfree(iso_cache.ptr);
			iso_cache.ptr = (ZLOX_UINT8 *)zlox_kmalloc(iso_cache.size);
		}
		zlox_atapi_drive_read_sectors(iso_ide_index, tmp_lba, (iso_cache.size/ZLOX_ATAPI_SECTOR_SIZE),
						iso_cache.ptr);
		iso_cache.lba = tmp_lba;
	}
	ZLOX_ISO_DIR_RECORD * dir_record = (ZLOX_ISO_DIR_RECORD *)iso_cache.ptr;
	ZLOX_UINT32 dir_offset,dir_sector;
	for(i=0;i <= index;i++)
	{
		dir_offset = (ZLOX_UINT32)dir_record - (ZLOX_UINT32)iso_cache.ptr;
		dir_sector = dir_offset / ZLOX_ATAPI_SECTOR_SIZE + 1;
		if(dir_offset >= cur_datasize)
			return 0;
		if(dir_record->length == 0)
		{
			if(dir_sector == cur_datasize / ZLOX_ATAPI_SECTOR_SIZE)
				return 0;
			// 当目录的extents(数据块)的某个扇区快用完时，它会从下一个扇区开始放置数据，所以这里需要直接跳到下一个扇区
			dir_record = (ZLOX_ISO_DIR_RECORD *)(iso_cache.ptr + 
						(dir_sector * ZLOX_ATAPI_SECTOR_SIZE));
		}
		if(i < index)
			dir_record = (ZLOX_ISO_DIR_RECORD *)((ZLOX_UINT8 *)dir_record + dir_record->length);
	}
	zlox_memcpy((ZLOX_UINT8 *)iso_dirent.name, 
		((ZLOX_UINT8 *)dir_record + sizeof(ZLOX_ISO_DIR_RECORD)),
		dir_record->file_name_length);
	iso_dirent.name[dir_record->file_name_length] = 0;
	if(iso_dirent.name[0] == 0 || iso_dirent.name[0] == 1)
		iso_dirent.name[0] = 0;

	iso_dirent.ino = dir_record->l_lba;
	return &iso_dirent;
}

// 从node目录节点中搜索name字符串对应的文件或子目录的节点
static ZLOX_FS_NODE * zlox_iso_finddir(ZLOX_FS_NODE *node, ZLOX_CHAR *name)
{
	ZLOX_PATH_TABLE_ENTRY * path_table = (ZLOX_PATH_TABLE_ENTRY *)iso_path_table;
	ZLOX_UINT16 var_length,parent_index,cur_index;
	ZLOX_SINT32 name_len = zlox_strlen(name),j;
	if(name_len <= 0)
		return 0;

	if((node->flags & 0x7) != ZLOX_FS_DIRECTORY)
		return 0;

	path_table = (ZLOX_PATH_TABLE_ENTRY *)(iso_path_table + 
					iso_path_table_extdata[node->impl - 1].pt_offset);
	cur_index = parent_index = node->impl;
	zlox_strcpy(iso_tmp_name,name);

	for(j=0;j < name_len;j++)
	{
		if(iso_tmp_name[j] == '/')
			iso_tmp_name[j] = '\0';
	}
	
	ZLOX_SINT32 tmp_name_offset = 0;
	ZLOX_CHAR * tmp_name = iso_tmp_name + tmp_name_offset;
	ZLOX_CHAR * tmp_path_name;
	ZLOX_BOOL is_dir = ZLOX_TRUE;
	ZLOX_UINT32 tmp_lba = path_table->lba;
	// 双层循环是为了找到最终目录的lba信息，例如: iso/BOOT/GRUB/GRUB.CFG;1 那么通过循环就可以找到
	// GRUB.CFG;1文件的父目录GRUB的LBA逻辑块信息，然后由LBA找到GRUB目录的extents数据块，从数据块里
	// 就可以搜索到GRUB.CFG;1文件的directory record信息了(directory record里也包含了文件的LBA和大小信息)
	// 如果name参数是 iso/BOOT/GRUB的话，就会先通过循环找到GRUB的父目录BOOT的信息，再由BOOT的数据块搜索GRUB目录的LBA等信息
	while(ZLOX_TRUE)
	{
		while(ZLOX_TRUE)
		{
			var_length = (path_table->dir_id_length%2 == 1) ? (path_table->dir_id_length + 1) : 
				path_table->dir_id_length;
			path_table = (ZLOX_PATH_TABLE_ENTRY *)((ZLOX_UINT8 *)path_table + 
						(sizeof(ZLOX_PATH_TABLE_ENTRY) + var_length));
			if(((ZLOX_UINT32)path_table - (ZLOX_UINT32)iso_path_table) >= iso_path_table_sz)
			{
				if((tmp_name_offset + zlox_strlen(tmp_name) + 1) < (name_len + 1))
					return 0;
				is_dir = ZLOX_FALSE;
				break;
			}
			cur_index++;
			tmp_path_name = (ZLOX_CHAR *)path_table + sizeof(ZLOX_PATH_TABLE_ENTRY);
			if(zlox_strlen(tmp_name) == path_table->dir_id_length && 
			   zlox_strcmpn(tmp_path_name,tmp_name,path_table->dir_id_length) == 0 && 
			   path_table->parent_index == parent_index)
			{
				break;
			}
		}
		if(is_dir == ZLOX_FALSE)
			break;
		tmp_name_offset += zlox_strlen(tmp_name) + 1;
		if(tmp_name_offset < name_len + 1)
		{
			tmp_name = iso_tmp_name + tmp_name_offset;
			tmp_lba = path_table->lba;
			parent_index = cur_index;
		}
		else
			break;
	}

	ZLOX_UINT32 cur_datasize;
	cur_datasize = iso_path_table_extdata[parent_index - 1].datasize;
	if(tmp_lba != iso_cache.lba)
	{
		if((cur_datasize > iso_cache.size) || (iso_cache.size > cur_datasize * 3))
		{
			iso_cache.size = cur_datasize / ZLOX_ATAPI_SECTOR_SIZE;
			iso_cache.size *= ZLOX_ATAPI_SECTOR_SIZE;
			if((cur_datasize % ZLOX_ATAPI_SECTOR_SIZE) > 0)
				iso_cache.size += ZLOX_ATAPI_SECTOR_SIZE;
			if(iso_cache.ptr != ZLOX_NULL)
				zlox_kfree(iso_cache.ptr);
			iso_cache.ptr = (ZLOX_UINT8 *)zlox_kmalloc(iso_cache.size);
		}
		zlox_atapi_drive_read_sectors(iso_ide_index, tmp_lba, (iso_cache.size/ZLOX_ATAPI_SECTOR_SIZE),
						iso_cache.ptr);
		iso_cache.lba = tmp_lba;
	}
	ZLOX_ISO_DIR_RECORD * dir_record = (ZLOX_ISO_DIR_RECORD *)iso_cache.ptr;
	ZLOX_UINT32 dir_offset,dir_sector;
	while(ZLOX_TRUE)
	{
		dir_offset = (ZLOX_UINT32)dir_record - (ZLOX_UINT32)iso_cache.ptr;
		dir_sector = dir_offset / ZLOX_ATAPI_SECTOR_SIZE + 1;
		if(dir_record->length == 0)
		{
			if(dir_sector == cur_datasize / ZLOX_ATAPI_SECTOR_SIZE)
				return 0;
			dir_record = (ZLOX_ISO_DIR_RECORD *)(iso_cache.ptr + 
						(dir_sector * ZLOX_ATAPI_SECTOR_SIZE));
		}
		tmp_path_name = (ZLOX_CHAR *)dir_record + sizeof(ZLOX_ISO_DIR_RECORD);
		if(zlox_strlen(tmp_name) == dir_record->file_name_length &&
		   zlox_strcmpn(tmp_path_name,tmp_name,dir_record->file_name_length) == 0)
			break;
		dir_record = (ZLOX_ISO_DIR_RECORD *)((ZLOX_UINT8 *)dir_record + dir_record->length);
	}

	// 搜索到文件或目录的directory record后，由directory record里的信息来组建iso_fsnode节点,以便进行返回
	zlox_memset((ZLOX_UINT8 *)&iso_fsnode,0,sizeof(ZLOX_FS_NODE));
	zlox_strcpy(iso_fsnode.name, tmp_name);
	if(is_dir == ZLOX_TRUE)
		iso_fsnode.flags = ZLOX_FS_DIRECTORY;
	else
		iso_fsnode.flags = ZLOX_FS_FILE;
	if(is_dir == ZLOX_TRUE)
	{
		iso_fsnode.readdir = &zlox_iso_readdir;
		iso_fsnode.finddir = &zlox_iso_finddir;
	}
	else
		iso_fsnode.read = &zlox_iso_read;
	if(is_dir == ZLOX_TRUE)
		iso_fsnode.impl = cur_index;
	iso_fsnode.length = dir_record->l_datasize;
	iso_fsnode.inode = dir_record->l_lba;
	return &iso_fsnode;
}

// 初始化Path Table(路径表)的额外数据
// 因为iso 9660文件系统的原始的Path Table里只有LBA逻辑块寻址信息，并没有包含目录的尺寸大小等信息
// 没有尺寸大小信息的话，会对后面的寻址带来不方便，因为当某个目录的extents(数据块部分)超过一个扇区的大小时，我们就
// 很难定位超过一个扇区的其他的文件了(没有尺寸大小就不知道该加载多少扇区)，
// 所以这里通过path_table_extdata来对原始的path table做个补充，将目录的尺寸大小信息以及
// 这些目录在Path Table里的offset偏移量记录下来
ZLOX_VOID zlox_init_path_table_extdata()
{
	ZLOX_PATH_TABLE_ENTRY * path_table = (ZLOX_PATH_TABLE_ENTRY *)iso_path_table;
	ZLOX_PATH_TABLE_ENTRY * parent_table;
	ZLOX_UINT16 var_length,count = 1,i,parent_index;

	// 首先确定path table里一共记录了多少个目录，后面就可以为iso_path_table_extdata分配足够的堆空间
	while(ZLOX_TRUE)
	{
		var_length = (path_table->dir_id_length%2 == 1) ? (path_table->dir_id_length + 1) : 
				path_table->dir_id_length;
		path_table = (ZLOX_PATH_TABLE_ENTRY *)((ZLOX_UINT8 *)path_table + 
					(sizeof(ZLOX_PATH_TABLE_ENTRY) + var_length));
		if(((ZLOX_UINT32)path_table - (ZLOX_UINT32)iso_path_table) >= iso_path_table_sz)
			break;
		else
			count++;
	}
	iso_path_table_extdata = (ZLOX_ISO_PATH_TABLE_EXTDATA *)zlox_kmalloc(sizeof(ZLOX_ISO_PATH_TABLE_EXTDATA) *
								 count);
	zlox_memset((ZLOX_UINT8 *)iso_path_table_extdata,0,sizeof(ZLOX_ISO_PATH_TABLE_EXTDATA) *
								 count);
	// 先设置好Root根目录的偏移量及尺寸大小信息
	iso_path_table_extdata[0].pt_offset = 0;
	iso_path_table_extdata[0].datasize = iso_root->length;
	path_table = (ZLOX_PATH_TABLE_ENTRY *)iso_path_table;
	// 循环设置每个目录的偏移量及大小信息
	for(i=1;i <= count;i++)
	{
		if(iso_path_table_extdata[i-1].datasize == 0)
		{
			// 从父目录的extents(数据块)里搜索当前目录的directory record(该记录中存储了目录的大小等信息)
			parent_index = path_table->parent_index;
			parent_table = (ZLOX_PATH_TABLE_ENTRY *)(iso_path_table + 
						iso_path_table_extdata[parent_index - 1].pt_offset);
			zlox_memset((ZLOX_UINT8 *)&iso_fsnode,0,sizeof(ZLOX_FS_NODE));
			iso_fsnode.flags = ZLOX_FS_DIRECTORY;
			iso_fsnode.impl =parent_index;
			iso_fsnode.inode = parent_table->lba;
			zlox_memcpy((ZLOX_UINT8 *)iso_tmp_name,(ZLOX_UINT8 *)path_table + sizeof(ZLOX_PATH_TABLE_ENTRY),
				path_table->dir_id_length);
			iso_tmp_name[path_table->dir_id_length] = '\0';
			ZLOX_FS_NODE * node = zlox_iso_finddir(&iso_fsnode, iso_tmp_name);
			iso_path_table_extdata[i-1].datasize = node->length;
			iso_path_table_extdata[i-1].pt_offset = (ZLOX_UINT32)path_table - (ZLOX_UINT32)iso_path_table;
		}
		var_length = (path_table->dir_id_length%2 == 1) ? (path_table->dir_id_length + 1) : 
				path_table->dir_id_length;
		path_table = (ZLOX_PATH_TABLE_ENTRY *)((ZLOX_UINT8 *)path_table + 
					(sizeof(ZLOX_PATH_TABLE_ENTRY) + var_length));
	}
	return ;
}

// 卸载iso目录，将堆中分配的相关内存资源释放掉
ZLOX_FS_NODE * zlox_unmount_iso()
{
	if(iso_root != ZLOX_NULL)
	{
		zlox_kfree(iso_root);
		iso_root = ZLOX_NULL;
		iso_ide_index = -1;
	}
	if(iso_path_table != ZLOX_NULL)
	{
		zlox_kfree(iso_path_table);
		iso_path_table = ZLOX_NULL;
		iso_path_table_sz = 0;
	}
	if(iso_path_table_extdata != ZLOX_NULL)
	{
		zlox_kfree(iso_path_table_extdata);
		iso_path_table_extdata = ZLOX_NULL;
	}
	if(iso_cache.ptr != ZLOX_NULL)
	{
		zlox_kfree(iso_cache.ptr);
		iso_cache.ptr = ZLOX_NULL;
		iso_cache.lba = 0;
		iso_cache.size = 0;
	}
	return iso_root;
}

// 挂载CDROM到iso目录下
ZLOX_FS_NODE * zlox_mount_iso()
{
	ZLOX_SINT32 i,j;
	iso_root = ZLOX_NULL;
	for(i = 0 ; i < 4 ; i++)
	{
		// 寻找ATAPI光盘设备
		if(ide_devices[i].Reserved == 0 || ide_devices[i].Type != ZLOX_IDE_ATAPI)
			continue;

		// 初始化ISO文件系统的根节点
		iso_root = (ZLOX_FS_NODE *)zlox_kmalloc(sizeof(ZLOX_FS_NODE));
		zlox_memset((ZLOX_UINT8 *)iso_root, 0, sizeof(ZLOX_FS_NODE));
		zlox_strcpy(iso_root->name, "iso");
		iso_root->mask = iso_root->uid = iso_root->gid = iso_root->inode = iso_root->length = 0;
		iso_root->flags = ZLOX_FS_DIRECTORY;
		iso_root->read = 0;
		iso_root->write = 0;
		iso_root->open = 0;
		iso_root->close = 0;
		iso_root->readdir = &zlox_iso_readdir;
		iso_root->finddir = &zlox_iso_finddir;
		iso_root->ptr = 0;
		iso_root->impl = 1;

		//设置iso目录对应的IDE设备号，访问iso时，会使用该设备号来读写对应的ATAPI光盘设备
		iso_ide_index = i;
		//Primary Volume Descriptor (主卷描述符，里面存储着Path Table路径表和Root根目录的LBA寻址信息)
		ZLOX_UINT8 * pvd = (ZLOX_UINT8 *)zlox_kmalloc(ZLOX_ATAPI_SECTOR_SIZE);
		//查找Primary Volume Descriptor (主卷描述符)
		for(j=0;;j++)
		{
			zlox_atapi_drive_read_sector(iso_ide_index,ZLOX_PVD_START_LBA + j,pvd);
			if(pvd[1] != ZLOX_VD_ID_1 || pvd[2] != ZLOX_VD_ID_2 ||
			   pvd[3] != ZLOX_VD_ID_3 || pvd[4] != ZLOX_VD_ID_4 || pvd[5] != ZLOX_VD_ID_5)
				goto err;
			else if(pvd[0] == ZLOX_PVD_TYPE)
				break;
			else if(pvd[0] == ZLOX_VD_TERMINATOR)
				goto err;
		}

		//获取根目录的directory record(目录记录，里面存储了目录的寻址等信息)
		ZLOX_ISO_DIR_RECORD * dir_record = (ZLOX_ISO_DIR_RECORD *)(pvd + ZLOX_PVD_ROOT_OFFSET);
		iso_root->inode = dir_record->l_lba;
		iso_root->length = dir_record->l_datasize;

		//将Path Table路径表缓存到iso_path_table对应的堆空间
		ZLOX_UINT32 path_table_sz = *((ZLOX_UINT32 *)(pvd + ZLOX_PVD_PTSZ_OFFSET));
		ZLOX_UINT32 path_table_lba = *((ZLOX_UINT32 *)(pvd + ZLOX_PVD_LPT_OFFSET));
		zlox_atapi_drive_read_sector(iso_ide_index,path_table_lba,pvd);
		iso_path_table = (ZLOX_UINT8 *)zlox_kmalloc(path_table_sz);
		zlox_memcpy(iso_path_table,pvd,path_table_sz);
		iso_path_table_sz = path_table_sz;
		zlox_kfree(pvd);
		zlox_init_path_table_extdata();
		break;

err:
		zlox_monitor_write("kernel: iso mount err...\n");
		zlox_kfree(iso_root);
		zlox_kfree(pvd);
		iso_root = ZLOX_NULL;
		break;
	}

	return iso_root;
}

