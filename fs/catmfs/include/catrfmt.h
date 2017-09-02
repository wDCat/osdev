//
// Created by dcat on 4/3/17.
//

#ifndef W2_CATMFSM_H
#define W2_CATMFSM_H

#include "../../../ker/include/system.h"
#include "../../../memory/include/kmalloc.h"
#include "fs_node.h"

#include "catrfmt_def.h"

catrfmt_t *catrfmt_init(uint32_t mem_addr);

uint32_t catrfmt_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff);

uint32_t catrfmt_read_inner(catrfmt_obj_t *fs_obj, uint32_t offset, uint32_t size, uint8_t *buff);

uint32_t catrfmt_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff);

uint32_t catrfmt_write_inner(catrfmt_obj_t *fs_obj, uint32_t offset, uint32_t size, uint8_t *buff);

fs_node_t *catrfmt_create_fs_node(catrfmt_obj_t *fs_obj);

/**
 * @return 1->found 0->not found -1->error
 * */
uint8_t catrfmt_findbyname(catrfmt_t *fs, char *fn, fs_node_t **fs_node_out);

void catrfmt_dumpfilelist(catrfmt_t *fs);

#endif //W2_CATMFSM_H
