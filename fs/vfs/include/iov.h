//
// Created by dcat on 11/7/17.
//

#ifndef W2_IOV_H
#define W2_IOV_H

#include "stdint.h"

#define IOV_MAX 1024
typedef struct iovec {
    void *iov_base;
    size_t iov_len;
} iovec_t;

ssize_t sys_readv(int fd, const struct iovec *iov, int iovcnt);

ssize_t sys_writev(int fd, const struct iovec *iov, int iovcnt);

#endif //W2_IOV_H
