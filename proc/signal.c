//
// Created by dcat on 9/14/17.
//


#include <proc.h>
#include <str.h>
#include "signal.h"

int do_signal_inner(int signal) {
    pcb_t *pcb = getpcb(getpid());
    switch (pcb->signal_handler[signal]) {
        case 1:
            goto _after;
        case 0:
            switch (signal) {
                case 0:
                    return 1;
                case SIGQUIT:
                case SIGILL:
                case SIGSEGV:
                case SIGSTOP:
                case SIGABRT:
                    dprintf("kill proc %x", getpid());
                    set_proc_status(pcb, STATUS_DIED);
                    break;
            }
            break;
        default:
            dprintf("call:%x", pcb->signal_handler[signal]);
            TODO;

    }
    _after:
    SIG_CLEAR(pcb->signal, signal);
    return 0;
}

int do_signal(regs_t *r) {
    pcb_t *pcb = getpcb(getpid());
    for (int x = 0; x < 32; x++) {
        if (BIT_GET(pcb->signal, x) && !BIT_GET(pcb->blocked, x)) {
            dprintf("do signal:%x", x + 1);
            if (do_signal_inner(x + 1))
                break;
        }
    }
}

void send_sig(pcb_t *pcb, int sig) {
    SIG_SET(pcb->signal, sig);
}

int sys_kill(pid_t pid, int sig) {
    send_sig(getpcb(pid), sig);
}