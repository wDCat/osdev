//
// Created by dcat on 8/22/17.
//

#ifndef W2_VFS_H
#define W2_VFS_H

#include "../../../ker/include/system.h"
#include "fs_node.h"

#define MAX_MOUNT_POINTS 64
#define MAX_FILE_HANDLES 10
#define MAX_PATH_LEN 1024

typedef __fs_special_t *(*mount_type_t)(void *dev, fs_node_t *node);

typedef int (*get_node_by_id_type_t)(__fs_special_t *fsp, uint32_t id, fs_node_t *node);

typedef int (*make_type_t)(struct fs_node *, __fs_special_t *fsp, uint8_t type, char *name);

typedef int (*rm_type_t)(struct fs_node *, __fs_special_t *fsp);

typedef struct {
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
typedef struct {
    //char path[MAX_PATH_LEN];
    bool present;
    fs_node_t node;
    mount_point_t *mp;
    uint32_t offset;
    uint32_t mode;
    void *reserved;
} file_handle_t;

mount_point_t mount_points[MAX_MOUNT_POINTS];
uint32_t mount_points_count;
file_handle_t global_fh_table[MAX_FILE_HANDLES];

void vfs_install();

void mount_rootfs(uint32_t initrd);

int load_initrd(uint32_t initrd);

void vfs_init(vfs_t *vfs);

int vfs_cd(vfs_t *vfs, const char *path);

int vfs_mount(vfs_t *vfs, const char *path, fs_t *fs, void *dev);

int32_t vfs_ls(vfs_t *vfs, dirent_t *dirs, uint32_t max_count);

int vfs_make(vfs_t *vfs, uint8_t type, const char *name);

int32_t vfs_read(vfs_t *vfs, uint32_t offset, uint32_t size, uchar_t *buff);

int8_t sys_open(const char *name, uint8_t mode);

int8_t kclose(uint32_t pid, int8_t fd);

int8_t kopen(uint32_t pid, const char *name, uint8_t mode);

int32_t sys_read(int8_t fd, int32_t size, uchar_t *buff);

int32_t kread(uint32_t pid, int8_t fd, uint32_t offset, int32_t size, uchar_t *buff);

#endif //W2_VFS_H
