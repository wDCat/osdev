//
// Created by dcat on 9/14/17.
//


#include <proc.h>
#include <str.h>
#include <exit.h>
#include "signal.h"

void signal_print_proc(pcb_t *pcb) {
    dprintf("signals:---------------");
    for (int x = 0; x < 32; x++) {
        if (BIT_GET(pcb->signal, x)) {
            if (BIT_GET(pcb->blocked, x)) {
                dprintf("%d blocked.");
            } else {
                dprintf("%d", x + 1);
            }
        }
    }
}

int do_signal_inner(pcb_t *pcb, int signal) {
    switch (pcb->signal_handler[signal]) {
        case 1:
            goto _after;
        case 0:
            switch (signal) {
                case 0:
                    return 1;
                case SIGINT:
                case SIGQUIT:
                case SIGILL:
                case SIGSEGV:
                case SIGSTOP:
                case SIGABRT:
                    dprintf("kill proc %x", getpid());
                    do_exit(pcb, -1);
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

int do_signal(pcb_t *pcb, regs_t *r) {
    for (int x = 0; x < 32; x++) {
        if (BIT_GET(pcb->signal, x) && !BIT_GET(pcb->blocked, x)) {
            dprintf("do signal:%x", x + 1);
            if (do_signal_inner(pcb, x + 1))
                break;
        }
    }
}

void send_sig(pcb_t *pcb, int sig) {
    if (pcb->pid >= 2 && pcb->present)
        SIG_SET(pcb->signal, sig);
    else
        deprintf("signal ignored.pid:%x", pcb->pid);
}

int sys_kill(pid_t pid, int sig) {
    send_sig(getpcb(pid), sig);
}