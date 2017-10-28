//
// Created by dcat on 9/3/17.
//

#include <system.h>
#include <str.h>
#include <proc.h>
#include <timer.h>
#include <mutex.h>
#include <schedule.h>
#include <signal.h>
#include "mutex.h"

void mutex_init(mutex_lock_t *m) {
    m->mutex = 1;
    m->holdpid = 0;
    memset(&m->wait_queue, 0, sizeof(proc_queue_t));
}

//FIXME some bug...
void mutex_lock(mutex_lock_t *m) {
    int ints;
    pcb_t *pcb = getpcb(getpid());
    ASSERT(pcb);
    scli(&ints);
    if (m->holdpid == getpid())return;
    uint8_t al;
    __asm__ __volatile__(""
            "movb $0,%%al;"
            "xchgb %%al,%0;"
            "movb %%al,%1;"::"m"(m->mutex), "m"(al), "ax"(0));
    if (al == 1) {
        m->holdpid = getpid();
        //dprintf("proc %x get lock:%x", pcb->pid, m);
        srestorei(&ints);
        return;
    } else {
        //suspend it
        dprintf("proc %x wait lock:%x", getpid(), m);
        set_proc_status(pcb, STATUS_WAIT);
        CHK(proc_queue_insert(&m->wait_queue, pcb), "");
        srestorei(&ints);
        do_schedule(NULL);
    }
    _err:
    srestorei(&ints);
    PANIC("exception happened.");
}

void mutex_unlock(mutex_lock_t *m) {
    int ints;
    scli(&ints);
    if (m->holdpid != getpid())goto _ret;
    //dprintf("proc %x release lck:%x", getpid(), m);
    __asm__ __volatile__(""
            "movb $1,%%al;"
            "xchgb %%al,%0;"::"m"(m->mutex), "ax"(0));
    proc_queue_wakeupall(&m->wait_queue, true);
    m->holdpid = -1;
    _ret:
    srestorei(&ints);
    //do_schedule(NULL);
}
