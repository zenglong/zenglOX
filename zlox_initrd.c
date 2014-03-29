// zlox_initrd.c -- Defines the interface for and structures relating to the initial ramdisk.

#include "zlox_kheap.h"
#include "zlox_initrd.h"

ZLOX_INITRD_HEADER * initrd_header; // The header.
ZLOX_INITRD_FILE_HEADER * file_headers; // The list of file headers.
ZLOX_FS_NODE *initrd_root; // Our root directory node.
ZLOX_FS_NODE *initrd_dev; // We also add a directory node for /dev, so we can mount devfs later on.
ZLOX_FS_NODE *root_nodes; // List of file nodes.
ZLOX_UINT32 nroot_nodes; // Number of file nodes.

ZLOX_DIRENT dirent;

static ZLOX_UINT32 zlox_initrd_read(ZLOX_FS_NODE * node, ZLOX_UINT32 arg_offset, ZLOX_UINT32 size, ZLOX_UINT8 * buffer)
{
	ZLOX_INITRD_FILE_HEADER header = file_headers[node->inode];
	if (arg_offset >= header.length)
		return 0;
	if(size == 0)
		return 0;
	if(buffer == 0)
		return 0;
	if (arg_offset + size > header.length)
		size = header.length - arg_offset;
	zlox_memcpy(buffer, (ZLOX_UINT8 *)(header.offset + arg_offset), size);
	return size;
}

static ZLOX_DIRENT *zlox_initrd_readdir(ZLOX_FS_NODE *node, ZLOX_UINT32 index)
{
	if (node == initrd_root && index == 0)
	{
		zlox_strcpy(dirent.name, "dev");
		dirent.name[3] = 0;
		dirent.ino = 0;
		return &dirent;
	}

	if (index-1 >= nroot_nodes)
		return 0;

	zlox_strcpy(dirent.name, root_nodes[index-1].name);
	dirent.name[zlox_strlen(root_nodes[index-1].name)] = 0;
	dirent.ino = root_nodes[index-1].inode;
	return &dirent;
}

static ZLOX_FS_NODE *zlox_initrd_finddir(ZLOX_FS_NODE *node, ZLOX_CHAR *name)
{
	if (node == initrd_root &&
		!zlox_strcmp(name, "dev") )
		return initrd_dev;

	ZLOX_UINT32 i;
	for (i = 0; i < nroot_nodes; i++)
		if (!zlox_strcmp(name, root_nodes[i].name))
			return &root_nodes[i];
	return 0;
}

ZLOX_FS_NODE * zlox_initialise_initrd(ZLOX_UINT32 location)
{
	// Initialise the main and file header pointers and populate the root directory.
	initrd_header = (ZLOX_INITRD_HEADER *)location;
	file_headers = (ZLOX_INITRD_FILE_HEADER *) (location+sizeof(ZLOX_INITRD_HEADER));

	// Initialise the root directory.
	initrd_root = (ZLOX_FS_NODE *)zlox_kmalloc(sizeof(ZLOX_FS_NODE));
	zlox_strcpy(initrd_root->name, "initrd");
	initrd_root->mask = initrd_root->uid = initrd_root->gid = initrd_root->inode = initrd_root->length = 0;
	initrd_root->flags = ZLOX_FS_DIRECTORY;
	initrd_root->read = 0;
	initrd_root->write = 0;
	initrd_root->open = 0;
	initrd_root->close = 0;
	initrd_root->readdir = &zlox_initrd_readdir;
	initrd_root->finddir = &zlox_initrd_finddir;
	initrd_root->ptr = 0;
	initrd_root->impl = 0;

	// Initialise the /dev directory (required!)
	initrd_dev = (ZLOX_FS_NODE *)zlox_kmalloc(sizeof(ZLOX_FS_NODE));
	zlox_strcpy(initrd_dev->name, "dev");
	initrd_dev->mask = initrd_dev->uid = initrd_dev->gid = initrd_dev->inode = initrd_dev->length = 0;
	initrd_dev->flags = ZLOX_FS_DIRECTORY;
	initrd_dev->read = 0;
	initrd_dev->write = 0;
	initrd_dev->open = 0;
	initrd_dev->close = 0;
	initrd_dev->readdir = &zlox_initrd_readdir;
	initrd_dev->finddir = &zlox_initrd_finddir;
	initrd_dev->ptr = 0;
	initrd_dev->impl = 0;

	root_nodes = (ZLOX_FS_NODE *)zlox_kmalloc(sizeof(ZLOX_FS_NODE) * initrd_header->nfiles);
	nroot_nodes = initrd_header->nfiles;

	// For every file...
	ZLOX_UINT32 i;
	for (i = 0; i < initrd_header->nfiles; i++)
	{
		// Edit the file's header - currently it holds the file offset
		// relative to the start of the ramdisk. We want it relative to the start
		// of memory.
		file_headers[i].offset += location;
		// Create a new file node.
		zlox_strcpy(root_nodes[i].name, (const ZLOX_CHAR *)file_headers[i].name);
		root_nodes[i].mask = root_nodes[i].uid = root_nodes[i].gid = 0;
		root_nodes[i].length = file_headers[i].length;
		root_nodes[i].inode = i;
		root_nodes[i].flags = ZLOX_FS_FILE;
		root_nodes[i].read = &zlox_initrd_read;
		root_nodes[i].write = 0;
		root_nodes[i].readdir = 0;
		root_nodes[i].finddir = 0;
		root_nodes[i].open = 0;
		root_nodes[i].close = 0;
		root_nodes[i].impl = 0;
	}
	return initrd_root;
}

