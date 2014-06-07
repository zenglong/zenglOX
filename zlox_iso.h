/* zlox_iso.h -- 包含和iso 9660文件系统相关的结构定义 */

#ifndef _ZLOX_ISO_H_
#define _ZLOX_ISO_H_

#include "zlox_common.h"
#include "zlox_fs.h"

#define ZLOX_PVD_START_LBA 16
#define ZLOX_PVD_TYPE 1
#define ZLOX_VD_TERMINATOR 255
#define ZLOX_VD_ID_1 0x43
#define ZLOX_VD_ID_2 0x44
#define ZLOX_VD_ID_3 0x30
#define ZLOX_VD_ID_4 0x30
#define ZLOX_VD_ID_5 0x31
#define ZLOX_PVD_PTSZ_OFFSET 132
#define ZLOX_PVD_LPT_OFFSET 140
#define ZLOX_PVD_ROOT_OFFSET 156

struct _ZLOX_PATH_TABLE_ENTRY
{
	ZLOX_UINT8 dir_id_length;
	ZLOX_UINT8 ext_length;
	ZLOX_UINT32 lba;
	ZLOX_UINT16 parent_index;
}__attribute__((packed));

typedef struct _ZLOX_PATH_TABLE_ENTRY ZLOX_PATH_TABLE_ENTRY;

struct _ZLOX_ISO_DIR_RECORD
{
	ZLOX_UINT8 length;
	ZLOX_UINT8 ext_length;
	ZLOX_UINT32 l_lba;
	ZLOX_UINT32 m_lba;
	ZLOX_UINT32 l_datasize;
	ZLOX_UINT32 m_datasize;
	ZLOX_UINT8 datatime[7];
	ZLOX_UINT8 file_flags;
	ZLOX_UINT8 file_unit_size;
	ZLOX_UINT8 interleave_gap_size;
	ZLOX_UINT32 vol_seq_number;
	ZLOX_UINT8 file_name_length;
}__attribute__((packed));

typedef struct _ZLOX_ISO_DIR_RECORD ZLOX_ISO_DIR_RECORD;

typedef struct _ZLOX_ISO_CACHE
{
	ZLOX_UINT8 * ptr;
	ZLOX_UINT32 lba;
	ZLOX_UINT32 size;
}ZLOX_ISO_CACHE;

typedef struct _ZLOX_ISO_PATH_TABLE_EXTDATA
{
	ZLOX_UINT32 pt_offset;
	ZLOX_UINT32 datasize;
}ZLOX_ISO_PATH_TABLE_EXTDATA;

// 卸载iso目录，将堆中分配的相关内存资源释放掉
ZLOX_FS_NODE * zlox_unmount_iso();
// 挂载CDROM到iso目录下
ZLOX_FS_NODE * zlox_mount_iso();

#endif // _ZLOX_ISO_H_

