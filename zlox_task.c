// task.c Implements the functionality needed to multitask.

#include "zlox_task.h"
#include "zlox_paging.h"
#include "zlox_kheap.h"
#include "zlox_descriptor_tables.h"

// The currently running task.
volatile ZLOX_TASK * current_task = 0;

// The start of the task linked list.
volatile ZLOX_TASK * ready_queue = 0;

volatile ZLOX_TASK * input_focus_task = 0;

// zlox_descriptor_tables.c
extern ZLOX_TSS_ENTRY tss_entry_double_fault;

// 所需的zlox_paging.c里的全局变量
extern ZLOX_PAGE_DIRECTORY * kernel_directory;
extern ZLOX_PAGE_DIRECTORY * current_directory;
extern ZLOX_UINT32 initial_esp;
extern ZLOX_UINT32 _zlox_read_eip();
extern ZLOX_VOID _zlox_switch_task_do(ZLOX_UINT32,ZLOX_UINT32,ZLOX_UINT32,ZLOX_UINT32);

// The next available process ID.
//ZLOX_UINT32 next_pid = 1;

ZLOX_PID_REUSE_LIST pid_reuse_list = {0};

ZLOX_UINT32 task_count = 0;

ZLOX_VOID zlox_move_stack(ZLOX_VOID * new_stack_start, ZLOX_UINT32 size);
ZLOX_SINT32 zlox_push_pid(ZLOX_SINT32 pid);
ZLOX_SINT32 zlox_pop_pid(ZLOX_BOOL needPop);

ZLOX_VOID zlox_initialise_tasking()
{
	// Rather important stuff happening, no interrupts please!
	asm volatile("cli");

	// Relocate the stack so we know where it is.
	zlox_move_stack((ZLOX_VOID *)0xE0000000, 0x2000);

	// Initialise the first task (kernel task)
	current_task = ready_queue = (ZLOX_TASK *)zlox_kmalloc(sizeof(ZLOX_TASK));
	current_task->sign = ZLOX_TSK_SIGN;
	//current_task->id = next_pid++;
	current_task->id = 1;
	current_task->esp = current_task->ebp = 0;
	current_task->eip = 0;
	current_task->init_esp = 0xE0000000;
	current_task->page_directory = current_directory;
	current_task->next = 0;
	current_task->prev = 0;
	current_task->parent = 0;
	current_task->kernel_stack = 0xF0000000;
	for(ZLOX_UINT32 tmp_esp = current_task->kernel_stack - ZLOX_KERNEL_STACK_SIZE; 
		tmp_esp < current_task->kernel_stack ;tmp_esp += 0x1000)
	{
		zlox_page_copy(tmp_esp);
	}
	zlox_memset((ZLOX_UINT8 *)(&current_task->msglist),0,sizeof(ZLOX_TASK_MSG_LIST));
	zlox_memset((ZLOX_UINT8 *)(&current_task->link_maps),0,sizeof(ZLOX_ELF_LINK_MAP_LIST));
	current_task->status = ZLOX_TS_RUNNING;
	current_task->args = 0;
	task_count++;

	tss_entry_double_fault.cr3 = current_directory->physicalAddr;
	tss_entry_double_fault.esp0 = current_task->kernel_stack;
	tss_entry_double_fault.esp = current_task->kernel_stack;
	tss_entry_double_fault.ebp = current_task->kernel_stack;

	// Reenable interrupts.
	asm volatile("sti");
}

