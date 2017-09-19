//
// Created by dcat on 9/19/17.
//

#ifndef W2_SWAP_DISK_H
#define W2_SWAP_DISK_H

#include "intdef.h"
#include "proc.h"
#include "blk_dev.h"

typedef struct {
    blk_dev_t *dev;
    uint8_t *bitset;
    uint32_t max_page_count;
    uint32_t p;
} swap_disk_t;

int swap_disk_init(swap_disk_t *sdisk, blk_dev_t *dev, uint32_t max_page_count);

uint32_t swap_disk_put(swap_disk_t *sdisk, pcb_t *pcb, uint32_t addr);

int swap_disk_get(swap_disk_t *sdisk, pcb_t *pcb, uint32_t addr, uint32_t id);

int swap_disk_clear(swap_disk_t *sdisk, uint32_t id);

#endif //W2_SWAP_DISK_H
