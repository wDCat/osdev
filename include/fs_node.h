//
// Created by dcat on 3/20/17.
//

#ifndef W2_FS_NODE_H
#define W2_FS_NODE_H

#include "../ker/include/system.h"

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08
typedef void __fs_special_t;

typedef int32_t (*read_type_t)(struct fs_node *, __fs_special_t *fsp, uint32_t, uint8_t *);

typedef int32_t (*write_type_t)(struct fs_node *, __fs_special_t *fsp, uint32_t, uint8_t *);

typedef int (*open_type_t)(struct file_handle *handler);

typedef int (*close_type_t)(struct file_handle *handler);

//typedef dirent_t *(*readdir_type_t)(struct fs_node *, uint32_t);
typedef int32_t (*readdir_type_t)(struct fs_node *node, __fs_special_t *fsp, uint32_t count, struct dirent *result);

typedef int (*finddir_type_t)(struct fs_node *, __fs_special_t *fsp, const char *name, struct fs_node *result_out);

typedef struct fs_node {
    char name[256];
    uint32_t type;//FS_**
    uint32_t mode;//POSIX
    uint32_t uid;
    uint32_t gid;
    uint32_t inode;
    uint32_t size;
    void *vfs_mp;
    __fs_special_t *fsp;
    uint32_t offset;
    void *reserved;
} fs_node_t;

#include "uvfs.h"

uint32_t read_fs(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff);

uint32_t write_fs(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff);

#endif //W2_FS_NODE_H
