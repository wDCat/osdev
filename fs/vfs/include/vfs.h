//
// Created by dcat on 8/22/17.
//

#ifndef W2_VFS_H
#define W2_VFS_H

#include "system.h"
#include "fs_node.h"
#include "ustat.h"
#include "uvfs.h"
#include "uproc.h"
#include "mutex.h"

#define MAX_FILENAME_LEN 256
#define MAX_PATH_LEN 1024
#define MAX_MOUNT_POINTS 64
#define MAX_FILE_HANDLES 10
#define MAX_PATH_LEN 1024
#define MAX_LINK_LOOP 10
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#define O_CREAT        0x0200

typedef __fs_special_t *(*mount_type_t)(void *dev, fs_node_t *node);

typedef int (*readinode_type_t)(__fs_special_t *, uint32_t inode, fs_node_t *node);

typedef int (*make_type_t)(struct fs_node *, __fs_special_t *, uint8_t type, char *name);

typedef int (*rm_type_t)(struct fs_node *, __fs_special_t *);

typedef int (*lseek_type_t)(struct fs_node *, __fs_special_t *, uint32_t offset);

typedef uint32_t (*tell_type_t)(struct fs_node *, __fs_special_t *);

typedef int (*symlink_type_t)(struct fs_node *, __fs_special_t *, const char *);

typedef int (*readlink_type_t)(struct fs_node *, __fs_special_t *, char *, int);

typedef int (*ioctl_type_t)(struct fs_node *, __fs_special_t *, unsigned int cmd, unsigned long arg);

typedef struct fs__ {
    char name[256];
    mount_type_t mount;
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;
    readinode_type_t readinode;
    make_type_t make;
    readdir_type_t readdir;
    finddir_type_t finddir;
    rm_type_t rm;
    lseek_type_t lseek;
    tell_type_t tell;
    symlink_type_t symlink;
    readlink_type_t readlink;
    ioctl_type_t ioctl;
} fs_t;
typedef struct {
    char path[256];
    __fs_special_t *fsp;
    fs_t *fs;
    fs_node_t root;
} mount_point_t;
typedef struct {
    char path[MAX_PATH_LEN];
    fs_node_t current;
    fs_node_t current_dir;
} vfs_t;
typedef struct file_handle {
    //char path[MAX_PATH_LEN];
    bool present;
    fs_node_t node;
    mount_point_t *mp;
    int flags:1;
    int amode;
    char *path;
    void *reserved;
} file_handle_t;

extern mount_point_t mount_points[MAX_MOUNT_POINTS];
extern uint32_t mount_points_count;
extern file_handle_t global_fh_table[MAX_FILE_HANDLES];
extern vfs_t vfs;
extern mutex_lock_t vfs_lock;

bool vfs_ready();

void vfs_install();

void mount_rootfs(uint32_t initrd);

int vfs_get_node(vfs_t *vfs, const char *path, fs_node_t *node);

void vfs_init(vfs_t *vfs);

int vfs_cd(vfs_t *vfs, const char *path);

mount_point_t *vfs_get_mount_point(vfs_t *vfs, fs_node_t *node);

int vfs_mount(vfs_t *vfs, const char *path, fs_t *fs, void *dev);

int32_t vfs_ls(vfs_t *vfs, dirent_t *dirs, uint32_t max_count);

int vfs_make(vfs_t *vfs, uint8_t type, const char *name);

int32_t vfs_read(vfs_t *vfs, uint32_t size, uchar_t *buff);

int32_t vfs_symlink(vfs_t *vfs, const char *name, const char *target);

int32_t sys_read(int8_t fd, uchar_t *buff, int32_t size);

int32_t kread(uint32_t pid, int8_t fd, uchar_t *buff, int32_t size);

int32_t kwrite(uint32_t pid, int8_t fd, uchar_t *buff, int32_t size);

int32_t sys_write(int8_t fd, uchar_t *buff, int32_t size);

int sys_ls(const char *path, dirent_t *dirents, uint32_t count);

int sys_access(const char *path, int mode);

int sys_chdir(const char *path);

int kdup3(pid_t pid, int oldfd, int newfd, int flags);

int sys_dup3(int oldfd, int newfd, int flags);

bool isopenedfd(pid_t pid, int fd);

char *sys_getcwd(char *buff, int len);

char *kgetcwd(pid_t pid);

int vfs_pretty_path(const char *path, char *out);

off_t klseek(uint32_t pid, int8_t fd, off_t offset, int where);

off_t sys_lseek(int8_t fd, off_t offset, int where);

off_t ktell(uint32_t pid, int8_t fd);

off_t sys_tell(int8_t fd);

int vfs_fix_path5(uint32_t pid, const char *name,
                  const char *cwd, char *out, int max_len);

int vfs_fix_path(uint32_t pid, const char *name, char *out, int max_len);

int vfs_get_node4(vfs_t *vfs, const char *path, fs_node_t *node, bool follow_link);

#endif //W2_VFS_H
