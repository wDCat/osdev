//
// Created by dcat on 8/17/17.
//

#ifndef W2_EXT2_H
#define W2_EXT2_H

#include "superblk.h"
#include "blk_dev.h"

#define SECTION_SIZE 512
#define EXT2_NDIR_BLOCKS 12
#define EXT2_SIND_BLOCK EXT2_NDIR_BLOCKS //一层
#define EXT2_DIND_BLOCK (EXT2_SIND_BLOCK + 1) //二层
#define EXT2_TIND_BLOCK (EXT2_DIND_BLOCK + 1) //三层
#define EXT2_N_BLOCKS (EXT2_TIND_BLOCK + 1)
#define EXT2_MAGIC 0xEF53
#define INODE_TYPE_FIFO 0x1000
#define INODE_TYPE_CHAR_DEV 0x2000
#define INODE_TYPE_DIRECTORY 0x4000
#define INODE_TYPE_BLOCK_DEV 0x6000
#define INODE_TYPE_FILE 0x8000
#define INODE_TYPE_SYMLINK 0xA000
#define INODE_TYPE_SOCKET 0xC000
#define INODE_DIR_TYPE_INDICATOR_DIRECTORY 0x2
#define INODE_DIR_TYPE_INDICATOR_FILE 0x1
#define IS_DIR(x) (((x) & 0xF000) == INODE_TYPE_DIRECTORY)
#define IS_FILE(x) (((x) & 0xF000) == INODE_TYPE_FILE)
#define EXT2_SUPER_BLK_OFFSET 1024
#define EXT2_BLOCK_GROUP_OFFSET 2048
typedef struct {
    uint32_t bg_block_bitmap;      /* block 指针指向 block bitmap */
    uint32_t bg_inode_bitmap;      /* block 指针指向 inode bitmap */
    uint32_t bg_inode_table;       /* block 指针指向 inodes table */
    uint16_t bg_free_blocks_count; /* 空闲的 blocks 计数 */
    uint16_t bg_free_inodes_count; /* 空闲的 inodes 计数 */
    uint16_t bg_dirs_count;   /* 目录计数 */
    uint16_t bg_pad;               /* 可以忽略 */
    uint32_t bg_reserved[3];       /* 可以忽略 */
} ext2_group_desc_t;
typedef struct {
    uint16_t i_mode;    /* File mode */
    uint16_t i_uid;     /* Low 16 bits of Owner Uid */
    uint32_t i_size;    /* 文件大小，单位是 byte */
    uint32_t i_atime;   /* Access time */
    uint32_t i_ctime;   /* Creation time */
    uint32_t i_mtime;   /* Modification time */
    uint32_t i_dtime;   /* Deletion Time */
    uint16_t i_gid;     /* Low 16 bits of Group Id */
    uint16_t i_links_count;          /* Links count */
    uint32_t i_sectors_count;               /* disk sectors 计数 */
    uint32_t i_flags;                /* File flags */
    uint32_t l_i_reserved1;          /* 可以忽略 */
/*0x40*/    uint32_t i_block[EXT2_SIND_BLOCK]; /* 一组 block 指针 */
    uint32_t i_singly_block;
    uint32_t i_doubly_block;
    uint32_t i_triply_block;
    uint32_t i_generation;           /* 可以忽略 */
    uint32_t i_file_acl;             /* 可以忽略 */
    uint32_t i_dir_acl;              /* 可以忽略 */
    uint32_t i_faddr;                /* 可以忽略 */
    uint8_t l_i_frag;               /* 可以忽略 */
    uint8_t l_i_fsize;              /* 可以忽略 */
    uint16_t i_pad1;                 /* 可以忽略 */
    uint16_t l_i_uid_high;           /* 可以忽略 */
    uint16_t l_i_gid_high;           /* 可以忽略 */
    uint32_t l_i_reserved2;          /* 可以忽略 */
} ext2_inode_t;
typedef struct {
    uint32_t inode_id;
    uint16_t size;
    uint8_t name_length;
    /*(only if the feature bit for "directory entries have file type byte" is set, else this is the most-significant 8 bits of the Name Length)*/
    uint8_t type_indicator;
    uint8_t name[];
} ext2_dir_t;

typedef struct {
    ext2_super_block_t super_blk;
    blk_dev_t *dev;
    uint32_t block_group_count;
    ext2_group_desc_t *block_group;
    uint32_t sections_per_group;
    uint32_t block_size;
    uint32_t fragment_size;
} ext2_t;
typedef struct {
    ext2_t *fs;
    ext2_inode_t *inode;
    uint32_t next_block_id;
    uint8_t *block;
    ext2_dir_t *cur_dir;
} ext2_dir_iterator_t;
#define BIT_GET(x, index) (((x)>>(index))&1)
#define BIT_SET(x, index) ((x)|=1<<(index))
#define BIT_CLEAR(x, index) ((x)^=1<<(index))

void ext2_init(blk_dev_t *dev);

#endif //W2_EXT2_H
