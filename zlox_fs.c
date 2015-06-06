// zlox_fs.c -- Defines the interface and structures relating to the virtual file system.

#include "zlox_fs.h"
#include "zlox_isr.h"
#include "zlox_task.h"

// zlox_task.c
extern ZLOX_TASK * current_task;

ZLOX_FS_NODE *fs_root = 0; // The root of the filesystem.

ZLOX_BOOL g_fs_lock = ZLOX_FALSE;

ZLOX_VOID zlox_fs_lock(ZLOX_TASK * task)
{
	while(g_fs_lock == ZLOX_TRUE)
	{
		zlox_isr_detect_proc_irq();
		asm("pause");
	}

	g_fs_lock = ZLOX_TRUE;
	if((task != ZLOX_NULL) && 
	   (task->fs_lock == ZLOX_FALSE))
	{
		task->fs_lock = ZLOX_TRUE;
	}
	return;
}

ZLOX_VOID zlox_fs_unlock(ZLOX_TASK * task)
{
	if(g_fs_lock == ZLOX_TRUE)
	{
		g_fs_lock = ZLOX_FALSE;
	}

	if((task != ZLOX_NULL) && 
	   (task->fs_lock == ZLOX_TRUE))
	{
		task->fs_lock = ZLOX_FALSE;
	}
	return;
}

ZLOX_UINT32 zlox_read_fs(ZLOX_FS_NODE *node, ZLOX_UINT32 offset, ZLOX_UINT32 size, ZLOX_UINT8 *buffer)
{
	zlox_fs_lock(current_task);
	if(node == ZLOX_NULL)
	{
		zlox_fs_unlock(current_task);
		return 0;
	}
	// Has the node got a read callback?
	if (node->read != 0)
	{
		ZLOX_UINT32 ret = node->read(node, offset, size, buffer);
		zlox_fs_unlock(current_task);
		return ret;
	}
	else
	{
		zlox_fs_unlock(current_task);
		return 0;
	}
}

ZLOX_UINT32 zlox_write_fs(ZLOX_FS_NODE *node, ZLOX_UINT32 offset, ZLOX_UINT32 size, ZLOX_UINT8 *buffer)
{
	zlox_fs_lock(current_task);
	if(node == ZLOX_NULL)
	{
		zlox_fs_unlock(current_task);
		return 0;
	}
	// Has the node got a read callback?
	if (node->write != 0)
	{
		ZLOX_UINT32 ret = node->write(node, offset, size, buffer);
		zlox_fs_unlock(current_task);
		return ret;
	}
	else
	{
		zlox_fs_unlock(current_task);
		return 0;
	}
}

ZLOX_FS_NODE * zlox_writedir_fs(ZLOX_FS_NODE *node, ZLOX_CHAR *name, ZLOX_UINT16 type)
{
	zlox_fs_lock(current_task);
	if(node == ZLOX_NULL)
	{
		zlox_fs_unlock(current_task);
		return ZLOX_NULL;
	}
	if(node->writedir != 0)
	{
		ZLOX_FS_NODE * ret = node->writedir(node, name, type);
		zlox_fs_unlock(current_task);
		return ret;
	}
	else
	{
		zlox_fs_unlock(current_task);
		return ZLOX_NULL;
	}
}

ZLOX_SINT32 zlox_writedir_fs_safe(ZLOX_FS_NODE * node, ZLOX_CHAR *name, ZLOX_UINT16 type, ZLOX_FS_NODE * output)
{
	zlox_fs_lock(current_task);
	if(node == ZLOX_NULL)
	{
		zlox_fs_unlock(current_task);
		return -1;
	}
	if(node->writedir != 0)
	{
		ZLOX_FS_NODE * ret = node->writedir(node, name, type);
		if(ret == ZLOX_NULL)
		{
			zlox_fs_unlock(current_task);
			return -1;
		}
		if(output == ZLOX_NULL)
		{
			zlox_fs_unlock(current_task);
			return -1;
		}
		zlox_memcpy((ZLOX_UINT8 *)output, (ZLOX_UINT8 *)ret, sizeof(ZLOX_FS_NODE));
		zlox_fs_unlock(current_task);
		return 0;
	}
	else
	{
		zlox_fs_unlock(current_task);
		return -1;
	}
}