ZLOX_VOID zlox_move_stack(ZLOX_VOID * new_stack_start, ZLOX_UINT32 size)
{
	ZLOX_UINT32 i;
	// Allocate some space for the new stack.
	for( i = (ZLOX_UINT32)new_stack_start;
		i >= ((ZLOX_UINT32)new_stack_start-size);
		i -= 0x1000)
	{
		// General-purpose stack is in user-mode.
		zlox_alloc_frame( zlox_get_page(i, 1, current_directory), 0 /* User mode */, 1 /* Is writable */ );
	}

	// Flush the TLB(translation lookaside buffer) by reading and writing the page directory address again.
	ZLOX_UINT32 pd_addr;
	asm volatile("mov %%cr3, %0" : "=r" (pd_addr));
	asm volatile("mov %0, %%cr3" : : "r" (pd_addr));

	// Old ESP and EBP, read from registers.
	ZLOX_UINT32 old_stack_pointer; asm volatile("mov %%esp, %0" : "=r" (old_stack_pointer));
	ZLOX_UINT32 old_base_pointer;  asm volatile("mov %%ebp, %0" : "=r" (old_base_pointer));

	// Offset to add to old stack addresses to get a new stack address.
	ZLOX_UINT32 offset = (ZLOX_UINT32)new_stack_start - initial_esp;

	// New ESP and EBP.
	ZLOX_UINT32 new_stack_pointer = old_stack_pointer + offset;
	ZLOX_UINT32 new_base_pointer  = old_base_pointer  + offset;

	// Copy the stack.
	zlox_memcpy((ZLOX_UINT8 *)new_stack_pointer, (ZLOX_UINT8 *)old_stack_pointer, initial_esp-old_stack_pointer);

	// Backtrace through the original stack, copying new values into
	// the new stack.  
	for(i = (ZLOX_UINT32)new_base_pointer; 
		(i >= new_stack_pointer && (i < (ZLOX_UINT32)new_stack_start)); )
	{
		ZLOX_UINT32 tmp = * (ZLOX_UINT32*)i;
		// it is a base pointer and remap it. 
		if (( old_stack_pointer < tmp) && (tmp < initial_esp))
		{
			tmp = tmp + offset;
			ZLOX_UINT32 *tmp2 = (ZLOX_UINT32 *)i;
			*tmp2 = tmp;
			i = tmp;
		}
		else
			break;
	}

	// Change stacks.
	asm volatile("mov %0, %%esp" : : "r" (new_stack_pointer));
	asm volatile("mov %0, %%ebp" : : "r" (new_base_pointer));
}

ZLOX_VOID zlox_switch_task()
{
	// If we haven't initialised tasking yet, just return.
	if (!current_task)
		return;

	// Read esp, ebp now for saving later on.
	ZLOX_UINT32 esp, ebp, eip;
	asm volatile("mov %%esp, %0" : "=r"(esp));
	asm volatile("mov %%ebp, %0" : "=r"(ebp));

	// Read the instruction pointer. We do some cunning logic here:
	// One of two things could have happened when this function exits - 
	//   (a) We called the function and it returned the EIP as requested.
	//   (b) We have just switched tasks, and because the saved EIP is essentially
	//	   the instruction after read_eip(), it will seem as if read_eip has just
	//	   returned.
	// In the second case we need to return immediately. To detect it we put a dummy
	// value in EAX further down at the end of this function. As C returns values in EAX,
	// it will look like the return value is this dummy value! (0x12345).
	eip = _zlox_read_eip();

	// Have we just switched tasks?
	if (eip == 0x12345)
		return;

	// No, we didn't switch tasks. Let's save some register values and switch.
	current_task->eip = eip;
	current_task->esp = esp;
	current_task->ebp = ebp;
	
	volatile ZLOX_TASK * orig_task = current_task;
	do
	{
		// Get the next task to run.
		current_task = current_task->next;
		// If we fell off the end of the linked list start again at the beginning.
		if (!current_task) 
			current_task = ready_queue;

		// 如果找到的下一个任务的状态是运行状态，则切换到该任务
		if(current_task->status == ZLOX_TS_RUNNING)
			break;
		// 如果当前任务有需要进行结束的子任务的话，就唤醒该任务
		else if(current_task->status == ZLOX_TS_WAIT && current_task->msglist.finish_task_num > 0)
		{
			current_task->status = ZLOX_TS_RUNNING;
			break;
		}
		// 如果任务列表里所有的任务都不是运行状态，则将第一个任务唤醒，并切换到第一个任务
		else if(current_task == orig_task)
		{
			current_task = ready_queue;
			current_task->status = ZLOX_TS_RUNNING;
			break;
		}
	}while(ZLOX_TRUE);

	eip = current_task->eip;
	esp = current_task->esp;
	ebp = current_task->ebp;

	// Make sure the memory manager knows we've changed page directory.
	current_directory = current_task->page_directory;
	
	// Change our kernel stack over.
	zlox_set_kernel_stack(current_task->kernel_stack);

	_zlox_switch_task_do(eip,esp,ebp,current_directory->physicalAddr);
}

