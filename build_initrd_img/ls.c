// ls.c -- 显示出文件列表

#include "common.h"
#include "syscall.h"
#include "fs.h"

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(task);

	SINT32 i = 0;
	DIRENT * node = (DIRENT *)syscall_umalloc(sizeof(DIRENT));
	FS_NODE * fsnode = (FS_NODE *)syscall_umalloc(sizeof(FS_NODE));
	FS_NODE * fs_root = (FS_NODE *)syscall_get_fs_root();
	if(argc == 1)
	{
		while (syscall_readdir_fs_safe(fs_root, i, node) != -1)
		{
			int ret = syscall_finddir_fs_safe(fs_root, node->name, fsnode);
			if(ret == -1)
				;
			else if ((fsnode->flags & 0x7) == FS_FILE)
			{
				syscall_cmd_window_write(node->name);
				syscall_cmd_window_put(' ');
			}
			else if((fsnode->flags & 0x7) == FS_DIRECTORY && (!strcmp(node->name,"iso") 
				|| !strcmp(node->name,"hd")))
			{
				syscall_cmd_window_write("[");
				syscall_cmd_window_write(node->name);
				syscall_cmd_window_write("] ");
			}
			i++;
		}
		goto end;
	}
	else if(argc == 2)
	{
		FS_NODE * arg_node = (FS_NODE *)syscall_umalloc(sizeof(FS_NODE));
		int ret = syscall_finddir_fs_safe(fs_root, argv[1], arg_node);
		if(ret == -1)
		{
			syscall_cmd_window_write("\"");
			syscall_cmd_window_write(argv[1]);
			syscall_cmd_window_write("\" is not exist in your file system ");
			if(arg_node != NULL)
				syscall_ufree(arg_node);
			goto end;
		}
		if((arg_node->flags & 0x7) == FS_FILE)
		{
			syscall_cmd_window_write(arg_node->name);
			syscall_cmd_window_write(" length(byte): ");
			syscall_cmd_window_write_dec(arg_node->length);
		}
		else if((arg_node->flags & 0x7) == FS_DIRECTORY)
		{
			i = 0;
			while (syscall_readdir_fs_safe(arg_node, i, node) != -1)
			{
				ret = syscall_finddir_fs_safe(arg_node, node->name, fsnode);
				if(ret == -1)
					;
				else if ((fsnode->flags & 0x7) == FS_FILE)
				{
					syscall_cmd_window_write(node->name);
					syscall_cmd_window_put(' ');
				}
				else if((fsnode->flags & 0x7) == FS_DIRECTORY)
				{
					syscall_cmd_window_write("[");
					syscall_cmd_window_write(node->name);
					syscall_cmd_window_write("] ");
				}
				i++;
			}
		}
		syscall_ufree(arg_node);
	}
	else
	{
		syscall_cmd_window_write("usage: ls [path]");
	}

end:
	if(node != NULL)
		syscall_ufree(node);
	if(fsnode != NULL)
		syscall_ufree(fsnode);
	return 0;
}

