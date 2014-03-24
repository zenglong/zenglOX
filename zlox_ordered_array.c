/* zlox_ordered_array.c -- Implementation for creating, inserting and deleting
	from ordered arrays. */

#include "zlox_ordered_array.h"

// addr线性地址为堆的index指针索引数组的起始地址,该数组里存储了堆里所有可供分配的hole的头部的指针值,
//	max_size表示该数组最多可以容纳多少指针,less_than函数指针用于向该数组进行插入删除时进行排序用的,
//	因为该数组里的指针是根据hole的尺寸来排序的,尺寸小的hole的指针值排在前面,尺寸大的排在后面,这样在查找时,
//	就可以找到符合要求的尺寸最小的hole
ZLOX_ORDERED_ARRAY zlox_place_ordered_array(ZLOX_VOID *addr, ZLOX_UINT32 max_size, ZLOX_LESSTHEN_FUNC less_than)
{
	ZLOX_ORDERED_ARRAY to_ret;
	to_ret.array = (ZLOX_VPTR *)addr;
	zlox_memset((ZLOX_UINT8 *)to_ret.array, 0, max_size*sizeof(ZLOX_VPTR));
	to_ret.size = 0;
	to_ret.max_size = max_size;
	to_ret.less_than = less_than;
	return to_ret;
}

// 将某个hole的头部的指针值插入到array对应的指针索引数组里,
//	尺寸小的hole的指针值排在array对应的数组的前面,尺寸大的排在后面
ZLOX_VOID zlox_insert_ordered_array(ZLOX_VPTR item, ZLOX_ORDERED_ARRAY *array)
{
	ZLOX_ASSERT(array->less_than);
	ZLOX_UINT32 iterator = 0;
	while (iterator < array->size && array->less_than(array->array[iterator], item))
		iterator++;
	if (iterator == array->size) // just add at the end of the array.
		array->array[array->size++] = item;
	else
	{
		ZLOX_VPTR tmp = array->array[iterator];
		array->array[iterator] = item;
		while (iterator < array->size)
		{
			iterator++;
			ZLOX_VPTR tmp2 = array->array[iterator];
			array->array[iterator] = tmp;
			tmp = tmp2;
		}
		array->size++;
	}
}

// 返回某个索引对应的hole的指针值
ZLOX_VPTR zlox_lookup_ordered_array(ZLOX_UINT32 i, ZLOX_ORDERED_ARRAY *array)
{
	ZLOX_ASSERT(i < array->size);
	return array->array[i];
}

// 从array里删除某个hole的指针值
ZLOX_VOID zlox_remove_ordered_array(ZLOX_UINT32 i, ZLOX_ORDERED_ARRAY *array)
{
	while (i < array->size)
	{
		array->array[i] = array->array[i+1];
		i++;
	}
	array->size--;
}

