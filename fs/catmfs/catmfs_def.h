//
// Created by dcat on 6/30/17.
//

#ifndef W2_CATMFS_DEF_H
#define W2_CATMFS_DEF_H


#define CATMFS_MAGIC 0xCACAACAC
#define CATMFS_DATA_START_MAGIC 0xAC2508DC
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
#endif //W2_CATMFS_DEF_H
