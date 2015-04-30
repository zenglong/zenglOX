/* zlox_kheap.c Kernel heap functions, also provides
	a placement malloc() for use before the heap is 
	initialised. */

#include "zlox_kheap.h"
#include "zlox_paging.h"

// end is defined in the linker script.
extern ZLOX_UINT32 _end;
extern ZLOX_PAGE_DIRECTORY * kernel_directory;
ZLOX_UINT32 placement_address = (ZLOX_UINT32)&_end;
ZLOX_HEAP * kheap = 0;

ZLOX_BOOL kheap_debug = ZLOX_TRUE; // for debug

ZLOX_UINT32 zlox_kmalloc_int(ZLOX_UINT32 sz, ZLOX_SINT32 align, ZLOX_UINT32 *phys)
{
	ZLOX_UINT32 tmp;
	// 是否初始化了堆,如果初始化了,则使用堆来分配内存
	if (kheap != 0)
	{
		ZLOX_VOID *addr = zlox_alloc(sz, (ZLOX_UINT8)align, kheap);
		if (phys != 0)
		{
			// 从分页表里获取addr线性地址对应的实际的物理内存地址
			ZLOX_PAGE *page = zlox_get_page((ZLOX_UINT32)addr, 0, kernel_directory);
			*phys = (page->frame * 0x1000) + ((ZLOX_UINT32)addr & 0xFFF);
		}
		// 返回线性地址
		return (ZLOX_UINT32)addr;
	}
	else
	{
		// This will eventually call malloc() on the kernel heap.
		// For now, though, we just assign memory at placement_address
		// and increment it by sz. Even when we've coded our kernel
		// heap, this will be useful for use before the heap is initialised.
		if (align == 1 && (placement_address & 0x00000FFF) )
		{
			// Align the placement address;
			placement_address &= 0xFFFFF000;
			placement_address += 0x1000;
		}
		if (phys)
		{
			*phys = placement_address;
		}
		tmp = placement_address;
		placement_address += sz;
		return tmp;
	}
}

ZLOX_UINT32 zlox_kmalloc_128k_align(ZLOX_UINT32 sz, ZLOX_UINT32 *phys)
{
	ZLOX_UINT32 tmp;
	if(kheap == 0)
	{
		if((placement_address & 0x0001FFFF))
		{
			// Align the placement address;
			placement_address &= 0xFFFE0000;
			placement_address += 0x20000;
		}
		if (phys)
		{
			*phys = placement_address;
		}
		tmp = placement_address;
		placement_address += sz;
		return tmp;
	}
	return 0;
}

ZLOX_VOID zlox_kfree(ZLOX_VOID *p)
{
	zlox_free(p, kheap);
}

ZLOX_UINT32 zlox_kmalloc_a(ZLOX_UINT32 sz)
{
	return zlox_kmalloc_int(sz, 1, 0);
}

ZLOX_UINT32 zlox_kmalloc_p(ZLOX_UINT32 sz, ZLOX_UINT32 *phys)
{
	return zlox_kmalloc_int(sz, 0, phys);
}

ZLOX_UINT32 zlox_kmalloc_ap(ZLOX_UINT32 sz, ZLOX_UINT32 *phys)
{
	return zlox_kmalloc_int(sz, 1, phys);
}

ZLOX_UINT32 zlox_kmalloc(ZLOX_UINT32 sz)
{
	return zlox_kmalloc_int(sz, 0, 0);
}

// 当堆空间不足时,扩展堆的物理内存 
static ZLOX_VOID zlox_expand(ZLOX_UINT32 new_size, ZLOX_HEAP * heap)
{
	// Sanity check.
	ZLOX_ASSERT(new_size > heap->end_address - heap->start_address);

	// Get the nearest following page boundary.
	// 扩展的尺寸必须是页对齐的
	if ((new_size & 0x00000FFF) != 0)
	{
		new_size &= 0xFFFFF000;
		new_size += 0x1000;
	}

	// Make sure we are not overreaching ourselves.
	ZLOX_ASSERT(heap->start_address + new_size <= heap->max_address);

	// This should always be on a page boundary.
	ZLOX_UINT32 old_size = heap->end_address-heap->start_address;

	ZLOX_UINT32 i = old_size;
	ZLOX_BOOL need_flushTLB = ZLOX_FALSE;
	while (i < new_size)
	{
		// 从frames位图里为扩展的线性地址分配物理内存
		zlox_alloc_frame( zlox_get_page(heap->start_address+i, 1, kernel_directory),
					 (heap->supervisor)?1:0, (heap->readonly)?0:1);
		i += 0x1000 /* page size */;
		need_flushTLB = ZLOX_TRUE;
	}
	if(need_flushTLB)
		zlox_page_Flush_TLB();
	heap->end_address = heap->start_address+new_size;
}

