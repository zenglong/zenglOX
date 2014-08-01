// syscall.c Defines the implementation of a system call system.

#include "syscall.h"

DEFN_SYSCALL1(monitor_write, SYSCALL_MONITOR_WRITE, const char*);
DEFN_SYSCALL1(monitor_write_hex, SYSCALL_MONITOR_WRITE_HEX, UINT32);
DEFN_SYSCALL1(monitor_write_dec, SYSCALL_MONITOR_WRITE_DEC, UINT32);
DEFN_SYSCALL1(monitor_put, SYSCALL_MONITOR_PUT, char);
DEFN_SYSCALL1(execve, SYSCALL_EXECVE, const char*);
DEFN_SYSCALL3(get_tskmsg,SYSCALL_GET_TSKMSG,void *,void *,BOOL);
DEFN_SYSCALL1(wait, SYSCALL_WAIT, void *);
DEFN_SYSCALL1(set_input_focus, SYSCALL_SET_INPUT_FOCUS, void *);
DEFN_SYSCALL0(get_currentTask,SYSCALL_GET_CURRENT_TASK);
DEFN_SYSCALL1(exit, SYSCALL_EXIT, int);
DEFN_SYSCALL1(finish, SYSCALL_FINISH, void *);
DEFN_SYSCALL1(get_args, SYSCALL_GET_ARGS, void *);
DEFN_SYSCALL1(get_init_esp, SYSCALL_GET_INIT_ESP, void *);
DEFN_SYSCALL1(umalloc, SYSCALL_UMALLOC, int);
DEFN_SYSCALL1(ufree, SYSCALL_UFREE, void *);
DEFN_SYSCALL4(read_fs,SYSCALL_READ_FS,void *,int,int,void *);
DEFN_SYSCALL2(readdir_fs,SYSCALL_READDIR_FS,void *,int);
DEFN_SYSCALL2(finddir_fs,SYSCALL_FINDDIR_FS,void *,void *);
DEFN_SYSCALL0(get_fs_root,SYSCALL_GET_FS_ROOT);
DEFN_SYSCALL2(get_frame_info,SYSCALL_GET_FRAME_INFO,void **,void *);
DEFN_SYSCALL0(get_kheap,SYSCALL_GET_KHEAP);
DEFN_SYSCALL3(get_version,SYSCALL_GET_VERSION,void *,void *,void *);
DEFN_SYSCALL0(reboot,SYSCALL_REBOOT);
DEFN_SYSCALL0(shutdown,SYSCALL_SHUTDOWN);
DEFN_SYSCALL0(idle_cpu,SYSCALL_IDLE_CPU);
DEFN_SYSCALL3(atapi_drive_read_sector,SYSCALL_ATAPI_DRIVE_READ_SECTOR,UINT32,UINT32,void *);
DEFN_SYSCALL2(atapi_drive_read_capacity,SYSCALL_ATAPI_DRIVE_READ_CAPACITY,UINT32,void *);
DEFN_SYSCALL0(ata_get_ide_info,SYSCALL_ATA_GET_IDE_INFO);
DEFN_SYSCALL0(mount_iso,SYSCALL_MOUNT_ISO);
DEFN_SYSCALL0(unmount_iso,SYSCALL_UNMOUNT_ISO);
DEFN_SYSCALL0(overflow_test,SYSCALL_OVERFLOW_TEST);
DEFN_SYSCALL5(ide_ata_access, SYSCALL_IDE_ATA_ACCESS, UINT8, UINT8, UINT32, UINT8, UINT8 *);
DEFN_SYSCALL2(mount_zenglfs, SYSCALL_MOUNT_ZENGLFS, UINT32, UINT32);
DEFN_SYSCALL0(unmount_zenglfs, SYSCALL_UNMOUNT_ZENGLFS);
DEFN_SYSCALL4(write_fs, SYSCALL_WRITE_FS, void *, UINT32 , UINT32 , UINT8 *);
DEFN_SYSCALL3(writedir_fs, SYSCALL_WRITEDIR_FS, void *, CHAR *, UINT16);
DEFN_SYSCALL1(remove_fs, SYSCALL_REMOVE_FS, void *);
DEFN_SYSCALL2(rename_fs, SYSCALL_RENAME_FS, void *, CHAR *);
DEFN_SYSCALL1(vga_set_mode, SYSCALL_VGA_SET_MODE, UINT32);
DEFN_SYSCALL2(vga_update_screen, SYSCALL_VGA_UPDATE_SCREEN, UINT8 *, UINT32);
DEFN_SYSCALL0(vga_get_text_font, SYSCALL_VGA_GET_TEXT_FONT);

