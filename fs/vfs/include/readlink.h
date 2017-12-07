//
// Created by dcat on 12/4/17.
//

#ifndef W2_READLINK_H
#define W2_READLINK_H

#include <stdint.h>
#include "uproc.h"

ssize_t kreadlinkat(pid_t pid, int dirfd, const char *pathname,
                    char *buf, size_t bufsiz);

ssize_t sys_readlinkat(int dirfd, const char *pathname,
                       char *buf, size_t bufsiz);

ssize_t sys_readlink(const char *pathname, char *buf, size_t bufsiz);

#endif //W2_READLINK_H
