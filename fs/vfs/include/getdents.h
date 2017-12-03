//
// Created by dcat on 12/1/17.
//

#ifndef W2_GETDENTS_H
#define W2_GETDENTS_H

#include "def.h"
#include "vfs.h"
#include "ugetdents.h"


int kgetdents(pid_t pid, unsigned int fd, struct linux_dirent *dirp, unsigned int count);

int sys_getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count);

#endif //W2_GETDENTS_H
