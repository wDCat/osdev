//
// Created by dcat on 9/7/17.
//

#ifndef W2_USTAT_H
#define W2_USTAT_H

#include "stdint.h"
#include "def.h"

typedef struct stat {
    dev_t st_dev;     /* ID of device containing file */
    ino_t st_ino;     /* inode number */
    mode_t st_mode;    /* protection */
    nlink_t st_nlink;   /* number of hard links */
    uid_t st_uid;     /* user ID of owner */
    gid_t  st_gid;     /* group ID of owner */
    dev_t st_rdev;    /* device ID (if special file) */
    off_t st_size;    /* total size, in bytes */
    blksize_t st_blksize; /* blocksize for file system I/O */
    blkcnt_t st_blocks;  /* number of 512B blocks allocated */
    uint32_t st_atime;   /* time of last access */
    uint32_t st_mtime;   /* time of last modification */
    uint32_t st_ctime;   /* time of last status change */
} stat_t;

#define stat64_t stat_t
#endif //W2_USTAT_H
