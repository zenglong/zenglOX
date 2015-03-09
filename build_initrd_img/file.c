// file.c -- 对硬盘进行读写文件等操作

#include "common.h"
#include "syscall.h"
#include "fs.h"

char file_tmp_name[128];

FS_NODE * ret_fsnode = NULL;

/*SINT32 memcmp(UINT8 * s1, UINT8 * s2, UINT32 n)
{
	UINT8 u1, u2;
	for ( ; n-- ; s1++, s2++) {
		u1 = * s1;
		u2 = * s2;
		if (u1 != u2) {
			return (u1-u2);
		}
	}
	return 0;
}*/

FS_NODE * create_empty_file(FS_NODE * fs_root, char * name)
{
	SINT32 name_len = strlen(name),j;
	BOOL flag = FALSE;
	if(name_len <= 0)
		return 0;

	strcpy(file_tmp_name,name);
	for(j=0;j < name_len;j++)
	{
		if(file_tmp_name[j] == '/')
		{
			file_tmp_name[j] = '\0';
			if(!flag)
				flag = TRUE;
		}
	}
	if(!flag)
	{
		int ret = syscall_writedir_fs_safe(fs_root, name, FS_FILE, ret_fsnode);
		if(ret == -1)
		{
			syscall_cmd_window_write(name);
			syscall_cmd_window_write(" is not writable , maybe it's not in hard disk or maybe it's full");
		}
		return ret_fsnode;
	}

	SINT32 tmp_name_offset = 0, tmp_name_len;
	CHAR * tmp_name = file_tmp_name + tmp_name_offset;
	FS_NODE * tmp_node = fs_root;
	int ret;
	BOOL isLast = FALSE;
	while(TRUE)
	{
		ret = syscall_finddir_fs_safe(tmp_node, tmp_name, ret_fsnode);
		tmp_name_len = strlen(tmp_name);
		if((tmp_name_offset + tmp_name_len + 1) >= name_len + 1)
			isLast = TRUE;
		if(ret == -1)
		{
			if(isLast)
			{
				ret = syscall_writedir_fs_safe(tmp_node, tmp_name, FS_FILE, ret_fsnode);
				if(ret == -1)
				{
					syscall_cmd_window_write(name);
					syscall_cmd_window_write(" is not writable , maybe it's not in hard disk or maybe it's full");
				}
				return ret_fsnode;
			}
			else
			{
				ret = syscall_writedir_fs_safe(tmp_node, tmp_name, FS_DIRECTORY, ret_fsnode);
				if(ret == -1)
				{
					syscall_cmd_window_write(tmp_name);
					syscall_cmd_window_write(" is not writable , maybe it's not in hard disk or maybe it's full");
					return NULL;
				}
			}
		}
		else
		{
			if(isLast)
			{
				if((ret_fsnode->flags & 0x7) == FS_DIRECTORY)
				{
					syscall_cmd_window_write(name);
					syscall_cmd_window_write(" is a directory");
				}
				else
				{
					syscall_cmd_window_write(name);
					syscall_cmd_window_write(" is exist");
				}
				return NULL;
			}
			else if((ret_fsnode->flags & 0x7) == FS_FILE)
			{
				syscall_cmd_window_write(tmp_name);
				syscall_cmd_window_write(" is a file not a directory");
				return NULL;
			}
		}
		tmp_node = ret_fsnode;
		tmp_name_offset += tmp_name_len + 1;
		if(tmp_name_offset < name_len + 1)
		{
			tmp_name = file_tmp_name + tmp_name_offset;
		}
		else
			break;
	}
	return ret_fsnode;
}

