//
// Created by dcat on 12/1/17.
//

#include <str.h>
#include <proc.h>
#include <errno.h>
#include <vfs.h>
#include "getdents.h"

int sys_ls(const char *path, dirent_t *dirents, uint32_t count) {
    CHK(vfs_cd(&vfs, path), "");
    return vfs_ls(&vfs, dirents, count);
    _err:
    return -1;
}

inline uchar_t get_dirent_type_by_fstype(uchar_t x) {
    switch (x) {
        case FS_DIRECTORY:
            return DT_DIR;
        case FS_FILE:
            return DT_REG;
        case FS_SYMLINK:
            return DT_LNK;
        default:
            return DT_UNKNOWN;
    }
}

int kgetdents(pid_t pid, unsigned int fd, struct linux_dirent *dirp, unsigned int count) {
    pcb_t *pcb = getpcb(pid);
    if (fd > MAX_FILE_HANDLES)return -EINVAL;
    file_handle_t *h = &pcb->fh[fd];
    if (!h->present)return -ENOENT;
    struct linux_dirent *odirp = dirp;
    unsigned int c = count / sizeof(struct linux_dirent);
    while (c) {
        int buffc = MIN(c, 10);
        dirent_t dirs[buffc];
        int cgot = h->mp->fs->readdir(&h->node, h->mp->fsp, buffc, dirs);
        ASSERT(cgot <= buffc);
        if (cgot <= 0)break;
        for (int x = 0; x < cgot; x++) {
            int namelen = MIN(dirs[x].name_len, 255);
            memcpy(dirp->d_name, dirs[x].name, namelen);
            dirp->d_name[namelen] = '\0';
            dirp->d_ino = dirs[x].node;
            dirp->d_off = sizeof(struct linux_dirent);//TODO compress
            dirp->d_reclen = sizeof(struct linux_dirent);
            dirp->d_type = get_dirent_type_by_fstype(dirs[x].type);
            dprintf("write %x o:%x inode:%x name:%s", dirp, dirp->d_name,
                    dirp->d_ino, dirp->d_name);
            dirp++;
        }
        c -= cgot;
    }
    return (int) ((uint32_t) dirp - (uint32_t) odirp);
}

int sys_getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count) {
    return kgetdents(getpid(), fd, dirp, count);
}