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
    int id;
    pid_t pid;
} procfs_snode_t;

typedef int (*procfs_precall_t)(fs_node_t *node, __fs_special_t *fsp_, procfs_snode_t *);

typedef struct {
    char fn[256];
    procfs_precall_t precall;
} procfs_item_t;

void procfs_create_type();

int procfs_insert_proc(pid_t pid);

int procfs_remove_proc(pid_t pid);

#endif //W2_PROCFS_H
