#include "fs.h"
#include "lib.h"
#include "printk.h"

typedef struct {
    char name[64];
    u8  *data;
    u64 size;
} fs_file_t;

typedef struct {
    int file_index;   /* -1 if free */
    u64 pos;
} fs_fd_t;

static fs_file_t files[FS_MAX_FILES];
static int nfiles;
static fs_fd_t fds[FS_MAX_FDS];

void fs_init(void)
{
    int i;
    for (i = 0; i < FS_MAX_FDS; i++)
        fds[i].file_index = -1;
    nfiles = 0;
}

int fs_register_file(const char *name, const u8 *data, u64 size)
{
    if (nfiles >= FS_MAX_FILES)
        return -1;
    strncpy(files[nfiles].name, name, 63);
    files[nfiles].data = (u8 *)data;
    files[nfiles].size = size;
    nfiles++;
    kprintf("[fs] registered '%s' (%llu bytes)\n", name, size);
    return 0;
}

int fs_open(const char *name, int flags)
{
    (void)flags;
    int i;
    for (i = 0; i < nfiles; i++) {
        if (strcmp(files[i].name, name) == 0) {
            int fd;
            for (fd = 0; fd < FS_MAX_FDS; fd++) {
                if (fds[fd].file_index == -1) {
                    fds[fd].file_index = i;
                    fds[fd].pos = 0;
                    return fd;
                }
            }
            return -1;
        }
    }
    return -1;
}

int fs_read(int fd, u8 *buf, u64 len)
{
    if (fd < 0 || fd >= FS_MAX_FDS || fds[fd].file_index < 0)
        return -1;
    fs_file_t *f = &files[fds[fd].file_index];
    u64 remaining = f->size - fds[fd].pos;
    if (len > remaining)
        len = remaining;
    memcpy((void *)buf, (void *)(f->data + fds[fd].pos), (size_t)len);
    fds[fd].pos += len;
    return (int)len;
}

int fs_write(int fd, const u8 *buf, u64 len)
{
    (void)fd; (void)buf; (void)len;
    return -1;   /* read-only filesystem */
}

int fs_close(int fd)
{
    if (fd < 0 || fd >= FS_MAX_FDS)
        return -1;
    fds[fd].file_index = -1;
    fds[fd].pos = 0;
    return 0;
}
