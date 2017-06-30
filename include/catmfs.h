//
// Created by dcat on 4/3/17.
//

#ifndef W2_CATMFS_H
#define W2_CATMFS_H

#include "system.h"
#include "kmalloc.h"
#include "fs_node.h"

#include "catmfs_def.h"

catmfs_t *catmfs_init(uint32_t mem_addr);

uint32_t catmfs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff);

uint32_t catmfs_read_inner(catmfs_obj_t *fs_obj, uint32_t offset, uint32_t size, uint8_t *buff);

uint32_t catmfs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff);

uint32_t catmfs_write_inner(catmfs_obj_t *fs_obj, uint32_t offset, uint32_t size, uint8_t *buff);

fs_node_t *catmfs_create_fs_node(catmfs_obj_t *fs_obj);

/**
 * @return 1->found 0->not found -1->error
 * */
uint8_t catmfs_findbyname(catmfs_t *fs, char *fn, fs_node_t **fs_node_out);

void catmfs_dumpfilelist(catmfs_t *fs);

#endif //W2_CATMFS_H
