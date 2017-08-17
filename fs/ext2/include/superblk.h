//
// Created by dcat on 8/17/17.
//

#ifndef W2_SUPERBLK_H
#define W2_SUPERBLK_H

#include "intdef.h"

typedef struct {
    uint32_t s_inodes_count;      /* inodes 计数 */
    uint32_t s_blocks_count;      /* blocks 计数 */
    uint32_t s_r_blocks_count;    /* 保留的 blocks 计数 */
    uint32_t s_free_blocks_count; /* 空闲的 blocks 计数 */
    uint32_t s_free_inodes_count; /* 空闲的 inodes 计数 */
    uint32_t s_first_data_block;  /* 第一个数据 block */
    uint32_t s_block_size;    /* block 的大小 */
    int32_t s_fragment_size;     /* 可以忽略 */
    uint32_t s_blocks_per_group;  /* 每 block group 的 block 数量 */
    uint32_t s_frags_per_group;   /* 可以忽略 */
    uint32_t s_inodes_per_group;  /* 每 block group 的 inode 数量 */
    uint32_t s_mtime;             /* Mount time */
    uint32_t s_wtime;             /* Write time */
    uint16_t s_mnt_count;         /* Mount count */
    int16_t s_max_mnt_count;     /* Maximal mount count */
    uint16_t s_magic;             /* Magic 签名 */
    uint16_t s_state;             /* File system state */
    uint16_t s_errors;            /* Behaviour when detecting errors */
    uint16_t s_minor_rev_level;   /* minor revision level */
    uint32_t s_lastcheck;         /* time of last check */
    uint32_t s_checkinterval;     /* max. time between checks */
    uint32_t s_creator_os;        /* 可以忽略 */
    uint32_t s_rev_level;         /* Revision level */
    uint16_t s_def_resuid;        /* Default uid for reserved blocks */
    uint16_t s_def_resgid;        /* Default gid for reserved blocks */
    uint32_t s_first_ino;         /* First non-reserved inode */
    uint16_t s_inode_size;        /* size of inode structure */
    uint16_t s_block_group_nr;    /* block group # of this superblock */
    uint32_t s_feature_compat;    /* compatible feature set */
    uint32_t s_feature_incompat;  /* incompatible feature set */
    uint32_t s_feature_ro_compat; /* readonly-compatible feature set */
    uint8_t s_uuid[16];          /* 128-bit uuid for volume */
    char s_volume_name[16];   /* volume name */
    char s_last_mounted[64];  /* directory where last mounted */
    uint32_t s_algorithm_usage_bitmap; /* 可以忽略 */
    uint8_t s_prealloc_blocks;        /* 可以忽略 */
    uint8_t s_prealloc_dir_blocks;    /* 可以忽略 */
    uint16_t s_padding1;               /* 可以忽略 */
    uint8_t s_journal_uuid[16]; /* uuid of journal superblock */
    uint32_t s_journal_inum;     /* 日志文件的 inode 号数 */
    uint32_t s_journal_dev;      /* 日志文件的设备号 */
    uint32_t s_last_orphan;      /* start of list of inodes to delete */
    uint32_t s_reserved[197];    /* 可以忽略 */
} ext2_super_block_t;

#endif //W2_SUPERBLK_H
