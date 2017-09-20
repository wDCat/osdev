//
// Created by dcat on 9/18/17.
//

#ifndef W2_SWAP_H
#define W2_SWAP_H

#include "intdef.h"
#include "proc.h"

#define MAX_SWAPPED_OUT_PAGE 64
#define SPAGE_TYPE_UNUSED 0x0
#define SPAGE_TYPE_SOUT 0x2
#define SPAGE_TYPE_PLOAD 0x4
#define SPAGE_TYPE_EMPTY 0x8
#define SPAGE_MARK_IN (1<<5)
typedef struct spage_info {
    uint32_t addr;
    uint8_t type;
    int8_t fd;
    uint32_t offset;
    //uint32_t in_offset; = 0
    //uint32_t size; = PAGE_SIZE
} spage_info_t;

int swap_out(pcb_t *pcb, uint32_t addr);

int swap_insert_pload_page(pcb_t *pcb, uint32_t addr, int8_t fd, uint32_t offset);

int swap_handle_page_fault(regs_t *r);

int swap_insert_empty_page(pcb_t *pcb, uint32_t addr);

void swap_install();

#endif //W2_SWAP_H
