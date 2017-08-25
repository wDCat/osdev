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
} vfs_t;
mount_point_t mount_points[MAX_MOUNT_POINTS];
uint32_t mount_points_count;

#endif //W2_VFS_H
