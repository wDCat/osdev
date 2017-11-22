//
// Created by dcat on 11/20/17.
//

#include "ioctl.h"
#include <system.h>
#include <str.h>
#include <proc.h>
#include <errno.h>

int kioctl(pid_t pid, unsigned int fd, unsigned int cmd, unsigned long arg) {
    pcb_t *pcb = getpcb(pid);
    file_handle_t *fh = &pcb->fh[fd];
    if (!fh->present) {
        dwprintf("fd %x not present.", fd);
        return -1;
    }
    if (fh->mp == 0) {
        dwprintf("[%x]mount point not found.", fd);
        return -1;
    }
    if (fh->mp->fs->ioctl == 0) {
        dwprintf("fs operation not implemented");
        return -EINVAL;
    }
    return fh->mp->fs->ioctl(&fh->node, fh->mp->fsp, cmd, arg);
}

int sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg) {
    dprintf("ioctl called.fd:%x cmd:%x", fd, cmd);
    return kioctl(getpid(), fd, cmd, arg);
};