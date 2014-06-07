// ls.c -- 显示出文件列表

#include "common.h"
#include "syscall.h"
#include "fs.h"

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(task);

	SINT32 i = 0;
	DIRENT *node = 0;
	FS_NODE * fs_root = (FS_NODE *)syscall_get_fs_root();
	if(argc == 1)
	{
		while ( (node = (DIRENT *)syscall_readdir_fs(fs_root, i)) != 0)
		{
			FS_NODE *fsnode = (FS_NODE *)syscall_finddir_fs(fs_root, node->name);
			if ((fsnode->flags & 0x7) == FS_FILE)
			{
				syscall_monitor_write(node->name);
				syscall_monitor_put(' ');
			}
			else if((fsnode->flags & 0x7) == FS_DIRECTORY && !strcmp(node->name,"iso"))
			{
				syscall_monitor_write("[");
				syscall_monitor_write(node->name);
				syscall_monitor_write("] ");
			}
			i++;
		}
	}
	else if(argc == 2)
	{
		FS_NODE * tmp_node = (FS_NODE *)syscall_finddir_fs(fs_root, argv[1]);
		if(tmp_node == NULL)
		{
			syscall_monitor_write("\"");
			syscall_monitor_write(argv[1]);
			syscall_monitor_write("\" is not exist in your file system ");
			return 0;
		}
		FS_NODE * arg_node = (FS_NODE *)syscall_umalloc(sizeof(FS_NODE));
		memcpy((UINT8 *)arg_node,(UINT8 *)tmp_node,sizeof(FS_NODE));
		if((arg_node->flags & 0x7) == FS_FILE)
		{
			syscall_monitor_write(arg_node->name);
			syscall_monitor_write(" length(byte): ");
			syscall_monitor_write_dec(arg_node->length);
		}
		else if((arg_node->flags & 0x7) == FS_DIRECTORY)
		{
			i = 0;
			while ( (node = (DIRENT *)syscall_readdir_fs(arg_node, i)) != 0)
			{
				FS_NODE *fsnode = (FS_NODE *)syscall_finddir_fs(arg_node, node->name);
				if ((fsnode->flags & 0x7) == FS_FILE)
				{
					syscall_monitor_write(node->name);
					syscall_monitor_put(' ');
				}
				else if((fsnode->flags & 0x7) == FS_DIRECTORY)
				{
					syscall_monitor_write("[");
					syscall_monitor_write(node->name);
					syscall_monitor_write("] ");
				}
				i++;
			}
		}
		syscall_ufree(arg_node);
	}
	else
	{
		syscall_monitor_write("usage: ls [path]");
	}

	return 0;
}

