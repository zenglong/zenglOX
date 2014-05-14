// ls.c -- 显示出文件列表

#include "common.h"
#include "syscall.h"
#include "fs.h"

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(task);
	UNUSED(argc);
	UNUSED(argv);

	SINT32 i = 0;
	DIRENT *node = 0;
	FS_NODE * fs_root = (FS_NODE *)syscall_get_fs_root();
	while ( (node = (DIRENT *)syscall_readdir_fs(fs_root, i)) != 0)
	{
		FS_NODE *fsnode = (FS_NODE *)syscall_finddir_fs(fs_root, node->name);
		if ((fsnode->flags & 0x7) == FS_FILE)
		{
			syscall_monitor_write(node->name);
			syscall_monitor_put(' ');
		}
		i++;
	}

	return 0;
}

