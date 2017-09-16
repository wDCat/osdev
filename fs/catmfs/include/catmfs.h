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

int catmfs_fs_node_load_catrfmt(fs_node_t *node, __fs_special_t *fsp_, uint32_t start_addr);

void catmfs_create_fstype();

catmfs_inode_t *catmfs_alloc_inode();

int catmfs_make(catmfs_special_t *fsp, catmfs_inode_t *inode, uint8_t type, char *name, uint32_t catrfmt_inode);

int catmfs_fs_node_make(fs_node_t *node, __fs_special_t *fsp_, uint8_t type, char *name);

int32_t catmfs_fs_node_readdir(fs_node_t *node, __fs_special_t *fsp_, uint32_t count, struct dirent *result);

int catmfs_fs_node_finddir(fs_node_t *node, __fs_special_t *fsp_, const char *name, fs_node_t *result_out);

int catmfs_get_fs_node(uint32_t inode_id, fs_node_t *node);

int32_t catmfs_fs_node_read(fs_node_t *node, __fs_special_t *fsp_, uint32_t offset, uint32_t size, uint8_t *buff);

int32_t catmfs_fs_node_write(fs_node_t *node, __fs_special_t *fsp_, uint32_t offset, uint32_t size, uint8_t *buff);

int catmfs_fs_node_rm(fs_node_t *node, __fs_special_t *fsp_);

__fs_special_t *catmfs_fs_node_mount(void *dev, fs_node_t *node);

#endif //W2_CATMFS_H
