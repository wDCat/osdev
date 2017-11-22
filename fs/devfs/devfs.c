//
// Created by dcat on 11/11/17.
//

#include <catmfs.h>
#include <str.h>
#include <vfs.h>
#include <mutex.h>
#include <kmalloc.h>
#include <devfs.h>
#include <proc.h>
#include "devfs.h"


bool __mounted = false;
devfs_special_t devfsp;

int devfs_register_dev(const char *name, file_operations_t *fops, void *extern_data) {
    return catmfs_make_overlay_file(devfsp.catfsp, (catmfs_inode_t *) devfsp.rootnode->inode,
                                    name, fops, extern_data);
}

int devfs_unregister_dev(const char *name) {
    fs_node_t node;
    CHK(catmfs_fs_node_finddir(devfsp.rootnode, devfsp.catfsp, name, &node), "");
    CHK(catmfs_fs_node_rm(&node, devfsp.catfsp), "");
    return 0;
    _err:
    deprintf("failed to unregister device.");
    return 1;
}

int32_t devfs_fs_node_read(fs_node_t *node, __fs_special_t *fsp_, uint32_t size, uint8_t *buff) {
    __fs_special_t *catfsp = ((devfs_special_t *) fsp_)->catfsp;
    return catmfs_fs_node_read(node, catfsp, size, buff);

}

int32_t devfs_fs_node_write(fs_node_t *node, __fs_special_t *fsp_, uint32_t size, uint8_t *buff) {
    __fs_special_t *catfsp = ((devfs_special_t *) fsp_)->catfsp;
    return catmfs_fs_node_write(node, catfsp, size, buff);
}

int devfs_fs_node_finddir(fs_node_t *node, __fs_special_t *fsp_, const char *name, fs_node_t *result_out) {
    return catmfs_fs_node_finddir(node, ((devfs_special_t *) fsp_)->catfsp, name, result_out);
}

int32_t devfs_fs_node_readdir(fs_node_t *node, __fs_special_t *fsp_, uint32_t count, struct dirent *result) {
    return catmfs_fs_node_readdir(node, ((devfs_special_t *) fsp_)->catfsp, count, result);
}

int devfs_fs_node_lseek(fs_node_t *node, __fs_special_t *fsp_, uint32_t offset) {
    node->offset = offset;
    return 0;
}

uint32_t devfs_fs_node_tell(fs_node_t *node, __fs_special_t *fsp_) {
    return node->offset;
}

__fs_special_t *devfs_fs_node_mount(void *dev, fs_node_t *node) {
    if (__mounted) {
        deprintf("/dev already mounted.");
        return NULL;
    }
    devfsp.catfsp = catmfs_fs_node_mount(NULL, node);
    if (devfsp.catfsp == NULL)goto _err;
    devfsp.rootnode = node;
    __mounted = true;
    return &devfsp;
    _err:
    return NULL;
}

int devfs_fs_node_symlink(struct fs_node *node, __fs_special_t *fsp_,
                          const char *target) {
    __fs_special_t *catfsp = ((devfs_special_t *) fsp_)->catfsp;
    return catmfs_fs_node_symlink(node, catfsp, target);
}

int devfs_fs_node_readlink(fs_node_t *node, __fs_special_t *fsp_,
                           char *buff, int max_len) {
    __fs_special_t *catfsp = ((devfs_special_t *) fsp_)->catfsp;
    int ret = catmfs_fs_node_readlink(node, catfsp, buff, max_len);
    if (!ret) {
        if (str_compare(buff, DEV_STDIN_SYMLINK_STUB)
            || str_compare(buff, DEV_STDOUT_SYMLINK_STUB)
            || str_compare(buff, DEV_STDERR_SYMLINK_STUB)) {
            pcb_t *pcb = getpcb(getpid());
            strformat(buff, "/dev/tty%d", pcb->tty_no);
            return 0;
        }
    }
    return ret;

}

int devfs_fs_node_ioctl(fs_node_t *node, __fs_special_t *fsp_,
                        unsigned int cmd, unsigned long arg) {
    __fs_special_t *catfsp = ((devfs_special_t *) fsp_)->catfsp;
    return catmfs_fs_node_ioctl(node, catfsp, cmd, arg);
}

int devfs_after_vfs_inited() {
    CHK(catmfs_fast_symlink(devfsp.rootnode, devfsp.catfsp, "stdin", DEV_STDIN_SYMLINK_STUB), "");
    CHK(catmfs_fast_symlink(devfsp.rootnode, devfsp.catfsp, "stdout", DEV_STDOUT_SYMLINK_STUB), "");
    CHK(catmfs_fast_symlink(devfsp.rootnode, devfsp.catfsp, "stderr", DEV_STDERR_SYMLINK_STUB), "");
    return 0;
    _err:
    return -1;
}

fs_t devfs = {
        .name="devfs",
        .mount = devfs_fs_node_mount,
        .read = devfs_fs_node_read,
        .write = devfs_fs_node_write,
        .finddir = devfs_fs_node_finddir,
        .readdir = devfs_fs_node_readdir,
        .lseek = devfs_fs_node_lseek,
        .tell = devfs_fs_node_tell,
        .symlink = devfs_fs_node_symlink,
        .readlink = devfs_fs_node_readlink,
        .ioctl = devfs_fs_node_ioctl
};

void devfs_create_fstype() {
}