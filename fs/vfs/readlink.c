//
// Created by dcat on 12/4/17.
//

#include <errno.h>
#include <str.h>
#include <vfs.h>
#include "readlink.h"
#include "fcntl.h"

ssize_t kreadlinkat(pid_t pid, int dirfd, const char *pathname,
                    char *buf, size_t bufsiz) {
    if (dirfd > MAX_PATH_LEN || (dirfd < 0 && dirfd != AT_FDCWD))
        return -EINVAL;
    const char *cwd;
    if (dirfd == AT_FDCWD) {
        cwd = kgetcwd(pid);
    } else {
        if (dirfd > MAX_PATH_LEN)return -EBADF;
        file_handle_t *fh = &getpcb(pid)->fh[dirfd];
        if (!fh->present || !fh->path)return -ENOENT;
        cwd = fh->path;
    }
    CHK(vfs_fix_path5(pid, pathname, cwd, buf, bufsiz), "");
    CHK(vfs_pretty_path(buf, NULL), "");
    fs_node_t n;
    CHK(vfs_get_node4(&vfs, buf, &n, false), "");
    mount_point_t *mp = vfs_get_mount_point(&vfs, &n);
    if (mp == NULL)goto _err;
    CHK(mp->fs->readlink(&n, mp->fsp, buf, bufsiz), "");
    dprintf("readlink result:%s", buf);
    return strlen(buf);
    _err:
    return -EFAULT;

}

ssize_t sys_readlinkat(int dirfd, const char *pathname,
                       char *buf, size_t bufsiz) {
    return kreadlinkat(getpid(), dirfd, pathname, buf, bufsiz);
}

ssize_t sys_readlink(const char *pathname, char *buf, size_t bufsiz) {
    return kreadlinkat(getpid(), AT_FDCWD, pathname, buf, bufsiz);
}

