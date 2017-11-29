//
// Created by dcat on 9/13/17.
//

#include <proc.h>
#include <signal.h>
#include <exit.h>
#include <str.h>
#include <errno.h>
#include "wait.h"
#include "schedule.h"

pid_t sys_waitpid(pid_t pid, int *status, int options) {
    dprintf("waiting for child pid:%d", pid);
    loop:
    if (pid < 0) {
        pcb_t *tpcb = getpcb(getpid())->cpcb;
        bool all_died = true;
        while (tpcb != NULL) {
            ASSERT(tpcb->fpcb == getpcb(getpid()));
            proc_status_t tstat = get_proc_status(tpcb);
            switch (tstat) {
                case STATUS_ZOMBIE:
                    if (status)
                        *status = (tpcb->exit_val & 0xFF) << 8 | (tpcb->exit_sig & 0xFF);
                    set_proc_status(tpcb, STATUS_DIED);
                    return tpcb->pid;
                case STATUS_WAIT:
                case STATUS_READY:
                case STATUS_RUN:
                case STATUS_NEW:
                    all_died = false;
                default:
                    break;
            }
            tpcb = tpcb->next_pcb;
        }
        if (all_died)return -ECHILD;
        set_proc_status(getpcb(getpid()), STATUS_WAIT);
        SIG_CLEAR(getpcb(getpid())->blocked, SIGCHLD);
        do_schedule(NULL);
        SIG_SET(getpcb(getpid())->blocked, SIGCHLD);
        goto loop;
    }
    pcb_t *tpcb = getpcb(pid);
    if (tpcb->fpcb != getpcb(getpid()))return -ECHILD;

    ASSERT(pid > 0);
    proc_status_t tstat = get_proc_status(tpcb);
    if (tstat == STATUS_ZOMBIE || tstat == STATUS_DIED) {
        if (status)
            *status = (tpcb->exit_val & 0xFF) << 8 | (tpcb->exit_sig & 0xFF);
        if (tstat == STATUS_ZOMBIE)
            set_proc_status(tpcb, STATUS_DIED);
        return pid;
    }

    set_proc_status(getpcb(getpid()), STATUS_WAIT);
    SIG_CLEAR(getpcb(getpid())->blocked, SIGCHLD);
    do_schedule(NULL);
    SIG_SET(getpcb(getpid())->blocked, SIGCHLD);
    goto loop;
}

pid_t sys_wait4(pid_t pid, int *wstatus, int options, struct rusage *rusage) {
    //TODO getrusage
    return sys_waitpid(pid, wstatus, options);
}

