//
// Created by dcat on 11/30/17.
//

#include <system.h>
#include <str.h>
#include <vfs.h>
#include <kmalloc.h>
#include <proc.h>
#include "open.h"

int8_t kopen(uint32_t pid, const char *name, int flags) {
    pcb_t *pcb = getpcb(pid);
    char *path = (char *) kmalloc(MAX_PATH_LEN);
    vfs_fix_path(pid, name, path, MAX_PATH_LEN);
    vfs_pretty_path(path, NULL);
    mount_point_t *mp;
    int8_t fd = -1;
    for (int8_t x = 0; x < MAX_FILE_HANDLES; x++) {
        if (!pcb->fh[x].present) {
            fd = x;
            break;
        }
    }
    if (fd == -1) {
        deprintf("cannot open more than %x files.", MAX_FILE_HANDLES);
        goto _ret;
    }
    file_handle_t *fh = &pcb->fh[fd];
    if (vfs_get_node(&pcb->vfs, path, &fh->node)) {
        if (!(flags & O_CREAT)) {
            deprintf("no such file or dir:%s type:%d", name, flags);
            fd = -1;
            goto _ret;
        } else {
            int x = strlen(path) - 2;
            while (path[x] != '/' && x >= 0)x--;
            if (x == 0 && path[0] != '/') {
                deprintf("Bad path:%s", path);
                fd = -1;
                goto _ret;
            }
            char origchr = path[x + 1];
            path[x + 1] = 0;
            vfs_cd(&pcb->vfs, path);
            path[x + 1] = origchr;
            dprintf("path:%s fn:%s", path, &path[x + 1]);
            if (vfs_make(&pcb->vfs, FS_FILE, &path[x + 1]) || vfs_get_node(&pcb->vfs, path, &fh->node)) {
                deprintf("cannot make new file:%s", path);
                fd = -1;
                goto _ret;
            }
        }
    }
    mp = vfs_get_mount_point(&pcb->vfs, &fh->node);
    if (mp == NULL) {
        deprintf("mount point not found:%s", path);
        fd = -1;
        goto _ret;
    }

    fh->present = 1;
    fh->flags = flags;
    fh->mp = mp;
    fh->node.offset = 0;
    if (mp->fs->open && mp->fs->open(fh)) {
        deprintf("fs's open callback failed.");
        goto _ret;
    }
    dprintf("proc %x open %s fd:%x", pid, path, fd);
    _ret:
    kfree(path);
    return fd;
}

int8_t sys_open(const char *name, int flags) {
    return kopen(getpid(), name, flags);
}

int8_t kclose(uint32_t pid, int8_t fd) {
    pcb_t *pcb = getpcb(pid);
    file_handle_t *fh = &pcb->fh[fd];
    if (!fh->present) {
        dwprintf("cannot close a closed file:%x", fd);
        return -1;
    }
    mount_point_t *mp = fh->mp;
    if (mp->fs->close && mp->fs->close(fh)) {
        deprintf("fs's close callback failed.");
        return -1;
    }
    fh->present = 0;
    fh->mp = NULL;
    return 0;
}

int8_t sys_close(int8_t fd) {
    return kclose(getpid(), fd);
}

void kclose_all(uint32_t pid) {
    pcb_t *pcb = getpcb(pid);
    for (int8_t x = 0; x < MAX_FILE_HANDLES; x++) {
        if (pcb->fh[x].present) {
            pcb->fh[x].present = 0;
            mount_point_t *mp = pcb->fh[x].mp;
            if (mp->fs->close && mp->fs->close(&pcb->fh[x])) {
                deprintf("fs's close callback failed.");
            }
        }
    }
}