// cat.c -- 显示出指定文件的内容

#include "common.h"
#include "syscall.h"

extern UINT32 _commontest_gl; // debug global value of other dynamic library

static int recursion()
{
	CHAR test[128] = {0};
	test[0] = test[0] + 0;
	recursion(); // 无穷递归，迫使用户栈溢出，用户栈溢出可以通过普通的page fault检测出来
	return 0;
}

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(task);

	_commontest_gl = 3;

	if(argc > 1 && strcmp(argv[1],"-u")==0)
	{
		recursion();
	}
	else if(argc > 1 && strcmp(argv[1],"-x")==0)
	{
		// 无穷循环测试
		for(;;)
			;
	}
	else if(argc > 1 && strcmp(argv[1],"-k")==0)
	{
		syscall_overflow_test();
	}
	else
	{
		syscall_cmd_window_write("usage: testoverflow [-u][-x][-k]");
	}

	return 0;
}

