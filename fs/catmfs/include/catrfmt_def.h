//
// Created by dcat on 6/30/17.
//

#ifndef W2_CATMFS_DEF_H
#define W2_CATMFS_DEF_H


#define CATRFMT_MAGIC 0xCACAACAC
#define CATRFMT_DATA_START_MAGIC 0xAC2508DC
typedef struct {
    uint32_t magic;
    uint32_t length;
    uint32_t obj_table_count;
} catrfmt_raw_header_t;
struct catrfmt_struct;
typedef struct {
    char name[256];
    int flag;
    uint32_t offset;//offset of data_addr;
    uint32_t length;
    fs_node_t *node;
    struct catrfmt_struct *fs;
} catrfmt_obj_t;
typedef struct {
    catrfmt_obj_t objects[32];
    uint32_t count;
} catrfmt_obj_table_t;
typedef struct catrfmt_struct {
    fs_node_t root;
    catrfmt_raw_header_t *header;
    catrfmt_obj_table_t *tables;
    uint32_t data_addr;
    uint32_t table_count;
} catrfmt_t;
#endif //W2_CATMFS_DEF_H
