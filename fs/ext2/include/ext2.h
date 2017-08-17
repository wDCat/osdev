//
// Created by dcat on 8/17/17.
//

#ifndef W2_EXT2_H
#define W2_EXT2_H

#include "superblk.h"

#define SECTION_SIZE 512
#define    EXT3_NDIR_BLOCKS        12
#define    EXT3_IND_BLOCK            EXT3_NDIR_BLOCKS
#define    EXT3_DIND_BLOCK            (EXT3_IND_BLOCK + 1)
#define    EXT3_TIND_BLOCK            (EXT3_DIND_BLOCK + 1)
#define    EXT3_N_BLOCKS            (EXT3_TIND_BLOCK + 1)

typedef int(*fs_section_read)(uint32_t offset, uint32_t len, uint8_t *buff);

typedef int(*fs_section_write)(uint32_t offset, uint32_t len, uint8_t *buff);

typedef struct {
    uint32_t bg_block_bitmap;      /* block 指针指向 block bitmap */
    uint32_t bg_inode_bitmap;      /* block 指针指向 inode bitmap */
    uint32_t bg_inode_table;       /* block 指针指向 inodes table */
    uint16_t bg_free_blocks_count; /* 空闲的 blocks 计数 */
    uint16_t bg_free_inodes_count; /* 空闲的 inodes 计数 */
    uint16_t bg_used_dirs_count;   /* 目录计数 */
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
    uint32_t i_blocks;               /* blocks 计数 */
    uint32_t i_flags;                /* File flags */
    uint32_t l_i_reserved1;          /* 可以忽略 */
    uint32_t i_block[EXT3_N_BLOCKS]; /* 一组 block 指针 */
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
    ext2_super_block_t super_blk;
    fs_section_read section_read;
    fs_section_write section_write;
    uint32_t block_group_count;
    ext2_group_desc_t *block_group;
} ext2_t;

void ext2_init();

#endif //W2_EXT2_H