// 根据需要回收一部分堆的物理内存,将回收的部分从页表和frames位图里去除
static ZLOX_UINT32 zlox_contract(ZLOX_UINT32 new_size, ZLOX_HEAP * heap)
{
	// Sanity check.
	ZLOX_ASSERT(new_size < heap->end_address-heap->start_address);

	ZLOX_UINT32 new_size_orig = new_size;

	// Get the nearest following page boundary.
	if (new_size & 0x00000FFF)
	{
		new_size &= 0xFFFFF000;
		new_size += 0x1000;
	}

	// Don't contract too far!
	// 回收的尺寸受到ZLOX_HEAP_MIN_SIZE的限制,防止回收过多的内存资源
	if (new_size < ZLOX_HEAP_MIN_SIZE)
	{
		if(ZLOX_HEAP_MIN_SIZE - new_size > 
			sizeof(ZLOX_KHP_HEADER) + sizeof(ZLOX_KHP_FOOTER))
			new_size = ZLOX_HEAP_MIN_SIZE;
		// 如果收缩堆空间后,剩余尺寸不能形成一个有效的hole,则放弃收缩,返回原来的尺寸
		else
			return (heap->end_address-heap->start_address);
	}

	ZLOX_UINT32 diff = new_size - new_size_orig;

	if(diff == 0)
		;
	else if(diff > 0)
	{
		ZLOX_KHP_HEADER * header = (ZLOX_KHP_HEADER *)(heap->start_address + new_size_orig);
		if(diff <= (sizeof(ZLOX_KHP_HEADER) + sizeof(ZLOX_KHP_FOOTER)))
		{
			new_size += 0x1000;
		}
		diff = new_size - new_size_orig;
		if(diff >= header->size)
		{
			return (heap->end_address-heap->start_address);
		}
	}

	ZLOX_UINT32 old_size = heap->end_address-heap->start_address;
	ZLOX_UINT32 i = old_size - 0x1000;
	ZLOX_BOOL need_flushTLB = ZLOX_FALSE;
	while (new_size <= i)
	{
		// 通过zlox_free_frame将需要回收的页面从页表和frames位图里去除
		zlox_free_frame(zlox_get_page(heap->start_address+i, 0, kernel_directory));
		i -= 0x1000;
		need_flushTLB = ZLOX_TRUE;
	}
	if(need_flushTLB)
		zlox_page_Flush_TLB();
	heap->end_address = heap->start_address + new_size;
	return new_size;
}

// 从heap的index里搜索出满足size尺寸的最小的hole
static ZLOX_SINT32 zlox_find_smallest_hole(ZLOX_UINT32 size, ZLOX_UINT8 page_align, ZLOX_HEAP * heap)
{
	// Find the smallest hole that will fit.
	ZLOX_UINT32 iterator = 0;
	while (iterator < heap->index.size)
	{
		ZLOX_KHP_HEADER *header = (ZLOX_KHP_HEADER *)zlox_lookup_ordered_array(iterator, &heap->index);
		// If the user has requested the memory be page-aligned
		if (page_align > 0)
		{
			// Page-align the starting point of this header.
			ZLOX_UINT32 location = (ZLOX_UINT32)header;
			ZLOX_SINT32 offset = 0;
			ZLOX_SINT32 base_hole_size = (ZLOX_SINT32)(sizeof(ZLOX_KHP_HEADER) + sizeof(ZLOX_KHP_FOOTER));
			if ( ((location + sizeof(ZLOX_KHP_HEADER)) & 0x00000FFF) != 0)
			{
				offset = 0x1000 /* page size */  - (location+sizeof(ZLOX_KHP_HEADER)) % 0x1000;
				if(offset <= base_hole_size)
					offset += 0x1000;
			}
			ZLOX_SINT32 hole_size = (ZLOX_SINT32)header->size - offset;
			// Can we fit now?
			// 去除对齐产生的offset偏移值后,如果剩余的hole尺寸大于等于所需的size,
			// 且offset因页对齐被剥离出去后可以产生一个新的hole的话,就说明当前找到的hole符合要求
			if ( (hole_size >= (ZLOX_SINT32)size) )
				break;
		}
		else if (header->size >= size)
			break;
		iterator++;
	}
	// Why did the loop exit?
	if (iterator == heap->index.size)
		return -1; // We got to the end and didn't find anything.
	else
		return iterator;
}

