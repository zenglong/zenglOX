// zlox_fs.c -- Defines the interface and structures relating to the virtual file system.

#include "zlox_fs.h"

ZLOX_FS_NODE *fs_root = 0; // The root of the filesystem.

ZLOX_UINT32 zlox_read_fs(ZLOX_FS_NODE *node, ZLOX_UINT32 offset, ZLOX_UINT32 size, ZLOX_UINT8 *buffer)
{
    // Has the node got a read callback?
    if (node->read != 0)
        return node->read(node, offset, size, buffer);
    else
        return 0;
}

ZLOX_DIRENT * zlox_readdir_fs(ZLOX_FS_NODE *node, ZLOX_UINT32 index)
{
    // Is the node a directory, and does it have a callback?
    if ( (node->flags & 0x7) == ZLOX_FS_DIRECTORY &&
         node->readdir != 0 )
        return node->readdir(node, index);
    else
        return 0;
}

ZLOX_FS_NODE * zlox_finddir_fs(ZLOX_FS_NODE *node, ZLOX_CHAR *name)
{
    // Is the node a directory, and does it have a callback?
    if ( (node->flags & 0x7) == ZLOX_FS_DIRECTORY &&
         node->finddir != 0 )
        return node->finddir(node, name);
    else
        return 0;
}

