//
// Created by dcat on 11/26/17.
//

#include <vfs.h>
#include <str.h>
#include <proc.h>
#include <kmalloc.h>
#include <errno.h>
#include "stat.h"

int sys_stat(const char *name, stat_t *stat) {
    return -ENOSYS;//debug..
    char *path = (char *) kmalloc(MAX_PATH_LEN);
    vfs_fix_path(getpid(), name, path, MAX_PATH_LEN);
    CHK(vfs_cd(&vfs, path), "No such file or directory.");
    fs_node_t *n = &vfs.current;
    stat->st_dev = 30;
    stat->st_ino = n->inode;
    stat->st_mode = n->mode;
    stat->st_nlink = 1;
    stat->st_uid = n->uid;
    stat->st_gid = n->gid;
    stat->st_rdev = 0;
    stat->st_size = n->size;
    stat->st_blksize = 0;
    stat->st_blocks = 0;
    stat->st_atime = 0;
    stat->st_mtime = 0;
    stat->st_ctime = 0;
    kfree(path);
    dprintf("stat done.");
    return 0;
    _err:
    kfree(path);
    return -1;
}

int sys_stat64(const char *name, stat64_t *stat) {
    int ret = -EINVAL;
    char *path = (char *) kmalloc(MAX_PATH_LEN);
    vfs_fix_path(getpid(), name, path, MAX_PATH_LEN);
    vfs_pretty_path(path, NULL);
    if (vfs_cd(&vfs, path)) {
        dwprintf("No such file or directory:%s", name);
        ret = -ENOENT;
        goto _err;
    }

    memset(stat, 0xFF, sizeof(stat64_t));
    fs_node_t *n = &vfs.current;
    stat->st_dev = 20;
    stat->st_ino = n->inode;
    stat->st_mode = n->mode;
    dprintf("debug:::st64:%d",n->mode);
    stat->st_nlink = 1;
    stat->st_uid = n->uid;
    stat->st_gid = n->gid;
    stat->st_rdev = 0;
    stat->st_size = n->size;
    stat->st_blksize = 0;
    stat->st_blocks = 0;
    stat->st_atime = 0;
    stat->st_mtime = 0;
    stat->st_ctime = 0;
    kfree(path);
    return 0;
    _err:
    kfree(path);
    return ret;
}