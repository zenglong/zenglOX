// fs.h -- 和文件系统相关的结构定义

#ifndef _FS_H_
#define _FS_H_

#include "common.h"

#define FS_FILE 0x01
#define FS_DIRECTORY 0x02

struct _FS_NODE;
struct _DIRENT;
typedef struct _FS_NODE FS_NODE;
typedef struct _DIRENT DIRENT;

struct _FS_NODE
{
	CHAR name[128]; // The filename.
	UINT32 mask; // The permissions mask.
	UINT32 uid; // The owning user.
	UINT32 gid; // The owning group.
	UINT32 flags; // Includes the node type. See #defines above.
	UINT32 inode; // This is device-specific - provides a way for a filesystem to identify files.
	UINT32 length; // Size of the file, in bytes.
	UINT32 impl; // An implementation-defined number.
	VOID * read;
	VOID * write;
	VOID * open;
	VOID * close;
	VOID * readdir;
	VOID * finddir;
	FS_NODE * ptr; // Used by mountpoints and symlinks.
};

struct _DIRENT
{
	CHAR name[128]; // Filename.
	UINT32 ino; // Inode number.
};

#endif // _FS_H_

