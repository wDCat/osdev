//
// Created by dcat on 12/1/17.
//

#ifndef W2_UGETDENTS_H
#define W2_UGETDENTS_H

#include "uvfs.h"

struct linux_dirent {
    ino_t d_ino;    /* 64-bit inode number */
    off_t d_off;    /* 64-bit offset to next structure */
    unsigned short d_reclen; /* Size of this dirent */
    unsigned char d_type;   /* File type */
    char d_name[MAX_FILENAME_LEN]; /* Filename (null-terminated) */
};
#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12
#define DT_WHT 14
#define IFTODT(x) ((x)>>12 & 017)
#define DTTOIF(x) ((x)<<12)
#define linux_dirent64 linux_dirent
#endif //W2_UGETDENTS_H
