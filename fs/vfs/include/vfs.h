//
// Created by dcat on 8/22/17.
//

#ifndef W2_VFS_H
#define W2_VFS_H

#include "system.h"
#include "fs_node.h"
#include "stat.h"
#include "uvfs.h"
#include "uproc.h"


#define MAX_FILENAME_LEN 256
#define MAX_PATH_LEN 1024
#define MAX_MOUNT_POINTS 64
#define MAX_FILE_HANDLES 10
#define MAX_PATH_LEN 1024

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

typedef __fs_special_t *(*mount_type_t)(void *dev, fs_node_t *node);

typedef int (*get_node_by_id_type_t)(__fs_special_t *fsp, uint32_t id, fs_node_t *node);

typedef int (*make_type_t)(struct fs_node *, __fs_special_t *fsp, uint8_t type, char *name);

typedef int (*rm_type_t)(struct fs_node *, __fs_special_t *fsp);

typedef int (*lseek_type_t)(struct fs_node *, __fs_special_t *fsp, uint32_t offset);

typedef struct fs__ {
    char name[256];
    mount_type_t mount;
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;
    get_node_by_id_type_t getnode;
    make_type_t make;
    readdir_type_t readdir;
    finddir_type_t finddir;
    rm_type_t rm;
    lseek_type_t lseek;
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
typedef struct  file_handle {
    //char path[MAX_PATH_LEN];
    bool present;
    fs_node_t node;
    mount_point_t *mp;
    uint32_t mode;
    void *reserved;
} file_handle_t;

extern mount_point_t mount_points[MAX_MOUNT_POINTS];
extern uint32_t mount_points_count;
extern file_handle_t global_fh_table[MAX_FILE_HANDLES];
extern vfs_t vfs;

void vfs_install();

void mount_rootfs(uint32_t initrd);

void vfs_init(vfs_t *vfs);

int vfs_cd(vfs_t *vfs, const char *path);

int vfs_mount(vfs_t *vfs, const char *path, fs_t *fs, void *dev);

int32_t vfs_ls(vfs_t *vfs, dirent_t *dirs, uint32_t max_count);

int vfs_make(vfs_t *vfs, uint8_t type, const char *name);

int32_t vfs_read(vfs_t *vfs, uint32_t size, uchar_t *buff);

int8_t sys_open(const char *name, uint8_t mode);

int8_t sys_close(int8_t fd);

int8_t kclose(uint32_t pid, int8_t fd);

int8_t kopen(uint32_t pid, const char *name, uint8_t mode);

int32_t sys_read(int8_t fd, int32_t size, uchar_t *buff);

int32_t kread(uint32_t pid, int8_t fd, int32_t size, uchar_t *buff);

int32_t kwrite(uint32_t pid, int8_t fd, int32_t size, uchar_t *buff);

int32_t sys_write(int8_t fd, int32_t size, uchar_t *buff);

int sys_stat(const char *path, stat_t *stat);

int sys_stat64(const char *name, stat64_t *stat);

int sys_ls(const char *path, dirent_t *dirents, uint32_t count);

void kclose_all(uint32_t pid);

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

#endif //W2_VFS_H
