//
// Created by dcat on 11/7/17.
//

#include <vfs.h>
#include <proc.h>
#include <str.h>
#include "iov.h"

ssize_t sys_readv(int fd, const struct iovec *iov, int iovcnt) {
    if (iovcnt > IOV_MAX)return -1;
    ssize_t ret = 0;
    for (int x = 0; x < iovcnt; x++) {
        ssize_t s = kread(getpid(), fd, (int32_t) iov[x].iov_len, iov[x].iov_base);
        if (s == -1)return ret;
        ret += s;
    }
    return ret;
}

ssize_t sys_writev(int fd, const struct iovec *iov, int iovcnt) {
    if (iovcnt > IOV_MAX)return -1;
    ssize_t ret = 0;
    for (int x = 0; x < iovcnt; x++) {
        ssize_t s = kwrite(getpid(), fd, (int32_t) iov[x].iov_len, iov[x].iov_base);
        if (s == -1)return ret;
        ret += s;
    }
    return ret;
}
