// mount.c -- 挂载文件系统的程式

#include "common.h"
#include "syscall.h"

/*
mount工具可以加载cdrom到iso目录，也可以将硬盘里格式化过的zenglfs分区给加载到hd目录，
当使用mount iso命令时，会通过syscall_mount_iso()系统调用将cdrom加载到iso目录
当使用类似 mount hd 0 1 命令时，则可以将0号IDE硬盘的1号分区给加载到hd目录
iso和hd是内核固定设置好的目录，目前不可以加载到其他的目录名上
*/

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(task);
	if(argc == 2 && strcmp(argv[1],"iso")==0)
	{
		int ret = syscall_mount_iso();
		if(ret != 0)
			syscall_cmd_window_write("mount iso to [iso] success! you can use \"ls iso\" to see "
						"the contents of iso directory");
		else
			syscall_cmd_window_write("mount iso failed...");
	}
	else if(argc == 4 && strcmp(argv[1],"hd")==0)
	{
		UINT32 ide_index = strToUInt(argv[2]);
		UINT32 pt = strToUInt(argv[3]);
		int ret = syscall_mount_zenglfs(ide_index, pt);
		if(ret != 0)
			syscall_cmd_window_write("mount to [hd] success! you can use \"ls hd\" to see "
						"the contents of hd directory");
	}
	else 
		syscall_cmd_window_write("usage: mount [iso][hd ide_index pt]");
	return 0;
}