// 用于对两个hole或block进行size(尺寸)比较的函数
static ZLOX_SINT8 zlox_header_less_than(ZLOX_VOID *a, ZLOX_VOID *b)
{
	return (((ZLOX_KHP_HEADER *)a)->size < ((ZLOX_KHP_HEADER *)b)->size)?1:0;
}

// 创建堆的函数,start为堆的起始线性地址,end_addr为堆的当前结束地址,max为堆可以expand(扩展)的最大尺寸,
//	supervisor表示该堆是否只运行内核代码访问(0表示用户程序可以访问,1表示用户程序不可访问),
//	readonly表示该堆是否是只读的(0表示可读写,1表示只读),在expand扩展堆空间时,supervisor和readonly
//	可以用来设置页表里的属性
// 另外,start开始的第一个部分是heap堆的index部分,index部分是一个数组,该数组存储了该堆里所有可供分配的hole的指针值,
//	并且index里的hole的指针是根据hole的尺寸由小到大排列的,尺寸小的hole的指针值排在前面,较大的排在后面
ZLOX_HEAP * zlox_create_heap(ZLOX_UINT32 start, ZLOX_UINT32 end_addr, 
							ZLOX_UINT32 max, ZLOX_UINT8 supervisor, ZLOX_UINT8 readonly)
{
	ZLOX_HEAP *heap = (ZLOX_HEAP *)zlox_kmalloc(sizeof(ZLOX_HEAP));

	// All our assumptions are made on startAddress and endAddress being page-aligned.
	ZLOX_ASSERT(start%0x1000 == 0);
	ZLOX_ASSERT(end_addr%0x1000 == 0);
	
	// Initialise the index.
	// 创建堆的index部分,index的含义在函数开头的注释里做了说明
	heap->index = zlox_place_ordered_array((ZLOX_VOID *)start, ZLOX_HEAP_INDEX_SIZE, &zlox_header_less_than);
	
	// Shift the start address forward to resemble where we can start putting data.
	// index后面就是实际的未分配的hole和已分配的block部分
	start += sizeof(ZLOX_VPTR) * ZLOX_HEAP_INDEX_SIZE;

	// Make sure the start address is page-aligned.
	if ((start & 0x00000FFF) != 0)
	{
		start &= 0xFFFFF000;
		start += 0x1000;
	}

	// for debug start
	if(kheap_debug)
	{
		// Initialise the index.
		// 创建堆的index部分,index的含义在函数开头的注释里做了说明
		heap->blk_index = zlox_place_ordered_array((ZLOX_VOID *)start, ZLOX_HEAP_INDEX_SIZE, &zlox_header_less_than);
	
		// Shift the start address forward to resemble where we can start putting data.
		// index后面就是实际的未分配的hole和已分配的block部分
		start += sizeof(ZLOX_VPTR) * ZLOX_HEAP_INDEX_SIZE;

		// Make sure the start address is page-aligned.
		if ((start & 0x00000FFF) != 0)
		{
			start &= 0xFFFFF000;
			start += 0x1000;
		}
	}
	// for debug end

	// Write the start, end and max addresses into the heap structure.
	heap->start_address = start;
	heap->end_address = end_addr;
	heap->max_address = max;
	heap->supervisor = supervisor;
	heap->readonly = readonly;

	// We start off with one large hole in the index.
	// 刚开始创建的堆,除了堆头部的index指针数组外,其余部分是一个大的hole,这样zlox_kmalloc函数就可以从该hole里分配到内存
	ZLOX_KHP_HEADER *hole = (ZLOX_KHP_HEADER *)start;
	hole->size = end_addr-start;
	hole->magic = ZLOX_HEAP_MAGIC;
	hole->is_hole = 1;
	ZLOX_KHP_FOOTER *hole_footer = (ZLOX_KHP_FOOTER *)((ZLOX_UINT32)hole + hole->size - sizeof(ZLOX_KHP_FOOTER));
	hole_footer->magic = ZLOX_HEAP_MAGIC;
	hole_footer->header = hole;
	zlox_insert_ordered_array((ZLOX_VPTR)hole, &heap->index);	 

	return heap;
}

