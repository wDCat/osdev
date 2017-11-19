//
// Created by dcat on 9/7/17.
//

#ifndef W2_UVFS_H
#define W2_UVFS_H

#include "stdint.h"
//Test
#define    R_OK    4        /* Test for read permission.  */
#define    W_OK    2        /* Test for write permission.  */
#define    X_OK    1        /* Test for execute permission.  */
#define    F_OK    0        /* Test for existence.  */
typedef int32_t off_t;
typedef struct dirent {
    uint32_t node;
    unsigned short name_len; /* size of this d_name 文件名长 */
    unsigned char type; /* the type of d_name 文件类型 */
    char name[256]; /* file name (null-terminated) 文件名，最长255字符 */
} dirent_t;
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR        0x0002
#define O_NDELAY    0x0004
#define O_APPEND    0x0008
#define O_CREAT        0x0200
#endif //W2_UVFS_H
