//
// Created by dcat on 11/20/17.
//

#include "ioctl.h"
#include <system.h>
#include <str.h>
#include <errno.h>

int sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg) {
    dprintf("ioctl called.fd:%x cmd:%x", fd, cmd);
    return -EINVAL;
};