// 从heap堆里查找能够容纳size尺寸的hole,如果找到该hole,则将该hole变为block,如果hole分离出block后,
//	还有多余的空间,则多余的部分形成新的hole,如果找不到满足size要求的hole,则对heap进行expand(扩展物理内存)
ZLOX_VOID * zlox_alloc(ZLOX_UINT32 size, ZLOX_UINT8 page_align, ZLOX_HEAP * heap)
{
	//	Make sure we take the size of header/footer into account.
	//	计算new_size时,除了要考虑size所需的内存尺寸外,还必须把头部,底部结构的尺寸也考虑进去
	ZLOX_UINT32 new_size = size + sizeof(ZLOX_KHP_HEADER) + sizeof(ZLOX_KHP_FOOTER);
	// Find the smallest hole that will fit.
	ZLOX_SINT32 iterator = zlox_find_smallest_hole(new_size, page_align, heap);
	
	if (iterator == -1) // If we didn't find a suitable hole
	{
		// Save some previous data.
		ZLOX_UINT32 old_length = heap->end_address - heap->start_address;
		ZLOX_UINT32 old_end_address = heap->end_address;

		// We need to allocate some more space.
		zlox_expand(old_length+new_size, heap);
		ZLOX_UINT32 new_length = heap->end_address - heap->start_address;
		ZLOX_KHP_FOOTER * old_end_footer = (ZLOX_KHP_FOOTER *)(old_end_address - sizeof(ZLOX_KHP_FOOTER));
		ZLOX_ASSERT(old_end_footer->magic == ZLOX_HEAP_MAGIC);
		// 在zlox_expand扩容了heap的堆空间后,新增空间的前面如果本身是一个hole的话,就将新增的空间合并到前面的hole里
		if(old_end_footer->header->magic == ZLOX_HEAP_MAGIC &&
			old_end_footer->header->is_hole == 1)
		{
			// The last header needs adjusting.
			ZLOX_KHP_HEADER *header = old_end_footer->header;
			header->size += new_length - old_length;
			// Rewrite the footer.
			ZLOX_KHP_FOOTER *footer = (ZLOX_KHP_FOOTER *) ( (ZLOX_UINT32)header + header->size - sizeof(ZLOX_KHP_FOOTER) );
			footer->header = header;
			footer->magic = ZLOX_HEAP_MAGIC;
		}
		// 如果前面不是一个hole,则将新增的空间变为一个单独的hole
		else
		{
			ZLOX_KHP_HEADER *header = (ZLOX_KHP_HEADER *)old_end_address;
			header->magic = ZLOX_HEAP_MAGIC;
			header->size = new_length - old_length;
			header->is_hole = 1;
			ZLOX_KHP_FOOTER *footer = (ZLOX_KHP_FOOTER *) (old_end_address + header->size - sizeof(ZLOX_KHP_FOOTER));
			footer->magic = ZLOX_HEAP_MAGIC;
			footer->header = header;
			zlox_insert_ordered_array((ZLOX_VPTR)header, &heap->index);
		}
		// We now have enough space. Recurse, and call the function again.
		return zlox_alloc(size, page_align, heap);
	}

	ZLOX_KHP_HEADER *orig_hole_header = (ZLOX_KHP_HEADER *)zlox_lookup_ordered_array(iterator, &heap->index);
	ZLOX_UINT32 orig_hole_pos = (ZLOX_UINT32)orig_hole_header;
	ZLOX_UINT32 orig_hole_size = orig_hole_header->size;
	// Here we work out if we should split the hole we found into two parts.
	// Is the original hole size - requested hole size less than the overhead for adding a new hole?
	// 判断找到的hole尺寸除了可以满足基本的new_size需求外,是否可以再分出一个hole,这里只有差值大于头与底部的大小,
	//	才能形成一个新的hole,所以这里差值做了小于等于的判断,当小于等于时,说明不能形成一个新的hole,
	//	则将hole整个分配出去
	if (orig_hole_size-new_size <= sizeof(ZLOX_KHP_HEADER)+sizeof(ZLOX_KHP_FOOTER))
	{
		// Then just increase the requested size to the size of the hole we found.
		// 如果hole满足new_size后,不能再分出一个hole了,则将整个hole都分配出去
		size += orig_hole_size-new_size;
		new_size = orig_hole_size;
	}

	// If we need to page-align the data, do it now and make a new hole in front of our block.
	// 如果需要对齐,而hole的头部下面的指针没有页对齐,则进行页对齐操作,
	//	且将页对齐后剩余出来的offset偏移值作为新hole的大小
	if (page_align && 
		((orig_hole_pos + sizeof(ZLOX_KHP_HEADER)) & 0x00000FFF)
		)
	{
		ZLOX_SINT32 offset = 0x1000 /* page size */ - (orig_hole_pos + sizeof(ZLOX_KHP_HEADER)) % 0x1000;
		if(offset <= (ZLOX_SINT32)(sizeof(ZLOX_KHP_HEADER)+sizeof(ZLOX_KHP_FOOTER)))
			offset += 0x1000;
		ZLOX_UINT32 new_location   = orig_hole_pos + offset;
		ZLOX_KHP_HEADER *hole_header = (ZLOX_KHP_HEADER *)orig_hole_pos;
		hole_header->size	= offset;
		hole_header->magic	= ZLOX_HEAP_MAGIC;
		hole_header->is_hole = 1;
		ZLOX_KHP_FOOTER *hole_footer = (ZLOX_KHP_FOOTER *) ( (ZLOX_UINT32)new_location - sizeof(ZLOX_KHP_FOOTER) );
		hole_footer->magic	= ZLOX_HEAP_MAGIC;
		hole_footer->header	= hole_header;
		orig_hole_pos		= new_location;
		orig_hole_size		= orig_hole_size - hole_header->size;
	}
	else
	{
		// Else we don't need this hole any more, delete it from the index.
		// hole变为block后,需要将hole从index指针数组里移除
		zlox_remove_ordered_array(iterator, &heap->index);
	}

	// Overwrite the original header...
	ZLOX_KHP_HEADER *block_header  = (ZLOX_KHP_HEADER *)orig_hole_pos;
	block_header->magic		= ZLOX_HEAP_MAGIC;
	block_header->is_hole	= 0;
	// 如果找到的hole减去所需的new_size后,剩余尺寸可以容纳一个有效的hole的话,就将剩余的部分变为一个新的hole
	if (orig_hole_size - new_size > sizeof(ZLOX_KHP_HEADER)+sizeof(ZLOX_KHP_FOOTER))
	{
		block_header->size = new_size;
		// ...And the footer
		ZLOX_KHP_FOOTER *block_footer = (ZLOX_KHP_FOOTER *) (orig_hole_pos + sizeof(ZLOX_KHP_HEADER) + size);
		block_footer->magic = ZLOX_HEAP_MAGIC;
		block_footer->header = block_header;
		ZLOX_KHP_HEADER *hole_header = (ZLOX_KHP_HEADER *) (orig_hole_pos + sizeof(ZLOX_KHP_HEADER) + size + sizeof(ZLOX_KHP_FOOTER));
		hole_header->magic = ZLOX_HEAP_MAGIC;
		hole_header->is_hole = 1;
		hole_header->size = orig_hole_size - new_size;
		ZLOX_KHP_FOOTER *hole_footer = (ZLOX_KHP_FOOTER *) ( (ZLOX_UINT32)hole_header + hole_header->size - sizeof(ZLOX_KHP_FOOTER) );
		hole_footer->magic = ZLOX_HEAP_MAGIC;
		hole_footer->header = hole_header;
		// Put the new hole in the index;
		zlox_insert_ordered_array((ZLOX_VPTR)hole_header, &heap->index);
	}
	// 剩余部分不足以形成新的hole,则将找到的hole整个分配出去
	else
	{
		block_header->size	  = orig_hole_size;
		ZLOX_KHP_FOOTER *block_footer  = (ZLOX_KHP_FOOTER *)(orig_hole_pos + block_header->size - sizeof(ZLOX_KHP_FOOTER));
		block_footer->magic	 = ZLOX_HEAP_MAGIC;
		block_footer->header	= block_header;
	}

	// for debug begin
	if(kheap_debug)
	{
		ZLOX_ASSERT(heap->blk_index.size < heap->blk_index.max_size);
		heap->blk_index.array[heap->blk_index.size++] = (ZLOX_VOID *)((ZLOX_UINT32)block_header+sizeof(ZLOX_KHP_HEADER));
	}
	// for debug end

	zlox_kheap_check_all_blk(); // for debug

	// ...And we're done!
	return (ZLOX_VOID *)((ZLOX_UINT32)block_header+sizeof(ZLOX_KHP_HEADER));
}