ZLOX_SINT32 zlox_fork()
{
	// We are modifying kernel structures, and so cannot interrupt
	//asm volatile("cli");

	// Take a pointer to this process' task struct for later reference.
	ZLOX_TASK * parent_task = (ZLOX_TASK *)current_task;

	// Clone the address space.
	ZLOX_PAGE_DIRECTORY * directory = zlox_clone_directory(current_directory,0);

	// Create a new process.
	ZLOX_TASK * new_task = (ZLOX_TASK *)zlox_kmalloc(sizeof(ZLOX_TASK));

	new_task->sign = ZLOX_TSK_SIGN;
	new_task->id = zlox_pop_pid(ZLOX_TRUE);
	if(new_task->id == 0)
	{
		new_task->id = task_count+1;
	}
	new_task->esp = new_task->ebp = 0;
	new_task->eip = 0;
	new_task->init_esp = current_task->init_esp;
	new_task->page_directory = directory;
	new_task->kernel_stack = current_task->kernel_stack;
	zlox_memset((ZLOX_UINT8 *)(&new_task->msglist),0,sizeof(ZLOX_TASK_MSG_LIST));
	zlox_memset((ZLOX_UINT8 *)(&new_task->link_maps),0,sizeof(ZLOX_ELF_LINK_MAP_LIST));
	new_task->status = ZLOX_TS_RUNNING;
	new_task->args = 0;
	new_task->next = 0;
	new_task->parent = (ZLOX_TASK *)current_task;

	// Add it to the end of the ready queue.
	ZLOX_TASK *tmp_task = (ZLOX_TASK *)ready_queue;
	while (tmp_task->next)
		tmp_task = tmp_task->next;
	tmp_task->next = new_task;
	new_task->prev = tmp_task;
	task_count++;

	// This will be the entry point for the new process.
	ZLOX_UINT32 eip = _zlox_read_eip();

	// We could be the parent or the child here - check.
	if (current_task == parent_task)
	{
		// We are the parent, so set up the esp/ebp/eip for our child.
		ZLOX_UINT32 esp; asm volatile("mov %%esp, %0" : "=r"(esp));
		ZLOX_UINT32 ebp; asm volatile("mov %%ebp, %0" : "=r"(ebp));
		ZLOX_UINT32 tmp_esp;
		// 为新进程拷贝内核栈数据
		current_directory = new_task->page_directory;		
		for(tmp_esp = new_task->kernel_stack - ZLOX_KERNEL_STACK_SIZE; tmp_esp < new_task->kernel_stack ;tmp_esp += 0x1000)
		{
			zlox_page_copy(tmp_esp);
		}
		// 为新进程拷贝用户栈数据
		for(tmp_esp = new_task->init_esp - ZLOX_USER_STACK_SIZE; tmp_esp < new_task->init_esp ;tmp_esp += 0x1000)
		{
			zlox_page_copy(tmp_esp);
		}
		current_directory = current_task->page_directory;

		new_task->esp = esp;
		new_task->ebp = ebp;
		new_task->eip = eip;
		//asm volatile("sti");

		return new_task->id;
	}
	else
	{
		// We are the child.
		return 0;
	}
}

ZLOX_SINT32 zlox_getpid()
{
	return current_task->id;
}

