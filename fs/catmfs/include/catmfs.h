//
// Created by dcat on 8/25/17.
//

#ifndef W2_CATMFS_H
#define W2_CATMFS_H

#include "fs_node.h"
#include "vfs.h"

#define CATMFS_MAGIC 0x3366
#define CATMFS_BLK 0xAA32
#define CATMFS_ERR_NONE 0
#define CATMFS_ERR_GEN 1
#define CATMFS_ERR_IO 2
#define CATMFS_ERR_NO_EMPTY_DIR 3
typedef struct catmfs_inode {
    uint16_t magic;
    uint8_t type;/*FS_** */
    uint16_t permission;
    uint32_t size;
    uint32_t uid;
    uint32_t gid;
    uint32_t reserved;/*DIR:child inodes count;FILE:undefined*/
    struct catmfs_inode *finode;
    uint32_t singly_block;
    //data;
} catmfs_inode_t;
typedef struct {
    uint16_t magic;
    //data
} catmfs_blk_t;
typedef struct {
    uint32_t inode;
    uint16_t type;
    uint8_t len;
    uint8_t name_len;
} catmfs_dir_t;
typedef struct {
    uint32_t root_inode_id;
    uint32_t page_size;
    uint32_t max_singly_blks;
    int8_t errno;
} catmfs_special_t;

void catmfs_create_fstype();

#endif //W2_CATMFS_H
