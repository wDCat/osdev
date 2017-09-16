//
// Created by dcat on 9/16/17.
//

#ifndef W2_PROCFS_H
#define W2_PROCFS_H
#define PROCFS_INODE_ROOT 0
#define PROCFS_PROC 1

#include "catmfs.h"

extern fs_t procfs;
typedef struct {
    catmfs_special_t *catfsp;
    fs_node_t **catrnode;
} procfs_special_t;
typedef struct {

} procfs_snode_t;

typedef int (*procfs_precall_t)(procfs_snode_t *);

typedef struct {
    char fn[256];
    procfs_precall_t precall;
} procfs_item_t;

void procfs_create_type();

#endif //W2_PROCFS_H
