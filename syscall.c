//
// Created by dcat on 7/7/17.
//
#include <str.h>
#include <idt.h>
#include "include/proc.h"
#include "include/syscall.h"
#include "include/timer.h"

long errno;

long helloworld() {
    putf_const("[USERMODE]Hello world\n");
    return 12;
}

long screen_print(const char *str) {
    putf_const("");//去掉这行下面就输出乱码....幬
    for (int x = 0; str[x] != '\0' && x < 0xFFFF; x++) {
        putc(str[x]);
    }
    return 0;
}

long fork_s(uint32_t ebx, uint32_t ecx, uint32_t edx, uint32_t esi, uint32_t edi, regs_t *r) {
    return fork(r);
}

long hello_switcher(pid_t pid) {
    switch_to_task(getpcb(pid));
}

const uint32_t syscalls_count = 6;
uint32_t syscalls_table[] = {
        &helloworld,
        &screen_print,
        &delay,
        &fork_s,
        &getpid,
        &hello_switcher
};


void syscall_install() {
    extern void _isr_syscall();
    idt_set_gate(0x60, (unsigned) _isr_syscall, 0x08, 0xEE);
    idt_set_gate(0x61, (unsigned) _isr_syscall, 1 << 3, 0xEF);
}

_impl_syscall0(helloworld, 0);

_impl_syscall1(screen_print, 1, const char*);

_impl_syscall1(delay, 2, uint32_t);

_impl_syscall0(fork, 3);

_impl_syscall0(getpid, 4);

_impl_syscall1(hello_switcher, 5, pid_t);

typedef uint32_t (*syscall_fun_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, regs_t *r);

int syscall_handler(regs_t *r) {
    //putf_const("[SYSCALL][No:%x]\n", r->eax);
    if (r->eax >= syscalls_count) {
        r->eax = -1;
        return 0;
    }
    syscall_fun_t fun = syscalls_table[r->eax];
    //A better implement?
    uint32_t ret = fun(r->ebx, r->ecx, r->edx, r->esi, r->edi, r);
    r->eax = ret;
    return 0;
}
