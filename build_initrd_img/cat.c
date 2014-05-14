// cat.c -- 显示出指定文件的内容

#include "common.h"
#include "syscall.h"
#include "fs.h"

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(task);

	if(argc == 2)
	{
		FS_NODE * fs_root = (FS_NODE *)syscall_get_fs_root();
		FS_NODE *fsnode = (FS_NODE *)syscall_finddir_fs(fs_root, argv[1]);
		if (fsnode == 0)
		{
			syscall_monitor_write("\"");
			syscall_monitor_write(argv[1]);
			syscall_monitor_write("\" is not exist in your file system ");
		}
		else if ((fsnode->flags & 0x7) == FS_FILE)
		{
			CHAR * buf = (CHAR *)syscall_umalloc(fsnode->length + 1);
			syscall_read_fs(fsnode,0,fsnode->length,buf);
			buf[fsnode->length] = '\0';
			syscall_monitor_write(buf);
			syscall_ufree(buf);
		}
		else if ((fsnode->flags & 0x7) == FS_DIRECTORY)
		{
			syscall_monitor_write("\"");
			syscall_monitor_write(argv[1]);
			syscall_monitor_write("\" is a directory ");
		}
	}
	else
	{
		syscall_monitor_write("usage: cat <filename>");
	}

	return 0;
}

