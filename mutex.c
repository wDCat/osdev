//
// Created by dcat on 9/3/17.
//

#include <system.h>
#include <str.h>
#include <proc.h>
#include <timer.h>
#include <mutex.h>
#include <schedule.h>
#include "mutex.h"

void mutex_init(mutex_lock_t *m) {
    m->mutex = 1;
    m->holdpid = 0;
}

//FIXME some bug...
void mutex_lock(mutex_lock_t *m) {
    bool dbgmsg_mark = false;
    uint8_t al;
    lock:
    __asm__ __volatile__(""
            "movb $0,%%al;"
            "xchgb %%al,%0;"
            "movb %%al,%1;"::"m"(m->mutex), "m"(al), "ax"(0));
    if (al == 1) {
        //dprintf("proc %x hold lck:%x", getpid(), m);
        return;
    } else {
        //suspend it
        if (!dbgmsg_mark) {
            dbgmsg_mark = true;
            dprintf("proc %x busy wait:%x %x", getpid(), m, dbgmsg_mark);
        }
        if (m->holdpid != 0)
            goto lock;
        set_proc_status(getpcb(getpid()), STATUS_WAIT);
        m->holdpid = getpid();
        do_schedule(NULL);
    }
}

void mutex_unlock(mutex_lock_t *m) {
    //dprintf("proc %x release lck:%x", getpid(), m);
    __asm__ __volatile__(""
            "movb $1,%%al;"
            "xchgb %%al,%0;"::"m"(m->mutex), "ax"(0));
    if (m->holdpid != 0) {
        set_proc_status(getpcb(getpid()), STATUS_READY);
        m->holdpid = 0;
        do_schedule(NULL);
    }
}
