//
// Created by dcat on 7/7/17.
//
#include <str.h>
#include <exec.h>
#include <wait.h>
#include <signal.h>
#include <exit.h>
#include "ker/include/idt.h"
#include "proc.h"
#include "syscall_handler.h"
#include "timer.h"

long errno;

long helloworld(uint32_t ebx, uint32_t ecx, uint32_t edx, uint32_t esi, uint32_t edi, regs_t *r) {
    dprintf("proc %x esp:%x useresp:%x tss_esp:%x", getpid(), r->esp, r->useresp, getpcb(getpid())->tss.esp);
    set_proc_status(getpcb(getpid()), STATUS_READY);
    switch_to_proc(getpcb(0));
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
        &sys_exit,
        &sys_open,
        &sys_close,
        &sys_read,
        &sys_write,
        &sys_stat,
        &sys_ls,
        &sys_exec,
        &sys_waitpid,
        &sys_kill,
        &sys_access,
        &sys_chdir,
        &sys_getcwd,
        &sys_lseek
};
uint32_t syscalls_count = sizeof(syscalls_table) / sizeof(uint32_t);

void syscall_install() {
    extern void _isr_syscall();
    idt_set_gate(0x60, (unsigned) _isr_syscall, 1 << 3, 0xEF);
}


typedef uint32_t (*syscall_fun_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, regs_t *r);

int syscall_handler(regs_t *r) {
    dprintf("syscall pid:%x no:%d ebx:%x ecx:%x edx:%x", getpid(), r->eax, r->ebx, r->ecx, r->edx);
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
