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

int procfs_insert_proc(pid_t pid) {
    fs_node_t *rnode = *psp.catrnode;
    fs_node_t tnode;
    char fn[10];
    strformat(fn, "%d", pid);
    CHK(catmfs_fs_node_make(rnode, psp.catfsp, FS_DIRECTORY, fn), "");
    CHK(catmfs_fs_node_finddir(rnode, psp.catfsp, fn, &tnode), "proc dir not found");
    for (int x = 0; x < procfs_items_count; x++) {
        fs_node_t item_node;
        procfs_item_t *item = &procfs_items[x];
        dprintf("creating item:%s", item->fn);
        CHK(catmfs_fs_node_make(&tnode, psp.catfsp, FS_FILE, item->fn), "");
        CHK(catmfs_fs_node_finddir(&tnode, psp.catfsp, item->fn, &item_node), "ERR");
        procfs_snode_t snode = {
                .id=x,
                .pid=pid
        };
        catmfs_fs_node_lseek(&item_node, psp.catfsp, 0);
        if (catmfs_fs_node_write(&item_node, psp.catfsp, sizeof(snode), &snode) != sizeof(snode)) {
            PANIC("fail to write file...");
        }
    }
    return 0;
    _err:
    return 1;
}

int procfs_remove_proc(pid_t pid) {
    dprintf("removing %x", pid);
    fs_node_t *rnode = *psp.catrnode;
    char fn[10];
    fs_node_t tnode, item_node;
    strformat(fn, "%d", pid);
    CHK(catmfs_fs_node_finddir(rnode, psp.catfsp, fn, &tnode), "proc dir not found");
    for (int x = 0; x < procfs_items_count; x++) {
        procfs_item_t *item = &procfs_items[x];
        dprintf("removing item:%s", item->fn);
        CHK(catmfs_fs_node_finddir(&tnode, psp.catfsp, item->fn, &item_node), "ERR");
        catmfs_fs_node_rm(&item_node, psp.catfsp);
    }
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

int32_t procfs_fs_node_read(fs_node_t *node, __fs_special_t *fsp_, uint32_t size, uint8_t *buff) {
    uint32_t offset = node->offset;
    __fs_special_t *catfsp = ((procfs_special_t *) fsp_)->catfsp;
    procfs_snode_t snode;
    catmfs_fs_node_lseek(node, catfsp, 0);
    if (catmfs_fs_node_read(node, catfsp, sizeof(snode), &snode) != sizeof(snode)) {
        deprintf("Fail to read snode");
        return -1;
    }
    if (snode.id < 256 && procfs_items[snode.id].precall) {
        dprintf("calling precall:%x for %s", procfs_items[snode.id].precall, procfs_items[snode.id].fn);
        procfs_items[snode.id].precall(node, catfsp, &snode);
    }
    catmfs_fs_node_lseek(node, catfsp, sizeof(snode) + offset);
    return catmfs_fs_node_read(node, catfsp, size, buff);
}

int32_t procfs_fs_node_write(fs_node_t *node, __fs_special_t *fsp_, uint32_t size, uint8_t *buff) {
    return -1;
}

int procfs_fs_node_finddir(fs_node_t *node, __fs_special_t *fsp_, const char *name, fs_node_t *result_out) {
    return catmfs_fs_node_finddir(node, ((procfs_special_t *) fsp_)->catfsp, name, result_out);
}

int32_t procfs_fs_node_readdir(fs_node_t *node, __fs_special_t *fsp_, uint32_t count, struct dirent *result) {
    return catmfs_fs_node_readdir(node, ((procfs_special_t *) fsp_)->catfsp, count, result);
}

__fs_special_t *procfs_fs_node_mount(void *dev, fs_node_t *node) {
    psp.catfsp = catmfs_fs_node_mount(NULL, node);
    *psp.catrnode = node;
    return (void *) &psp;
}

int procfs_item_status(fs_node_t *node, __fs_special_t *fsp_, procfs_snode_t *snode) {

    pcb_t *pcb = getpcb(snode->pid);
    char *status;
    char buff[256];
    switch (pcb->status) {
        case STATUS_RUN:
            status = "RUN";
            break;
        case STATUS_WAIT:
            status = "WAIT";
            break;
        case STATUS_READY:
            status = "READY";
            break;
        case STATUS_DIED:
            status = "DIED";
    }
    catmfs_fs_node_lseek(node, fsp_, sizeof(procfs_snode_t));
    strformat(buff, "Name:\t%s\n"
            "Status:\t%s\n"
            "Pid:\t%x\n", pcb->name, status, snode->pid);
    catmfs_fs_node_write(node, fsp_, strlen(buff), buff);
    return 0;
}

int procfs_item_cmdline(fs_node_t *node, __fs_special_t *fsp_, procfs_snode_t *snode) {
    pcb_t *pcb = getpcb(snode->pid);
    catmfs_fs_node_lseek(node, fsp_, sizeof(procfs_snode_t));
    catmfs_fs_node_write(node, fsp_, strlen(pcb->cmdline), pcb->cmdline);
    return 0;
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
    procfs_insert_item("status", procfs_item_status);
    procfs_insert_item("cmdline", procfs_item_cmdline);
}