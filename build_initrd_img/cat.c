// cat.c -- 显示出指定文件的内容

#include "common.h"
#include "syscall.h"
#include "fs.h"

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(task);

	FS_NODE * fsnode = (FS_NODE *)syscall_umalloc(sizeof(FS_NODE));

	if(argc == 2 || argc == 3)
	{
		FS_NODE * fs_root = (FS_NODE *)syscall_get_fs_root();
		int ret = syscall_finddir_fs_safe(fs_root, argv[1], fsnode);
		UINT32 show_char_num = 0;
		if(argc == 3)
			show_char_num = strToUInt(argv[2]);

		if (ret == -1)
		{
			syscall_cmd_window_write("\"");
			syscall_cmd_window_write(argv[1]);
			syscall_cmd_window_write("\" is not exist in your file system ");
		}
		else if ((fsnode->flags & 0x7) == FS_FILE)
		{
			CHAR * buf = (CHAR *)syscall_umalloc(fsnode->length + 1);
			syscall_read_fs(fsnode,0,fsnode->length,buf);
			if(show_char_num > 0 && show_char_num < fsnode->length)
				buf[show_char_num] = '\0';
			buf[fsnode->length] = '\0';
			syscall_cmd_window_write(buf);
			syscall_ufree(buf);
		}
		else if ((fsnode->flags & 0x7) == FS_DIRECTORY)
		{
			syscall_cmd_window_write("\"");
			syscall_cmd_window_write(argv[1]);
			syscall_cmd_window_write("\" is a directory ");
		}
	}
	else
	{
		syscall_cmd_window_write("usage: cat <filename> [show char num]");
	}

	if(fsnode != NULL)
		syscall_ufree(fsnode);

	return 0;
}