/*
当使用mount工具将硬盘分区挂载到hd目录后，就可以使用
file工具对hd目录进行读写文件或创建文件，创建子目录，删除文件，
删除子目录，重命名文件，重命名子目录的操作，
例如：file hd/hello命令表示在hd目录内创建一个空的hello文件
然后使用ls hd命令就可以在hd目录中查看到刚才创建的hello文件了，
通过ls hd/hello命令可以看到该hello文件的length尺寸为0，
通过file ata hd/ata2命令可以将ramdisk里的ata文件拷贝到hd目录内，同时，
在hd目录里的文件名为ata2，接着就可以通过hd/ata2命令来执行ata工具的操作了，如下所示:
zenglOX> file ata hd/ata2
copy content of ata to hd/ata2 success
zenglOX> hd/ata2 -s
ata0-master(....) ......
.........................
zenglOX> 

通过file -rm hd/ata2命令就可以将刚才创建的ata2文件给删除掉。
file工具里为了简化起见，目录是在创建文件时，一起被创建的，
在你自定义的程式里，也可以根据file里的原理来单独创建某目录。
例如 file ata hd/dir/ata命令表示将ata文件拷贝到hd目录里的dir目录内，
如果dir目录不存在，则会创建该目录。

使用-rm参数删除目录时，必须先将目录里的所有内容删除，再删除该目录，
当然在你自定义的程式里，也可以通过递归方法，让工具自动将目录里的文件删除，再将目录删除，
file工具目前为了简化起见，并没有这么做。

通过file -rename hd/hello hello2命令可以将hd目录的hello文件重命名为hello2，如果
hello是一个目录的话，就将该目录重命名为hello2，如果重命名的名称里包含斜杠'/'，则'/'会被zenglfs替换为下划线'_'

以上就是该工具的用法。
*/

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(task);

	FS_NODE * fs_root = (FS_NODE *)syscall_get_fs_root();
	ret_fsnode = (FS_NODE *)syscall_umalloc(sizeof(FS_NODE));
	int ret_val = 0;
	if(argc == 2 && argv[1][0] != '-')
	{
		char * name = argv[1];
		if(create_empty_file(fs_root, name) != NULL)
			syscall_cmd_window_write("create an empty file success, you can use \"ls\" to see it");
		ret_val = 0;
		goto end;
	}
	else if(argc == 3)
	{
		if(!strcmp(argv[1],"-rm"))
		{
			char * name = argv[2];
			int ret;
			ret = syscall_finddir_fs_safe(fs_root, name, ret_fsnode);
			if(ret == -1)
			{
				syscall_cmd_window_write(name);
				syscall_cmd_window_write(" is not exist");
				ret_val = -1;
				goto end;
			}
			if(syscall_remove_fs(ret_fsnode) == 0)
			{
				syscall_cmd_window_write("remove failed");
				ret_val = -1;
				goto end;
			}
			else
			{
				syscall_cmd_window_write("remove ");
				syscall_cmd_window_write(name);
				ret_val = 0;
				goto end;
			}
		}
		else if(argv[1][0] != '-')
		{
			char * src_name = argv[1];
			char * dest_name = argv[2];
			int ret;
			FS_NODE * src_fsnode = (FS_NODE *)syscall_umalloc(sizeof(FS_NODE));
			ret = syscall_finddir_fs_safe(fs_root, src_name, src_fsnode);
			if(ret == -1)
			{
				syscall_cmd_window_write(src_name);
				syscall_cmd_window_write(" is not exist");
				if(src_fsnode != NULL)
					syscall_ufree(src_fsnode);
				ret_val = -1;
				goto end;
			}
			else if ((src_fsnode->flags & 0x7) != FS_FILE)
			{
				syscall_cmd_window_write(src_name);
				syscall_cmd_window_write(" is not a file");
				if(src_fsnode != NULL)
					syscall_ufree(src_fsnode);
				ret_val = -1;
				goto end;
			}
			FS_NODE * dest_fsnode = (FS_NODE *)syscall_umalloc(sizeof(FS_NODE));
			ret = syscall_finddir_fs_safe(fs_root, dest_name, dest_fsnode);
			if(ret == -1)
			{
				FS_NODE * tmp_node;
				if((tmp_node = create_empty_file(fs_root, dest_name)) == NULL)
				{
					if(src_fsnode != NULL)
						syscall_ufree(src_fsnode);
					if(dest_fsnode != NULL)
						syscall_ufree(dest_fsnode);
					ret_val = -1;
					goto end;
				}
				memcpy((UINT8 *)dest_fsnode,(UINT8 *)tmp_node,sizeof(FS_NODE));
			}
			else if(dest_fsnode->write == NULL)
			{
				syscall_cmd_window_write(dest_name);
				syscall_cmd_window_write(" can not be write");
				syscall_ufree(src_fsnode);
				if(src_fsnode != NULL)
					syscall_ufree(src_fsnode);
				if(dest_fsnode != NULL)
					syscall_ufree(dest_fsnode);
				ret_val = -1;
				goto end;
			}
			UINT8 * buf = (UINT8 *)syscall_umalloc(src_fsnode->length);
			syscall_read_fs(src_fsnode,0,src_fsnode->length,buf);
			syscall_write_fs(dest_fsnode,0,src_fsnode->length,buf);

			/*UINT8 * buf_dest = (UINT8 *)syscall_umalloc(src_fsnode->length); // debug
			syscall_read_fs(dest_fsnode,0,src_fsnode->length,buf_dest); // debug
			if(memcmp(buf_dest, buf, src_fsnode->length) != 0) // debug
			{
				syscall_cmd_window_write("src not equal to dest detect!"); // debug
			}
			syscall_ufree(buf_dest);*/

			syscall_ufree(buf);
			if(src_fsnode != NULL)
				syscall_ufree(src_fsnode);
			if(dest_fsnode != NULL)
				syscall_ufree(dest_fsnode);
			syscall_cmd_window_write("copy content of ");
			syscall_cmd_window_write(src_name);
			syscall_cmd_window_write(" to ");
			syscall_cmd_window_write(dest_name);
			syscall_cmd_window_write(" success");
			ret_val = 0;
			goto end;
		}
	}
	else if(argc == 4 && !strcmp(argv[1],"-rename"))
	{
		char * src_name = argv[2];
		char * dest_name = argv[3];
		int ret;
		ret = syscall_finddir_fs_safe(fs_root, src_name, ret_fsnode);
		if(ret == -1)
		{
			syscall_cmd_window_write(src_name);
			syscall_cmd_window_write(" is not exist");
			ret_val = -1;
			goto end;
		}
		if(syscall_rename_fs(ret_fsnode, dest_name) == 0)
		{
			syscall_cmd_window_write(src_name);
			syscall_cmd_window_write(" rename failed");
			ret_val = -1;
			goto end;
		}
		else
		{
			char * c = dest_name;
			syscall_cmd_window_write(src_name);
			syscall_cmd_window_write(" rename to ");
			for(;(*c)!=0;c++)
			{
				if((*c) == '/')
					(*c) = '_';
			}
			syscall_cmd_window_write(dest_name);
			syscall_cmd_window_write(" success");
			ret_val = 0;
			goto end;
		}
	}

	syscall_cmd_window_write("usage: file [filename][src_file dest_file][-rm delfile][-rename src_name dest_name]");

end:
	if(ret_fsnode != NULL)
		syscall_ufree(ret_fsnode);
	return ret_val;
}