ZLOX_VOID zlox_switch_to_user_mode()
{
	// Set up our kernel stack.
	zlox_set_kernel_stack(current_task->kernel_stack);
	
	// Set up a stack structure for switching to user mode.
	asm volatile(
		"cli\n\t"
		"mov $0x23, %ax\n\t"
		"mov %ax, %ds\n\t"
		"mov %ax, %es\n\t"
		"mov %ax, %fs\n\t"
		"mov %ax, %gs\n\t"
		"mov %esp, %eax\n\t"
		"pushl $0x23\n\t"
		"pushl %eax\n\t"
		"pushf\n\t"
		"pop %eax\n\t"	// Get EFLAGS back into EAX. The only way to read EFLAGS is to pushf then pop.
		"orl $0x200,%eax\n\t"	// Set the IF flag.
		"pushl %eax\n\t"	// Push the new EFLAGS value back onto the stack.
		"pushl $0x1B\n\t"
		"push $1f\n\t"
		"iret\n"
	"1:"
		); 
	  
}

// 将消息压入消息列表，消息列表里的消息是采用的先进入的消息先处理的方式
ZLOX_SINT32 zlox_push_tskmsg(ZLOX_TASK_MSG_LIST * msglist , ZLOX_TASK_MSG * msg)
{
	if(!msglist->isInit) // 如果没进行过初始化，则初始化消息列表
	{
		msglist->size = ZLOX_TSK_MSGLIST_SIZE;
		msglist->ptr = (ZLOX_TASK_MSG *)zlox_kmalloc(msglist->size * sizeof(ZLOX_TASK_MSG));
		zlox_memset((ZLOX_UINT8 *)msglist->ptr,0,msglist->size * sizeof(ZLOX_TASK_MSG));		
		msglist->count = 0;
		msglist->cur = 0;
		msglist->isInit = ZLOX_TRUE;
	}
	else if(msglist->count == msglist->size) // 如果消息列表数目达到了当前容量的上限，则对消息列表进行动态扩容
	{
		ZLOX_TASK_MSG * tmp_ptr;
		msglist->size += ZLOX_TSK_MSGLIST_SIZE;
		tmp_ptr = (ZLOX_TASK_MSG *)zlox_kmalloc(msglist->size * sizeof(ZLOX_TASK_MSG));
		zlox_memcpy((ZLOX_UINT8 *)(tmp_ptr + msglist->cur),(ZLOX_UINT8 *)(msglist->ptr + msglist->cur),
				(msglist->count - msglist->cur) * sizeof(ZLOX_TASK_MSG));
		if(msglist->cur > 0)
		{
			zlox_memcpy((ZLOX_UINT8 *)(tmp_ptr + msglist->count),(ZLOX_UINT8 *)msglist->ptr,
				msglist->cur * sizeof(ZLOX_TASK_MSG));
		}
		zlox_kfree(msglist->ptr);
		msglist->ptr = tmp_ptr;
	}
	
	ZLOX_UINT32 index = ((msglist->cur + msglist->count) < msglist->size) ? (msglist->cur + msglist->count) : 
				(msglist->cur + msglist->count - msglist->size);
	msglist->ptr[index] = *msg;
	msglist->count++;
	return 0;
}

// 获取消息列表中的消息,needPop参数表示是否需要将消息弹出消息列表
ZLOX_TASK_MSG * zlox_pop_tskmsg(ZLOX_TASK_MSG_LIST * msglist,ZLOX_BOOL needPop)
{
	ZLOX_TASK_MSG * ret;
	if(msglist->count <= 0)
		return ZLOX_NULL;

	ret = &msglist->ptr[msglist->cur];
	if(needPop)
	{
		msglist->cur = ((msglist->cur + 1) < msglist->size) ? (msglist->cur + 1) : 0;
		msglist->count = (msglist->count - 1) > 0 ? (msglist->count - 1) : 0;
	}
	return ret;
}

