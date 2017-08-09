//
// Created by dcat on 3/20/17.
//

#ifndef W2_FS_NODE_H
#define W2_FS_NODE_H

#include "system.h"

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08

typedef uint32_t (*read_type_t)(struct fs_node *, uint32_t, uint32_t, uint8_t *);

typedef uint32_t (*write_type_t)(struct fs_node *, uint32_t, uint32_t, uint8_t *);

typedef void (*open_type_t)(struct fs_node *);

typedef void (*close_type_t)(struct fs_node *);

typedef struct dirent *(*readdir_type_t)(struct fs_node *, uint32_t);

typedef struct fs_node *(*finddir_type_t)(struct fs_node *, char *name);

typedef struct fs_node {
    char name[256];
    uint32_t mask;
    uint32_t uid;
    uint32_t gid;
    uint32_t flags;       // Includes the node type. See #defines above.
    uint32_t inode;       // This is device-specific - provides a way for a filesystem to identify files.
    uint32_t length;      // Size of the file, in bytes.
    uint32_t impl;        // An implementation-defined number.
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;
    readdir_type_t readdir;
    finddir_type_t finddir;
    struct fs_node *ptr;
} fs_node_t;

uint32_t read_fs(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff);

uint32_t write_fs(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff);

#endif //W2_FS_NODE_H
