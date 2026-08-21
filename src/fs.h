#ifndef SLOPOS_FS_H
#define SLOPOS_FS_H

#include "types.h"

#define FS_MAX_FILES 16
#define FS_MAX_FDS   32

void fs_init(void);
int  fs_register_file(const char *name, const u8 *data, u64 size);

/* VFS syscall-style operations (Linux-like) */
int  fs_open(const char *name, int flags);
int  fs_read(int fd, u8 *buf, u64 len);
int  fs_write(int fd, const u8 *buf, u64 len);
int  fs_close(int fd);

#endif