// 向task任务发送消息
ZLOX_SINT32 zlox_send_tskmsg(ZLOX_TASK * task , ZLOX_TASK_MSG * msg)
{
	ZLOX_SINT32 retval = -1;
	if((task == ZLOX_NULL) || (task->sign != ZLOX_TSK_SIGN))
		return retval;

	if(msg->type == ZLOX_MT_KEYBOARD)
	{
		if(msg->keyboard.type == ZLOX_MKT_ASCII || 
			msg->keyboard.type == ZLOX_MKT_KEY)
		{
			retval = zlox_push_tskmsg(&task->msglist,msg);
		}
	}
	else if(msg->type == ZLOX_MT_TASK_FINISH)
	{
		retval = zlox_push_tskmsg(&task->msglist,msg);
		task->msglist.finish_task_num++;
	}
	return retval;
}

// 从task任务中获取消息
ZLOX_SINT32 zlox_get_tskmsg(ZLOX_TASK * task,ZLOX_TASK_MSG * msgbuf,ZLOX_BOOL needPop)
{
	ZLOX_TASK_MSG * tskmsg;
	if(task->sign != ZLOX_TSK_SIGN)
		return -1;
	if(msgbuf == ZLOX_NULL)
		return -1;
	tskmsg = zlox_pop_tskmsg(&task->msglist,needPop);
	if(tskmsg == ZLOX_NULL)
		return 0;
	if(tskmsg->type == ZLOX_MT_TASK_FINISH)
		task->msglist.finish_task_num--;
	zlox_memcpy((ZLOX_UINT8 *)msgbuf,(ZLOX_UINT8 *)tskmsg,sizeof(ZLOX_TASK_MSG));
	return 1;
}

// 将task任务设置为wait等待状态
ZLOX_SINT32 zlox_wait(ZLOX_TASK * task)
{
	// 将任务状态设置为等待状态
	task->status = ZLOX_TS_WAIT;
	// 如果task是当前任务，则进行任务切换
	if(task == current_task)
		zlox_switch_task();
	return 0;
}

// 将task任务设置为获取键盘之类的输入数据的任务，这样键盘就会向该任务发送按键消息
ZLOX_SINT32 zlox_set_input_focus(ZLOX_TASK * task)
{
	if(task->sign != ZLOX_TSK_SIGN)
		return -1;
	input_focus_task = task;
	return 0;
}

// 获取当前任务的指针值
ZLOX_TASK * zlox_get_currentTask()
{
	return (ZLOX_TASK *)current_task;
}

// 结束当前任务，并向父任务或首任务发送结束消息
ZLOX_SINT32 zlox_exit(ZLOX_SINT32 exit_code)
{
	ZLOX_TASK * parent_task = current_task->parent;
	ZLOX_TASK * notify_task = parent_task;
	ZLOX_TASK_MSG ascii_msg = {0};
	if(parent_task == 0)
		return -1;

	// 如果父任务处于wait等待状态，则将其唤醒，并发送结束消息
	if(parent_task->status == ZLOX_TS_WAIT)
		parent_task->status = ZLOX_TS_RUNNING;
	// 如果父任务已经结束了，则向第一个任务发送结束消息
	else if(parent_task->status == ZLOX_TS_FINISH)
	{
		if(ready_queue->status == ZLOX_TS_WAIT)
			ready_queue->status = ZLOX_TS_RUNNING;
		else if(ready_queue->status == ZLOX_TS_FINISH)
		{
			current_task->status = ZLOX_TS_ZOMBIE; // 如果没有任务可以进行通知的话，则当前任务就变为僵死任务
			return -1;
		}
		notify_task = (ZLOX_TASK *)ready_queue;
	}

	current_task->status = ZLOX_TS_FINISH;
	ascii_msg.type = ZLOX_MT_TASK_FINISH;
	ascii_msg.finish_task.exit_task = (ZLOX_TASK *)current_task;
	ascii_msg.finish_task.exit_code = exit_code;
	zlox_send_tskmsg(notify_task,&ascii_msg);
	zlox_switch_task();
	return 0;
}

