//
// Created by dcat on 9/16/17.
//

#include <vfs.h>
#include <str.h>
#include <proc.h>
#include "procfs.h"

fs_t procfs;

int32_t procfs_print_plist(uint32_t offset, uint32_t count, struct dirent *result) {
    int p = 0;
    for (pid_t x = 1; x < MAX_PROC_COUNT; x++) {
        pcb_t *pcb = getpcb(x);
        if (pcb->present) {
            if (offset) {
                offset--;
                continue;
            }
            dprintf("found proc:%x", pcb->pid);
            itos(pcb->pid, result[p].name);
            result[p].name_len = (unsigned short) strlen(result[p].name);
            result[p].type = FS_DIRECTORY;
            result[p].node = (uint32_t) (x & 0xFFFF + (PROCFS_PROC << 16));
            p++;
        }
    }
    return p + 1;
}

int32_t procfs_fs_node_readdir(fs_node_t *node, __fs_special_t *fsp_, uint32_t count, struct dirent *result) {
    pid_t pid = (pid_t) (node->inode && 0xFFFF);
    uint16_t type = (uint16_t) ((node->inode >> 16) & 0xFFFF);
    switch (type) {
        case PROCFS_INODE_ROOT:
            return procfs_print_plist(0, count, result);
        default:
            deprintf("Unknown inode type:%x", type);
            return -1;
    }
}

__fs_special_t *procfs_fs_node_mount(void *dev, fs_node_t *node) {
    node->inode = 0;
    node->flags = FS_DIRECTORY;
    return (void *) 1;
}

void procfs_create_type() {
    memset(&procfs, 0, sizeof(fs_t));
    strcpy(procfs.name, "PROCFS_TEST");
    procfs.mount = procfs_fs_node_mount;
    procfs.readdir = procfs_fs_node_readdir;
}