//
// Created by dcat on 9/16/17.
//

#include <vfs.h>
#include <str.h>
#include <proc.h>
#include <kmalloc.h>
#include <procfs.h>
#include <catmfs.h>
#include "procfs.h"

fs_t procfs;
procfs_special_t psp;
procfs_item_t procfs_items[256];
uint8_t procfs_items_count = 0;

int procfs_fs_node_make(fs_node_t *node, __fs_special_t *fsp_, uint8_t type, char *name) {
    return 1;
}

int procfs_insert_proc(int pid) {
    fs_node_t *rnode = *psp.catrnode;
    fs_node_t tnode;
    char fn[10];
    strformat(fn, "%d", pid);
    CHK(catmfs_fs_node_make(rnode, psp.catfsp, FS_DIRECTORY, fn), "");
    CHK(catmfs_fs_node_finddir(rnode, psp.catfsp, fn, &tnode), "proc dir not found");
    for (int x = 0; x < procfs_items_count; x++) {
        procfs_item_t *item = &procfs_items[x];
        CHK(catmfs_fs_node_make(&tnode, psp.catfsp, FS_FILE, item->fn), "");
    }
    return 0;
    _err:
    return 1;
}

int procfs_remove_proc(int pid) {
    fs_node_t *rnode = *psp.catrnode;
    char fn[10];
    fs_node_t tnode;
    itos(pid, fn);
    CHK(catmfs_fs_node_finddir(rnode, psp.catfsp, fn, &tnode), "proc dir not found");
    CHK(catmfs_fs_node_rm(&tnode, psp.catfsp), "");
    return 0;
    _err:
    return 1;
}

int procfs_insert_item(const char *fn, procfs_precall_t precall) {
    procfs_item_t *item = &procfs_items[procfs_items_count];
    strcpy(item->fn, fn);
    item->precall = precall;
    procfs_items_count++;
}

int32_t procfs_fs_node_read(fs_node_t *node, __fs_special_t *fsp_, uint32_t offset, uint32_t size, uint8_t *buff) {
}

int32_t procfs_fs_node_write(fs_node_t *node, __fs_special_t *fsp_, uint32_t offset, uint32_t size, uint8_t *buff) {
    return -1;
}

int procfs_fs_node_finddir(fs_node_t *node, __fs_special_t *fsp_, const char *name, fs_node_t *result_out) {
    dprintf("neko try to find %s.", name);
    return catmfs_fs_node_finddir(node, ((procfs_special_t *) fsp_)->catfsp, name, result_out);
}

int32_t procfs_fs_node_readdir(fs_node_t *node, __fs_special_t *fsp_, uint32_t count, struct dirent *result) {
    return catmfs_fs_node_readdir(node, ((procfs_special_t *) fsp_)->catfsp, count, result);
}

__fs_special_t *procfs_fs_node_mount(void *dev, fs_node_t *node) {
    psp.catfsp = catmfs_fs_node_mount(NULL, node);
    *psp.catrnode = node;
    procfs_insert_proc(2);
    return (void *) &psp;
}

void procfs_create_type() {
    memset(procfs_items, 0, sizeof(procfs_items));
    memset(&procfs, 0, sizeof(fs_t));
    strcpy(procfs.name, "PROCFS_TEST");
    procfs.mount = procfs_fs_node_mount;
    procfs.make = procfs_fs_node_make;
    procfs.readdir = procfs_fs_node_readdir;
    procfs.finddir = procfs_fs_node_finddir;
    procfs.read = procfs_fs_node_read;
    procfs.write = procfs_fs_node_write;
    procfs_insert_item("status", NULL);
    procfs_insert_item("cmdline", NULL);
}