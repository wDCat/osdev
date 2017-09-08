//
// Created by dcat on 7/7/17.
//
#include <str.h>
#include "ker/include/idt.h"
#include "proc.h"
#include "syscall_handler.h"
#include "timer.h"

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

long hello_switcher(pid_t pid, uint32_t ecx, uint32_t edx, uint32_t esi, uint32_t edi, regs_t *r) {
    cli();
    save_proc_state(getpcb(getpid()), r);//so silly the cat...
    sti();
    switch_to_proc(getpcb(pid));
}

uint32_t syscalls_table[] = {
        &helloworld,
        &screen_print,
        &delay,
        &fork_s,
        &getpid,
        &hello_switcher,
        &proc_exit,
        &sys_open,
        &sys_close,
        &sys_read,
        &sys_write,
        &sys_stat,
        &sys_ls
};
uint32_t syscalls_count = sizeof(syscalls_table) / sizeof(uint32_t);

void syscall_install() {
    extern void _isr_syscall();
    idt_set_gate(0x60, (unsigned) _isr_syscall, 1 << 3, 0xEF);
}


typedef uint32_t (*syscall_fun_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, regs_t *r);

int syscall_handler(regs_t *r) {
    dprintf("syscall no:%x ebx:%x ecx:%x edx:%x", r->eax, r->ebx, r->ecx, r->edx);
    if (r->eax >= syscalls_count) {
        r->eax = -1;
        return 0;
    }
    syscall_fun_t fun = (syscall_fun_t) syscalls_table[r->eax];
    //A better implement?
    uint32_t ret = fun(r->ebx, r->ecx, r->edx, r->esi, r->edi, r);
    r->eax = ret;
    return 0;
}
