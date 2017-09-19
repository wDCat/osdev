//
// Created by dcat on 8/18/17.
//

#ifndef W2_BLK_DEV_H
#define W2_BLK_DEV_H

#include "intdef.h"

typedef int(*blk_dev_section_read)(uint32_t offset, uint32_t len, uint8_t *buff);

typedef int(*blk_dev_section_write)(uint32_t offset, uint32_t len, uint8_t *buff);

typedef struct {
    char name[256];
    blk_dev_section_read read;
    blk_dev_section_write write;
} blk_dev_t;
extern blk_dev_t disk1, swap_disk;

void blk_dev_install();

#endif //W2_BLK_DEV_H
