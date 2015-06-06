// ps.c -- 列举出任务信息和内存信息

#include "common.h"
#include "syscall.h"
#include "task.h"
#include "kheap.h"

int main(TASK * task, int argc, char * argv[])
{
	if(argc == 1 || (argc > 1 && strcmp(argv[1],"-d")==0) ||
		(argc > 1 && strcmp(argv[1],"-x")==0) || 
		(argc > 1 && strIsNum(argv[1])) )
	{
		TASK * first_task;
		TASK * tmp_task;
		UINT32 show_num = 0;
		UINT32 show_count = 0;
		tmp_task = task;
		while(tmp_task != 0)
		{
			first_task = tmp_task;
			tmp_task = first_task->prev;
		}
		tmp_task = first_task;
		if(argc == 2)
		{
			if(strIsNum(argv[1]))
				show_num = strToUInt(argv[1]);
		}
		else if(argc > 2)
		{
			if(strIsNum(argv[2]))
				show_num = strToUInt(argv[2]);
		}
		do{
			syscall_cmd_window_write("ID:");
			syscall_cmd_window_write_dec(tmp_task->id);
			syscall_cmd_window_write(" status:");
			switch(tmp_task->status)
			{
			case TS_WAIT:
				syscall_cmd_window_write("wait");
				break;
			case TS_ATA_WAIT:
				syscall_cmd_window_write("ata_wait");
				break;
			case TS_RUNNING:
				syscall_cmd_window_write("running");
				break;
			case TS_FINISH:
				syscall_cmd_window_write("finish");
				break;
			case TS_ZOMBIE:
				syscall_cmd_window_write("zombie");
				break;
			}
			syscall_cmd_window_write(" args:\"");
			if(tmp_task->args != 0)
				syscall_cmd_window_write(tmp_task->args);
			syscall_cmd_window_put('"');
			if(tmp_task->parent == 0)
				syscall_cmd_window_write(" system first task");
			else
			{
				syscall_cmd_window_write(" ParentID:");
				syscall_cmd_window_write_dec(tmp_task->parent->id);
			}
			
			if((argc > 1 && strcmp(argv[1],"-x")==0))
			{
				syscall_cmd_window_write(" msgnum:");
				syscall_cmd_window_write_dec(tmp_task->msglist.count);
				syscall_cmd_window_write("\n");
			}
			else if((argc > 1 && strcmp(argv[1],"-d")==0))
			{
				syscall_cmd_window_write("\n");
				ELF_LINK_MAP * tmp_map;
				ELF_LINK_MAP_LIST * maplst = &tmp_task->link_maps;
				for(SINT32 i = 0; i < maplst->count; i++)
				{
					tmp_map = maplst->ptr + i;
					if(i == 0)
						syscall_cmd_window_write("<< exec");
					else
					{
						syscall_cmd_window_write("<< dyn \"");
						syscall_cmd_window_write(tmp_map->soname);
						syscall_cmd_window_write("\"");
					}
					syscall_cmd_window_write(" entry:");
					syscall_cmd_window_write_hex(tmp_map->entry);
					syscall_cmd_window_write(" vaddr:");
					syscall_cmd_window_write_hex(tmp_map->vaddr);
					syscall_cmd_window_write(" msize:");
					syscall_cmd_window_write_dec(tmp_map->msize);
					syscall_cmd_window_write(" byte >>\n");
				}
				syscall_cmd_window_write("\n");
			}
			else
				syscall_cmd_window_write("\n");
			tmp_task = tmp_task->next;
			show_count++;
			if(show_num != 0 && show_count >= show_num)
				break;
		}while(tmp_task != 0);
	}
	else if(argc > 1 && strcmp(argv[1],"-u")==0)
	{
		TASK * first_task;
		TASK * tmp_task;
		UINT32 show_num = 0;
		UINT32 show_count = 0;
		tmp_task = task;
		while(tmp_task != 0)
		{
			first_task = tmp_task;
			tmp_task = first_task->prev;
		}
		tmp_task = first_task;
		if(argc == 2)
		{
			if(strIsNum(argv[1]))
				show_num = strToUInt(argv[1]);
		}
		else if(argc > 2)
		{
			if(strIsNum(argv[2]))
				show_num = strToUInt(argv[2]);
		}
		do{
			syscall_cmd_window_write("ID:");
			syscall_cmd_window_write_dec(tmp_task->id);
			syscall_cmd_window_write(" args:\"");
			if(tmp_task->args != 0)
				syscall_cmd_window_write(tmp_task->args);
			syscall_cmd_window_put('"');
			if(tmp_task->parent == 0)
				syscall_cmd_window_write(" system first task");
			else
			{
				syscall_cmd_window_write(" ParentID:");
				syscall_cmd_window_write_dec(tmp_task->parent->id);
			}
			syscall_cmd_window_write("\n");
			syscall_cmd_window_write("<< uheap start: ");
			syscall_cmd_window_write_hex(((HEAP *)(tmp_task->heap))->start_address);
			syscall_cmd_window_write(" end: ");
			syscall_cmd_window_write_hex(((HEAP *)(tmp_task->heap))->end_address);
			syscall_cmd_window_write(" size: ");
			syscall_cmd_window_write_dec(((HEAP *)(tmp_task->heap))->end_address - 
						((HEAP *)(tmp_task->heap))->start_address);
			syscall_cmd_window_write(" byte ");
			if(((HEAP *)(tmp_task->heap))->end_address - ((HEAP *)(tmp_task->heap))->start_address > 1024)
			{
				syscall_cmd_window_write("[");
				syscall_cmd_window_write_dec((((HEAP *)(tmp_task->heap))->end_address - 
							((HEAP *)(tmp_task->heap))->start_address) / 1024);
				syscall_cmd_window_write(" Kbyte]");
			}			
			syscall_cmd_window_write(" >>\n<< hole number: ");
			syscall_cmd_window_write_dec(((HEAP *)(tmp_task->heap))->index.size);
			syscall_cmd_window_write(" >>\n\n");
			tmp_task = tmp_task->next;
			show_count++;
			if(show_num != 0 && show_count >= show_num)
				break;
		}while(tmp_task != 0);
	}
	else if(argc > 1 && strcmp(argv[1],"-m")==0)
	{
		UINT32 * frames;
		UINT32 nframes;
		UINT32 i, j, count;
		UINT32 nframe_lines;
		syscall_get_frame_info((void **)&frames,&nframes);
		nframe_lines = nframes/(8*4);
		count = 0;
		for (i = 0; i < nframe_lines; i++)
		{
			if(frames[i] == 0xFFFFFFFF)
				count += 32;
			else if(frames[i] == 0)
				continue;
			else if (frames[i] != 0xFFFFFFFF) // nothing free, exit early.
			{
				// at least one bit is free here.
				for (j = 0; j < 32; j++)
				{
					UINT32 toTest = 0x1 << j;
					if (frames[i]&toTest)
					{
						count++;
					}
				}
			}
		}
		syscall_cmd_window_write("total memory: ");
		syscall_cmd_window_write_dec(nframes * 4);
		syscall_cmd_window_write(" Kbyte\n");
		syscall_cmd_window_write("usage memory: ");
		syscall_cmd_window_write_dec(count * 4);
		syscall_cmd_window_write(" Kbyte \n");
		syscall_cmd_window_write("free memory: ");
		syscall_cmd_window_write_dec((nframes - count) * 4);
		syscall_cmd_window_write(" Kbyte \n");
		
		HEAP * kheap = (HEAP *)syscall_get_kheap();
		syscall_cmd_window_write("kheap start: ");
		syscall_cmd_window_write_hex(kheap->start_address);
		syscall_cmd_window_write(" end: ");
		syscall_cmd_window_write_hex(kheap->end_address);
		syscall_cmd_window_write(" size: ");
		syscall_cmd_window_write_dec(kheap->end_address - kheap->start_address);
		syscall_cmd_window_write(" byte ");
		if(kheap->end_address - kheap->start_address > 1024)
		{
			syscall_cmd_window_write("[");
			syscall_cmd_window_write_dec((kheap->end_address - kheap->start_address) / 1024);
			syscall_cmd_window_write(" Kbyte]");
		}
		syscall_cmd_window_put('\n');
		syscall_cmd_window_write("kheap hole number: ");
		syscall_cmd_window_write_dec(kheap->index.size);
		syscall_cmd_window_put('\n');
		syscall_cmd_window_write("kheap block number: ");
		syscall_cmd_window_write_dec(kheap->blk_index.size);
		syscall_cmd_window_put('\n');
	}
	else
	{
		syscall_cmd_window_write("usage: ps [shownum][-m][-d [shownum]][-u [shownum]][-x [shownum]]");
	}
	return 0;
}