ZLOX_DIRENT * zlox_readdir_fs(ZLOX_FS_NODE *node, ZLOX_UINT32 index)
{
	zlox_fs_lock(current_task);
	if (node == ZLOX_NULL)
	{
		zlox_fs_unlock(current_task);
		return ZLOX_NULL;
	}
	// Is the node a directory, and does it have a callback?
	if ( (node->flags & 0x7) == ZLOX_FS_DIRECTORY &&
		node->readdir != 0 )
	{
		ZLOX_DIRENT * ret = node->readdir(node, index);
		zlox_fs_unlock(current_task);
		return ret;
	}
	else
	{
		zlox_fs_unlock(current_task);
		return ZLOX_NULL;
	}
}

ZLOX_SINT32 zlox_readdir_fs_safe(ZLOX_FS_NODE *node, ZLOX_UINT32 index, ZLOX_DIRENT * output)
{
	zlox_fs_lock(current_task);
	if (node == ZLOX_NULL)
	{
		zlox_fs_unlock(current_task);
		return -1;
	}

	// Is the node a directory, and does it have a callback?
	if ( (node->flags & 0x7) == ZLOX_FS_DIRECTORY &&
		node->readdir != 0 )
	{
		ZLOX_DIRENT * ret = node->readdir(node, index);
		if(ret == ZLOX_NULL)
		{
			zlox_fs_unlock(current_task);
			return -1;
		}
		if(output == ZLOX_NULL)
		{
			zlox_fs_unlock(current_task);
			return -1;
		}
		zlox_memcpy((ZLOX_UINT8 *)output, (ZLOX_UINT8 *)ret, sizeof(ZLOX_DIRENT));
		zlox_fs_unlock(current_task);
		return 0;
	}
	else
	{
		zlox_fs_unlock(current_task);
		return -1;
	}
}

ZLOX_FS_NODE * zlox_finddir_fs(ZLOX_FS_NODE *node, ZLOX_CHAR *name)
{
	zlox_fs_lock(current_task);
	if (node == ZLOX_NULL)
	{
		zlox_fs_unlock(current_task);
		return ZLOX_NULL;
	}
	// Is the node a directory, and does it have a callback?
	if ( (node->flags & 0x7) == ZLOX_FS_DIRECTORY &&
	 	node->finddir != 0 )
	{
		ZLOX_FS_NODE * ret = node->finddir(node, name);
		zlox_fs_unlock(current_task);
		return ret;
	}
	else
	{
		zlox_fs_unlock(current_task);
		return ZLOX_NULL;
	}
}

ZLOX_SINT32 zlox_finddir_fs_safe(ZLOX_FS_NODE *node, ZLOX_CHAR *name, ZLOX_FS_NODE * output)
{
	zlox_fs_lock(current_task);
	if (node == ZLOX_NULL)
	{
		zlox_fs_unlock(current_task);
		return -1;
	}

	// Is the node a directory, and does it have a callback?
	if ( (node->flags & 0x7) == ZLOX_FS_DIRECTORY &&
		node->finddir != 0 )
	{
		ZLOX_FS_NODE * ret = node->finddir(node, name);
		if(ret == ZLOX_NULL)
		{
			zlox_fs_unlock(current_task);
			return -1;
		}
		if(output == ZLOX_NULL)
		{
			zlox_fs_unlock(current_task);
			return -1;
		}
		zlox_memcpy((ZLOX_UINT8 *)output, (ZLOX_UINT8 *)ret, sizeof(ZLOX_FS_NODE));
		zlox_fs_unlock(current_task);
		return 0;
	}
	else
	{
		zlox_fs_unlock(current_task);
		return -1;
	}
}

ZLOX_UINT32 zlox_remove_fs(ZLOX_FS_NODE *node)
{
	zlox_fs_lock(current_task);
	if (node == ZLOX_NULL)
	{
		zlox_fs_unlock(current_task);
		return 0;
	}
	if ( node->remove != 0 )
	{
		ZLOX_UINT32 ret = node->remove(node);
		zlox_fs_unlock(current_task);
		return ret;
	}
	else
	{
		zlox_fs_unlock(current_task);
		return 0;
	}
}

ZLOX_UINT32 zlox_rename_fs(ZLOX_FS_NODE *node, ZLOX_CHAR * rename)
{
	zlox_fs_lock(current_task);
	if(node == ZLOX_NULL)
	{
		zlox_fs_unlock(current_task);
		return 0;
	}
	if(node->rename != 0)
	{
		ZLOX_UINT32 ret = node->rename(node, rename);
		zlox_fs_unlock(current_task);
		return ret;
	}
	else
	{
		zlox_fs_unlock(current_task);
		return 0;
	}
}

// get the root of the filesystem.
ZLOX_FS_NODE * zlox_get_fs_root()
{
	return fs_root;
}