// 将p指针所在的block(已分配的堆内存)变为hole(未分配的堆内存),同时在变为hole之后,
//	如果该hole的左右相邻位置存在hole的话,则将这些hole合并为一个大的hole,
//	另外,如果free释放生成的hole刚好又位于堆的底部时,则将堆空间进行contract收缩操作
ZLOX_VOID zlox_free(ZLOX_VOID *p, ZLOX_HEAP *heap)
{
	// Exit gracefully for null pointers.
	if (p == 0)
		return;

	// Get the header and footer associated with this pointer.
	ZLOX_KHP_HEADER *header = (ZLOX_KHP_HEADER *)((ZLOX_UINT32)p - sizeof(ZLOX_KHP_HEADER));
	ZLOX_KHP_FOOTER *footer = (ZLOX_KHP_FOOTER *)((ZLOX_UINT32)header + header->size - sizeof(ZLOX_KHP_FOOTER));

	// Sanity checks.
	ZLOX_ASSERT(header->magic == ZLOX_HEAP_MAGIC);
	ZLOX_ASSERT(footer->magic == ZLOX_HEAP_MAGIC);

	// 如果本身就是一个hole,则直接返回
	if(header->is_hole == 1)
		return;

	// Make us a hole.
	header->is_hole = 1;

	// Do we want to add this header into the 'free holes' index?
	ZLOX_CHAR do_add = 1;

	// Unify left
	// If the thing immediately to the left of us is a footer...
	// 合并左侧低地址方向的hole,如果左侧刚好有一个hole,则将当前释放的hole直接合并到左侧的hole里
	ZLOX_KHP_FOOTER *test_footer = (ZLOX_KHP_FOOTER *)((ZLOX_UINT32)header - sizeof(ZLOX_KHP_FOOTER));
	if((ZLOX_UINT32)test_footer > heap->start_address)
	{
		if (test_footer->magic == ZLOX_HEAP_MAGIC &&
			test_footer->header->is_hole == 1)
		{
			ZLOX_UINT32 cache_size = header->size; // Cache our current size.
			header = test_footer->header; // Rewrite our header with the new one.
			footer->header = header; // Rewrite our footer to point to the new header.
			header->size += cache_size; // Change the size.
			do_add = 0; // Since this header is already in the index, we don't want to add it again.
		}
	}

	// Unify right
	// If the thing immediately to the right of us is a header...
	// 合并右侧高地址方向的hole,如果右侧刚好有一个hole,则将右侧的hole合并到当前释放的hole里
	ZLOX_KHP_HEADER *test_header = (ZLOX_KHP_HEADER*)((ZLOX_UINT32)footer + sizeof(ZLOX_KHP_FOOTER));
	if((ZLOX_UINT32)test_header < heap->end_address)
	{
		if (test_header->magic == ZLOX_HEAP_MAGIC &&
			test_header->is_hole == 1)
		{
			header->size += test_header->size; // Increase our size.
			test_footer = (ZLOX_KHP_FOOTER *) ( (ZLOX_UINT32)test_header + // Rewrite it's footer to point to our header.
										test_header->size - sizeof(ZLOX_KHP_FOOTER) );
			footer = test_footer;
			footer->header = header;
			// Find and remove this header from the index.
			ZLOX_UINT32 iterator = 0;
			while ((iterator < heap->index.size) &&
				   (zlox_lookup_ordered_array(iterator, &heap->index) != (ZLOX_VPTR)test_header))
				iterator++;

			// Make sure we actually found the item.
			ZLOX_ASSERT(iterator < heap->index.size);
			// Remove it.
			zlox_remove_ordered_array(iterator, &heap->index);
		}
	}

	// If the footer location is the end address, we can contract.
	// 如果hole位于堆的底部,则对堆进行contract收缩操作,以回收一部分物理内存资源
	if ( (ZLOX_UINT32)footer+sizeof(ZLOX_KHP_FOOTER) == heap->end_address)
	{
		ZLOX_UINT32 old_length = heap->end_address-heap->start_address;
		ZLOX_UINT32 new_length = zlox_contract( (ZLOX_UINT32)header - heap->start_address, heap);
		// Check how big we will be after resizing.
		if (header->size - (old_length-new_length) > 0)
		{
			// We will still exist, so resize us.
			header->size -= old_length-new_length;
			footer = (ZLOX_KHP_FOOTER *)((ZLOX_UINT32)header + header->size - sizeof(ZLOX_KHP_FOOTER));
			footer->magic = ZLOX_HEAP_MAGIC;
			footer->header = header;
		}
		else
		{
			// We will no longer exist :(. Remove us from the index.
			ZLOX_UINT32 iterator = 0;
			while ( (iterator < heap->index.size) &&
					(zlox_lookup_ordered_array(iterator, &heap->index) != (ZLOX_VPTR)header))
				iterator++;
			// If we didn't find ourselves, we have nothing to remove.
			if (iterator < heap->index.size)
				zlox_remove_ordered_array(iterator, &heap->index);
			do_add = 0;
		}
	}

	// If required, add us to the index.
	// 由于生成了新的hole,所以根据需要将该hole的头部的指针值设置到index对应的数组里
	if (do_add == 1)
		zlox_insert_ordered_array((ZLOX_VPTR)header, &heap->index);

	// for debug begin
	if(kheap_debug)
	{
		ZLOX_UINT32 iterator = 0;
		while ((iterator < heap->blk_index.size) &&
					(zlox_lookup_ordered_array(iterator, &heap->blk_index) != (ZLOX_VPTR)p))
				iterator++;
		ZLOX_ASSERT(iterator < heap->blk_index.size);
		if (iterator < heap->blk_index.size)
			zlox_remove_ordered_array(iterator, &heap->blk_index);
	}
	// for debug end
}

