//
// Created by dcat on 7/7/17.
//

#include "syscall.h"

void syscall_install() {
    extern void _isr_syscall();
    idt_set_gate(0x60, (unsigned) _isr_syscall, 0x08, 0x8E);
}

int syscall_handler(regs_t *r) {
    putf_const("[SYSCALL][No:%x]\n", r->eax);
    return 0;
}