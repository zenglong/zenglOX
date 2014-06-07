// unmount.c -- 卸载文件系统的程式

#include "common.h"
#include "syscall.h"

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(task);
	if(argc == 2 && strcmp(argv[1],"iso")==0)
	{
		int ret = syscall_unmount_iso();
		if(ret == 0)
			syscall_monitor_write("unmount iso success!");
		else
			syscall_monitor_write("unmount iso failed...");
	}
	else 
		syscall_monitor_write("usage: unmount iso");
	return 0;
}

