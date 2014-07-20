// zlox_fs.h -- Defines the interface for and structures relating to the virtual file system.

#ifndef _ZLOX_FS_H_
#define _ZLOX_FS_H_

#include "zlox_common.h"

#define ZLOX_FS_FILE 0x01
#define ZLOX_FS_DIRECTORY 0x02

struct _ZLOX_FS_NODE;
struct _ZLOX_DIRENT;
typedef struct _ZLOX_FS_NODE ZLOX_FS_NODE;
typedef struct _ZLOX_DIRENT ZLOX_DIRENT;

typedef ZLOX_UINT32 (*ZLOX_READ_CALLBACK)(ZLOX_FS_NODE *,ZLOX_UINT32,ZLOX_UINT32,ZLOX_UINT8 *);
typedef ZLOX_UINT32 (*ZLOX_WRITE_CALLBACK)(ZLOX_FS_NODE *,ZLOX_UINT32,ZLOX_UINT32,ZLOX_UINT8 *);
typedef ZLOX_VOID (*ZLOX_OPEN_CALLBACK)(ZLOX_FS_NODE *);
typedef ZLOX_VOID (*ZLOX_CLOSE_CALLBACK)(ZLOX_FS_NODE *);
typedef ZLOX_DIRENT * (*ZLOX_READDIR_CALLBACK)(ZLOX_FS_NODE *,ZLOX_UINT32);
typedef ZLOX_FS_NODE * (*ZLOX_FINDDIR_CALLBACK)(ZLOX_FS_NODE *,ZLOX_CHAR *name);
typedef ZLOX_FS_NODE * (*ZLOX_WRITEDIR_CALLBACK)(ZLOX_FS_NODE *,ZLOX_CHAR *name,ZLOX_UINT16 type);
typedef ZLOX_UINT32 (*ZLOX_REMOVE_CALLBACK)(ZLOX_FS_NODE *);
typedef ZLOX_UINT32 (*ZLOX_RENAME_CALLBACK)(ZLOX_FS_NODE *node, ZLOX_CHAR * rename);

struct _ZLOX_FS_NODE
{
	ZLOX_CHAR name[128]; // The filename.
	ZLOX_UINT32 mask; // The permissions mask.
	ZLOX_UINT32 uid; // The owning user.
	ZLOX_UINT32 gid; // The owning group.
	ZLOX_UINT32 flags; // Includes the node type. See #defines above.
	ZLOX_UINT32 inode; // This is device-specific - provides a way for a filesystem to identify files.
	ZLOX_UINT32 length; // Size of the file, in bytes.
	ZLOX_UINT32 impl; // An implementation-defined number.
	ZLOX_READ_CALLBACK read;
	ZLOX_WRITE_CALLBACK write;
	ZLOX_OPEN_CALLBACK open;
	ZLOX_CLOSE_CALLBACK close;
	ZLOX_READDIR_CALLBACK readdir;
	ZLOX_WRITEDIR_CALLBACK writedir;
	ZLOX_FINDDIR_CALLBACK finddir;
	ZLOX_REMOVE_CALLBACK remove;
	ZLOX_RENAME_CALLBACK rename;
	ZLOX_FS_NODE * ptr; // Used by mountpoints and symlinks.
};

struct _ZLOX_DIRENT
{
	ZLOX_CHAR name[128]; // Filename.
	ZLOX_UINT32 ino; // Inode number.
};

extern ZLOX_FS_NODE * fs_root; // The root of the filesystem.

ZLOX_UINT32 zlox_read_fs(ZLOX_FS_NODE *node, ZLOX_UINT32 offset, ZLOX_UINT32 size, ZLOX_UINT8 *buffer);
ZLOX_UINT32 zlox_write_fs(ZLOX_FS_NODE *node, ZLOX_UINT32 offset, ZLOX_UINT32 size, ZLOX_UINT8 *buffer);
ZLOX_FS_NODE * zlox_writedir_fs(ZLOX_FS_NODE *node, ZLOX_CHAR *name, ZLOX_UINT16 type);
ZLOX_DIRENT * zlox_readdir_fs(ZLOX_FS_NODE *node, ZLOX_UINT32 index);
ZLOX_FS_NODE * zlox_finddir_fs(ZLOX_FS_NODE *node, ZLOX_CHAR *name);
ZLOX_UINT32 zlox_remove_fs(ZLOX_FS_NODE *node);
ZLOX_UINT32 zlox_rename_fs(ZLOX_FS_NODE *node, ZLOX_CHAR * rename);
// get the root of the filesystem.
ZLOX_FS_NODE * zlox_get_fs_root();

#endif //_ZLOX_FS_H_

