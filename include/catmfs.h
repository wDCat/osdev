//
// Created by dcat on 4/3/17.
//

#ifndef W2_CATMFS_H
#define W2_CATMFS_H

#include "system.h"
#include "kmalloc.h"
#include "fs_node.h"

#define CATMFS_MAGIC 0xCACAACAC
#define CATMFS_DATA_START_MAGIC 0xACCACCDD
typedef struct {
    uint32_t magic;
    uint32_t length;
    uint32_t obj_table_count;
} catmfs_raw_header_t;
struct catmfs_struct;
typedef struct {
    char name[256];
    int flag;
    uint32_t offset;//offset of data_addr;
    uint32_t length;
    fs_node_t *node;
    struct catmfs_struct *fs;
} catmfs_obj_t;
typedef struct {
    catmfs_obj_t objects[32];
    uint32_t count;
} catmfs_obj_table_t;
typedef struct catmfs_struct {
    fs_node_t root;
    catmfs_raw_header_t *header;
    catmfs_obj_table_t *tables;
    uint32_t data_addr;
    uint32_t table_count;
} catmfs_t;

catmfs_t *catmfs_init(uint32_t mem_addr);

uint32_t catmfs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff);

uint32_t catmfs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buff);

#endif //W2_CATMFS_H
