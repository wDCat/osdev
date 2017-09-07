//
// Created by dcat on 9/7/17.
//

#ifndef W2_UVFS_H
#define W2_UVFS_H

#include "stdint.h"

typedef struct dirent {
    uint32_t node;
    unsigned short name_len; /* length of this d_name 文件名长 */
    unsigned char type; /* the type of d_name 文件类型 */
    char name[256]; /* file name (null-terminated) 文件名，最长255字符 */
} dirent_t;
#endif //W2_UVFS_H
