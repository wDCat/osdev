//
// Created by dcat on 9/13/17.
//

#include <proc.h>
#include <signal.h>
#include "wait.h"
#include "schedule.h"

pid_t sys_waitpid(pid_t pid, int *status, int options) {
    pcb_t *tpcb = getpcb(pid);
    /*
    pcb_t *pcb = getpcb(getpid());
    pcb->wait_for = pid;
    do_schedule(NULL);
    *status &= 0x00FF;
    *status |= (tpcb->exit_val & 0xFF) << 8;
    return pid;*/
    tpcb->hold_proc++;
    loop:
    if (get_proc_status(tpcb) == STATUS_DIED) {
        *status &= 0x00FF;
        *status |= (tpcb->exit_val & 0xFF) << 8;
        tpcb->hold_proc--;
        return pid;
    }
    set_proc_status(getpcb(getpid()), STATUS_WAIT);
    SIG_CLEAR(getpcb(getpid())->blocked, SIGCHLD);
    do_schedule(NULL);
    SIG_SET(getpcb(getpid())->blocked, SIGCHLD);
    goto loop;
}