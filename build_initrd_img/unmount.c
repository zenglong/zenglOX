// unmount.c -- 卸载文件系统的程式

#include "common.h"
#include "syscall.h"

/*
使用unmount iso命令可以卸载iso目录
使用unmount hd命令可以卸载hd目录
*/

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(task);
	if(argc == 2 && strcmp(argv[1],"iso")==0)
	{
		int ret = syscall_unmount_iso();
		if(ret == 0)
			syscall_cmd_window_write("unmount iso success!");
		else
			syscall_cmd_window_write("unmount iso failed...");
	}
	else if(argc == 2 && strcmp(argv[1],"hd")==0)
	{
		int ret = syscall_unmount_zenglfs();
		if(ret == 0)
			syscall_cmd_window_write("unmount hd success!");
		else
			syscall_cmd_window_write("unmount hd failed...");
	}
	else 
		syscall_cmd_window_write("usage: unmount [iso][hd]");
	return 0;
}

