//
// Created by dcat on 9/7/17.
//

#ifndef W2_STAT_H
#define W2_STAT_H

#include "stdint.h"

typedef struct stat {
    uint32_t st_dev;     /* ID of device containing file */
    uint32_t st_ino;     /* inode number */
    uint32_t st_mode;    /* protection */
    uint32_t st_nlink;   /* number of hard links */
    uint32_t st_uid;     /* user ID of owner */
    uint32_t st_gid;     /* group ID of owner */
    uint32_t st_rdev;    /* device ID (if special file) */
    uint32_t st_size;    /* total size, in bytes */
    uint32_t st_blksize; /* blocksize for file system I/O */
    uint32_t st_blocks;  /* number of 512B blocks allocated */
    uint32_t st_atime;   /* time of last access */
    uint32_t st_mtime;   /* time of last modification */
    uint32_t st_ctime;   /* time of last status change */
} stat_t;
#endif //W2_STAT_H
