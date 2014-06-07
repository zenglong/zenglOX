// mount.c -- 挂载文件系统的程式

#include "common.h"
#include "syscall.h"

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(task);
	if(argc == 2 && strcmp(argv[1],"iso")==0)
	{
		int ret = syscall_mount_iso();
		if(ret != 0)
			syscall_monitor_write("mount iso to [iso] success! you can use \"ls iso\" to see "
						"the contents of iso directory");
		else
			syscall_monitor_write("mount iso failed...");
	}
	else 
		syscall_monitor_write("usage: mount iso");
	return 0;
}