// 将需要结束的任务的相关资源给释放掉，并从任务列表里移除
ZLOX_SINT32 zlox_finish(ZLOX_TASK * task)
{
	ZLOX_TASK * prev_task;
	ZLOX_TASK * next_task;
	if(task->sign != ZLOX_TSK_SIGN ||
		task->status != ZLOX_TS_FINISH)
		return -1;
	zlox_free_directory(task->page_directory);
	zlox_kfree(task->msglist.ptr);
	zlox_elf_free_lnk_maplst(&task->link_maps);
	zlox_kfree(task->link_maps.ptr);
	zlox_kfree(task->args);
	prev_task = task->prev;
	next_task = task->next;
	if(prev_task != 0 && prev_task->sign == ZLOX_TSK_SIGN)
		prev_task->next = next_task;
	if(next_task != 0 && next_task->sign == ZLOX_TSK_SIGN)
		next_task->prev = prev_task;
	zlox_push_pid(task->id);
	zlox_memset((ZLOX_UINT8 *)task,0,sizeof(ZLOX_TASK));
	zlox_kfree(task);
	task_count--;
	return 0;
}

// 获取task任务的参数
ZLOX_UINT32 zlox_get_args(ZLOX_TASK * task)
{
	if(task->sign != ZLOX_TSK_SIGN)
		return -1;
	return (ZLOX_UINT32)task->args;
}

// 获取task任务用户栈的初始值
ZLOX_UINT32 zlox_get_init_esp(ZLOX_TASK * task)
{
	if(task->sign != ZLOX_TSK_SIGN)
		return -1;
	return task->init_esp;
}

ZLOX_SINT32 zlox_push_pid(ZLOX_SINT32 pid)
{
	if(!pid_reuse_list.isInit) // 如果没进行过初始化，则初始化PID重利用列表
	{
		pid_reuse_list.size = ZLOX_PID_REUSE_LIST_SIZE;
		pid_reuse_list.ptr = (ZLOX_SINT32 *)zlox_kmalloc(pid_reuse_list.size * sizeof(ZLOX_SINT32));
		zlox_memset((ZLOX_UINT8 *)pid_reuse_list.ptr,0,pid_reuse_list.size * sizeof(ZLOX_SINT32));		
		pid_reuse_list.count = 0;
		pid_reuse_list.isInit = ZLOX_TRUE;
	}
	else if(pid_reuse_list.count == pid_reuse_list.size) // 如果PID重利用列表数目达到了当前容量的上限，则对PID重利用列表进行动态扩容
	{
		ZLOX_SINT32 * tmp_ptr;
		pid_reuse_list.size += ZLOX_PID_REUSE_LIST_SIZE;
		tmp_ptr = (ZLOX_SINT32 *)zlox_kmalloc(pid_reuse_list.size * sizeof(ZLOX_SINT32));
		zlox_memcpy((ZLOX_UINT8 *)(tmp_ptr + ZLOX_PID_REUSE_LIST_SIZE),(ZLOX_UINT8 *)pid_reuse_list.ptr,
				(pid_reuse_list.size - ZLOX_PID_REUSE_LIST_SIZE) * sizeof(ZLOX_SINT32));
		zlox_kfree(pid_reuse_list.ptr);
		pid_reuse_list.ptr = tmp_ptr;
	}
	
	ZLOX_UINT32 index = pid_reuse_list.size - pid_reuse_list.count - 1;
	pid_reuse_list.ptr[index] = pid;
	pid_reuse_list.count++;
	return 0;
}

ZLOX_SINT32 zlox_pop_pid(ZLOX_BOOL needPop)
{
	ZLOX_SINT32 ret;
	if(pid_reuse_list.count <= 0)
		return 0;

	ret = pid_reuse_list.ptr[pid_reuse_list.size - pid_reuse_list.count];
	if(needPop)
	{
		pid_reuse_list.count = (pid_reuse_list.count - 1) > 0 ? (pid_reuse_list.count - 1) : 0;
	}
	return ret;
}

