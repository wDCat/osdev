//
// Created by dcat on 11/29/17.
//

#include <errno.h>
#include <str.h>
#include "fcntl.h"

int kfcntl(pid_t pid, int fd, int cmd, void *arg) {
    pcb_t *pcb = getpcb(pid);
    if (fd < 0 || fd > MAX_FILE_HANDLES)
        return -EINVAL;
    file_handle_t *h = &pcb->fh[fd];
    if (!h->present) {
        dwprintf("fd %x(pid:%x) not opened", fd, pid);
        return -EBADFD;
    }
    switch (cmd) {
        case F_GETFD:
            return h->flags;
        case F_SETFD:
            h->flags = (int) arg;
            return 0;
        case F_GETFL:
            return h->amode;
        case F_SETFL:
            h->amode = (int) arg;
            return 0;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC: {
            int lfd = (int) arg;
            if (lfd < 0 || lfd > MAX_FILE_HANDLES)
                return -EINVAL;
            mutex_lock(&pcb->lock);
            for (int x = lfd; x < MAX_FILE_HANDLES; x++) {
                if (!pcb->fh[x].present) {
                    memcpy(h, &pcb->fh[x], sizeof(file_handle_t));
                    if (cmd == F_DUPFD_CLOEXEC) {
                        pcb->fh[x].flags |= FD_CLOEXEC;
                    }
                    mutex_unlock(&pcb->lock);
                    return x;
                }
            }
            mutex_unlock(&pcb->lock);
            return -EMFILE;
        }
        default:
            return -EINVAL;
    }
}

int sys_fcntl(int fd, int cmd, void *arg) {
    return kfcntl(getpid(), fd, cmd, arg);
}