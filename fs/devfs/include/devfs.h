//
// Created by dcat on 11/11/17.
//

#ifndef W2_DEVFS_H
#define W2_DEVFS_H

#include "catmfs.h"

#define DEV_STDIN_SYMLINK_STUB "stdin_stub!"
#define DEV_STDOUT_SYMLINK_STUB "stdout_stub!"
#define DEV_STDERR_SYMLINK_STUB "stderr_stub!"
extern fs_t devfs;
typedef struct {
    catmfs_special_t *catfsp;
    fs_node_t *rootnode;
} devfs_special_t;

void devfs_create_fstype();

int devfs_after_vfs_inited();

int devfs_register_dev(const char *name, file_operations_t *fops, void *extern_data);

int devfs_unregister_dev(const char *name);

#endif //W2_DEVFS_H
