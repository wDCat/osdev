//
// Created by dcat on 11/22/17.
//

#ifndef W2_BRK_H
#define W2_BRK_H

#include "proc.h"

long kbrk(pid_t pid, void *addr);

long sys_brk(void *addr);

#endif //W2_BRK_H
