// task.c Implements the functionality needed to multitask.

#include "zlox_task.h"
#include "zlox_paging.h"
#include "zlox_kheap.h"
#include "zlox_descriptor_tables.h"

// The currently running task.
volatile ZLOX_TASK * current_task = 0;

// The start of the task linked list.
volatile ZLOX_TASK * ready_queue = 0;

// 所需的zlox_paging.c里的全局变量
extern ZLOX_PAGE_DIRECTORY * kernel_directory;
extern ZLOX_PAGE_DIRECTORY * current_directory;
extern ZLOX_UINT32 initial_esp;
extern ZLOX_UINT32 _zlox_read_eip();
extern ZLOX_VOID _zlox_switch_task_do(ZLOX_UINT32,ZLOX_UINT32,ZLOX_UINT32,ZLOX_UINT32);

// The next available process ID.
ZLOX_UINT32 next_pid = 1;

ZLOX_VOID zlox_move_stack(ZLOX_VOID * new_stack_start, ZLOX_UINT32 size);

ZLOX_VOID zlox_initialise_tasking()
{
	// Rather important stuff happening, no interrupts please!
	asm volatile("cli");

	// Relocate the stack so we know where it is.
	zlox_move_stack((ZLOX_VOID *)0xE0000000, 0x2000);

	// Initialise the first task (kernel task)
	current_task = ready_queue = (ZLOX_TASK *)zlox_kmalloc(sizeof(ZLOX_TASK));
	current_task->id = next_pid++;
	current_task->esp = current_task->ebp = 0;
	current_task->eip = 0;
	current_task->init_esp = 0xE0000000;
	current_task->page_directory = current_directory;
	current_task->next = 0;
	current_task->kernel_stack = zlox_kmalloc_a(ZLOX_KERNEL_STACK_SIZE);

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
	
	// Get the next task to run.
	current_task = current_task->next;
	// If we fell off the end of the linked list start again at the beginning.
	if (!current_task) 
		current_task = ready_queue;

	eip = current_task->eip;
	esp = current_task->esp;
	ebp = current_task->ebp;

	// Make sure the memory manager knows we've changed page directory.
	current_directory = current_task->page_directory;
	
	// Change our kernel stack over.
	zlox_set_kernel_stack(current_task->kernel_stack + ZLOX_KERNEL_STACK_SIZE);

	_zlox_switch_task_do(eip,esp,ebp,current_directory->physicalAddr);
}

ZLOX_SINT32 zlox_fork()
{
	// We are modifying kernel structures, and so cannot interrupt
	asm volatile("cli");

	// Take a pointer to this process' task struct for later reference.
	ZLOX_TASK * parent_task = (ZLOX_TASK *)current_task;

	// Clone the address space.
	ZLOX_PAGE_DIRECTORY * directory = zlox_clone_directory(current_directory,0);

	// Create a new process.
	ZLOX_TASK * new_task = (ZLOX_TASK *)zlox_kmalloc(sizeof(ZLOX_TASK));

	new_task->id = next_pid++;
	new_task->esp = new_task->ebp = 0;
	new_task->eip = 0;
	new_task->init_esp = current_task->init_esp;
	new_task->page_directory = directory;
	new_task->kernel_stack = zlox_kmalloc_a(ZLOX_KERNEL_STACK_SIZE);
	new_task->next = 0;

	// Add it to the end of the ready queue.
	ZLOX_TASK *tmp_task = (ZLOX_TASK *)ready_queue;
	while (tmp_task->next)
		tmp_task = tmp_task->next;
	tmp_task->next = new_task;

	// This will be the entry point for the new process.
	ZLOX_UINT32 eip = _zlox_read_eip();

	// We could be the parent or the child here - check.
	if (current_task == parent_task)
	{
		// We are the parent, so set up the esp/ebp/eip for our child.
		ZLOX_UINT32 esp; asm volatile("mov %%esp, %0" : "=r"(esp));
		ZLOX_UINT32 ebp; asm volatile("mov %%ebp, %0" : "=r"(ebp));
		ZLOX_UINT32 tmp_off;
		ZLOX_UINT32 tmp_esp;
		ZLOX_UINT32 i;
		// 为新进程拷贝内核栈数据
		zlox_memcpy((ZLOX_UINT8 *)new_task->kernel_stack,(ZLOX_UINT8 *)current_task->kernel_stack,ZLOX_KERNEL_STACK_SIZE);
		// 为新进程拷贝用户栈数据
		current_directory = new_task->page_directory;		
		for(tmp_esp = new_task->init_esp - 0x2000; tmp_esp < new_task->init_esp ;tmp_esp += 0x1000)
		{
			zlox_page_copy(tmp_esp);
		}
		current_directory = current_task->page_directory;

		tmp_off = esp - current_task->kernel_stack;
		new_task->esp = new_task->kernel_stack + tmp_off;
		tmp_off = ebp - current_task->kernel_stack;
		new_task->ebp = new_task->kernel_stack + tmp_off;
		if(new_task->kernel_stack > current_task->kernel_stack)
			tmp_off = new_task->kernel_stack - current_task->kernel_stack;
		else
			tmp_off = current_task->kernel_stack - new_task->kernel_stack;

		// Backtrace through the original stack, copying new values into
		// the new stack.  
		for(i = (ZLOX_UINT32)new_task->ebp; 
			(i >= new_task->esp && (i < (ZLOX_UINT32)(new_task->kernel_stack + ZLOX_KERNEL_STACK_SIZE))); )
		{
			ZLOX_UINT32 tmp = * (ZLOX_UINT32*)i;
			// it is a base pointer and remap it. 
			if (( esp < tmp) && (tmp < (current_task->kernel_stack + ZLOX_KERNEL_STACK_SIZE)))
			{
				if(new_task->kernel_stack > current_task->kernel_stack)
					tmp = tmp + tmp_off;
				else
					tmp = tmp - tmp_off;
				ZLOX_UINT32 *tmp2 = (ZLOX_UINT32 *)i;
				*tmp2 = tmp;
				i = tmp;
			}
			else
				break;
		}
		new_task->eip = eip;
		asm volatile("sti");

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
	zlox_set_kernel_stack(current_task->kernel_stack + ZLOX_KERNEL_STACK_SIZE);
	
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

