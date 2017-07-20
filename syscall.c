//
// Created by dcat on 7/7/17.
//

#include "include/syscall.h"
#include "include/timer.h"

long errno;

long helloworld() {
    putf_const("[USERMODE]Hello world\n");
    return 0;
}

long screen_print(const char *str) {
    putf_const("");//去掉这行下面就输出乱码....幬
    for (int x = 0; str[x] != '\0' && x < 0xFFFF; x++) {
        putc(str[x]);
    }
    return 0;
}

const uint32_t syscalls_count = 3;
uint32_t syscalls_table[3] = {
        &helloworld,
        &screen_print,
        &delay
};


void syscall_install() {
    extern void _isr_syscall();
    idt_set_gate(0x60, (unsigned) _isr_syscall, 0x08, 0xEE);
}

_impl_syscall0(helloworld, 0);

_impl_syscall1(screen_print, 1, const char*);

_impl_syscall1(delay, 2, uint32_t);

typedef uint32_t (*syscall_fun_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

int syscall_handler(regs_t *r) {
    //putf_const("[SYSCALL][No:%x]\n", r->eax);
    if (r->eax >= syscalls_count) {
        r->eax = -1;
        return 0;
    }
    syscall_fun_t fun = syscalls_table[r->eax];
    //A better implement?
    uint32_t ret = fun(r->ebx, r->ecx, r->edx, r->esi, r->edi);
    r->eax = ret;
    return 0;
}
