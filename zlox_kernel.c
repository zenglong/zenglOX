/*zlox_kernel.c Defines the C-code kernel entry point, calls initialisation routines.*/

#include "zlox_monitor.h"
#include "zlox_descriptor_tables.h"
#include "zlox_time.h"
#include "zlox_paging.h"
#include "zlox_kheap.h"

//zenglOX kernel main entry
ZLOX_SINT32 zlox_kernel_main(ZLOX_VOID * mboot_ptr)
{
	// init gdt and idt
	zlox_init_descriptor_tables();	

	// Initialise the screen (by clearing it)
	zlox_monitor_clear();

	// 未初始化堆时,为a分配内存
	ZLOX_UINT32 a = zlox_kmalloc(8);

	zlox_init_paging();

	// Write out a sample string
	zlox_monitor_write("hello world!\nwelcome to zenglOX v0.0.6!\n");

	// 在zlox_init_paging函数里初始化堆后,为b,c,d分配和释放内存
	ZLOX_UINT32 b = zlox_kmalloc(8);
	ZLOX_UINT32 c = zlox_kmalloc(8);
	zlox_monitor_write("a: ");
	zlox_monitor_write_hex(a);
	zlox_monitor_write(", b: ");
	zlox_monitor_write_hex(b);
	zlox_monitor_write("\nc: ");
	zlox_monitor_write_hex(c);

	zlox_kfree((ZLOX_VOID *)c);
	zlox_kfree((ZLOX_VOID *)b);
	ZLOX_UINT32 d = zlox_kmalloc(12);
	zlox_monitor_write(", d: ");
	zlox_monitor_write_hex(d);

	/*asm volatile("int $0x3");
	asm volatile("int $0x4");
	
	asm volatile("sti");
	zlox_init_timer(50);*/

	return (ZLOX_SINT32)mboot_ptr;
}

