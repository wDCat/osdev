//
// Created by dcat on 7/7/17.
//

#ifndef W2_SYSCALL_H
#define W2_SYSCALL_H

#include "../ker/include/isrs.h"
#include "../proc/elf/include/proc.h"

void syscall_install();

int syscall_handler(regs_t *r);

#endif //W2_SYSCALL_H
