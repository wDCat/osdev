//
// Created by dcat on 8/22/17.
//

#ifndef W2_VFS_H
#define W2_VFS_H

#include "system.h"
#include "fs_node.h"

#define MAX_MOUNT_POINTS 64

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
    char path[256];
    fs_node_t current;
    fs_node_t current_dir;
} vfs_t;

mount_point_t mount_points[MAX_MOUNT_POINTS];
uint32_t mount_points_count;

void vfs_init(vfs_t *vfs);

int vfs_cd(vfs_t *vfs, const char *path);

int vfs_mount(vfs_t *vfs, const char *path, fs_t *fs, void *dev);

int32_t vfs_ls(vfs_t *vfs, dirent_t *dirs, uint32_t max_count);

int vfs_make(vfs_t *vfs, uint8_t type, const char *name);

int32_t vfs_read(vfs_t *vfs, uint32_t offset, uint32_t size, uint8_t *buff);

#endif //W2_VFS_H