ZLOX_SINT32 zlox_kheap_check_all_blk()
{
	ZLOX_HEAP * heap = kheap;
	// for debug begin
	if(kheap_debug)
	{
		ZLOX_UINT32 iterator = 0;
		while(iterator < heap->blk_index.size)
		{
			ZLOX_VPTR p = zlox_lookup_ordered_array(iterator, &heap->blk_index);
			// Get the header and footer associated with this pointer.
			ZLOX_KHP_HEADER *header = (ZLOX_KHP_HEADER *)((ZLOX_UINT32)p - sizeof(ZLOX_KHP_HEADER));
			ZLOX_KHP_FOOTER *footer = (ZLOX_KHP_FOOTER *)((ZLOX_UINT32)header + header->size - sizeof(ZLOX_KHP_FOOTER));

			// Sanity checks.
			ZLOX_ASSERT(header->magic == ZLOX_HEAP_MAGIC);
			ZLOX_ASSERT(footer->magic == ZLOX_HEAP_MAGIC);
			iterator++;
		}

		// check all holes
		iterator = 0;
		while(iterator < heap->index.size)
		{
			ZLOX_KHP_HEADER *header = (ZLOX_KHP_HEADER *)zlox_lookup_ordered_array(iterator, &heap->index);
			ZLOX_KHP_FOOTER *footer = (ZLOX_KHP_FOOTER *)((ZLOX_UINT32)header + header->size - sizeof(ZLOX_KHP_FOOTER));

			// Sanity checks.
			ZLOX_ASSERT(header->magic == ZLOX_HEAP_MAGIC);
			ZLOX_ASSERT(footer->magic == ZLOX_HEAP_MAGIC);
			iterator++;
		}
	}
	return 0;
	// for debug end
}

// 获取kheap，主要用于系统调用
ZLOX_UINT32 zlox_get_kheap()
{
	return (ZLOX_UINT32)kheap;
}

