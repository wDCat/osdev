//
// Created by dcat on 7/7/17.
//

#include "syscall.h"

void syscall_install() {
}

int syscall_handler(regs_t *r) {
    putf_const("[SYSCALL][No:%x]\n", r->eax);
    return 0;
}