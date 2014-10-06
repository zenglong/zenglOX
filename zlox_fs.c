// zlox_fs.c -- Defines the interface and structures relating to the virtual file system.

#include "zlox_fs.h"

ZLOX_FS_NODE *fs_root = 0; // The root of the filesystem.

ZLOX_UINT32 zlox_read_fs(ZLOX_FS_NODE *node, ZLOX_UINT32 offset, ZLOX_UINT32 size, ZLOX_UINT8 *buffer)
{
    if(node == 0)
        return 0;
    // Has the node got a read callback?
    if (node->read != 0)
        return node->read(node, offset, size, buffer);
    else
        return 0;
}

ZLOX_UINT32 zlox_write_fs(ZLOX_FS_NODE *node, ZLOX_UINT32 offset, ZLOX_UINT32 size, ZLOX_UINT8 *buffer)
{
    if(node == 0)
        return 0;
    // Has the node got a read callback?
    if (node->write != 0)
        return node->write(node, offset, size, buffer);
    else
        return 0;
}

ZLOX_FS_NODE * zlox_writedir_fs(ZLOX_FS_NODE *node, ZLOX_CHAR *name, ZLOX_UINT16 type)
{
	if(node == 0)
		return 0;
	if(node->writedir != 0)
		return node->writedir(node, name, type);
	else
		return 0;
}

ZLOX_SINT32 zlox_writedir_fs_safe(ZLOX_FS_NODE * node, ZLOX_CHAR *name, ZLOX_UINT16 type, ZLOX_FS_NODE * output)
{
	if(node == 0)
		return -1;
	if(node->writedir != 0)
	{
		ZLOX_FS_NODE * ret = node->writedir(node, name, type);
		if(ret == ZLOX_NULL)
			return -1;
		if(output == ZLOX_NULL)
			return -1;
		zlox_memcpy((ZLOX_UINT8 *)output, (ZLOX_UINT8 *)ret, sizeof(ZLOX_FS_NODE));
		return 0;
	}
	else
		return -1;
}

ZLOX_DIRENT * zlox_readdir_fs(ZLOX_FS_NODE *node, ZLOX_UINT32 index)
{
    if (node == 0)
        return 0;
    // Is the node a directory, and does it have a callback?
    if ( (node->flags & 0x7) == ZLOX_FS_DIRECTORY &&
         node->readdir != 0 )
        return node->readdir(node, index);
    else
        return 0;
}

ZLOX_SINT32 zlox_readdir_fs_safe(ZLOX_FS_NODE *node, ZLOX_UINT32 index, ZLOX_DIRENT * output)
{
	if (node == 0)
		return -1;

	// Is the node a directory, and does it have a callback?
	if ( (node->flags & 0x7) == ZLOX_FS_DIRECTORY &&
		node->readdir != 0 )
	{
		ZLOX_DIRENT * ret = node->readdir(node, index);
		if(ret == ZLOX_NULL)
			return -1;
		if(output == ZLOX_NULL)
			return -1;
		zlox_memcpy((ZLOX_UINT8 *)output, (ZLOX_UINT8 *)ret, sizeof(ZLOX_DIRENT));
		return 0;
	}
	else
		return -1;
}

ZLOX_FS_NODE * zlox_finddir_fs(ZLOX_FS_NODE *node, ZLOX_CHAR *name)
{
    if (node == 0)
        return 0;
    // Is the node a directory, and does it have a callback?
    if ( (node->flags & 0x7) == ZLOX_FS_DIRECTORY &&
         node->finddir != 0 )
        return node->finddir(node, name);
    else
        return 0;
}

ZLOX_SINT32 zlox_finddir_fs_safe(ZLOX_FS_NODE *node, ZLOX_CHAR *name, ZLOX_FS_NODE * output)
{
	if (node == 0)
		return -1;

	// Is the node a directory, and does it have a callback?
	if ( (node->flags & 0x7) == ZLOX_FS_DIRECTORY &&
		node->finddir != 0 )
	{
		ZLOX_FS_NODE * ret = node->finddir(node, name);
		if(ret == ZLOX_NULL)
			return -1;
		if(output == ZLOX_NULL)
			return -1;
		zlox_memcpy((ZLOX_UINT8 *)output, (ZLOX_UINT8 *)ret, sizeof(ZLOX_FS_NODE));
		return 0;
	}
	else
		return -1;
}

ZLOX_UINT32 zlox_remove_fs(ZLOX_FS_NODE *node)
{
    if (node == 0)
        return 0;
    if ( node->remove != 0 )
        return node->remove(node);
    else
        return 0;
}

ZLOX_UINT32 zlox_rename_fs(ZLOX_FS_NODE *node, ZLOX_CHAR * rename)
{
	if(node == 0)
		return 0;
	if(node->rename != 0)
		return node->rename(node, rename);
	else
		return 0;
}

// get the root of the filesystem.
ZLOX_FS_NODE * zlox_get_fs_root()
{
	return fs_root;
}

