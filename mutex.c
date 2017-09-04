//
// Created by dcat on 9/3/17.
//

#include <system.h>
#include <str.h>
#include <proc.h>
#include "mutex.h"

void mutex_init(mutex_lock_t *m) {
    m->mutex = 1;
}

void mutex_lock(mutex_lock_t *m) {
    uint16_t al;
    lock:
    __asm__ __volatile__(""
            "movb $0,%%al;"
            "xchgb %%al,%0;"
            "mov %%al,%1;"::"m"(m->mutex), "m"(al), "ax"(0));
    if (al > 0) {
        return;
    } else {
        //suspend it
        //dprintf("Suspend proc %x", getpid());
        goto lock;//busy wait!
    }
}

void mutex_unlock(mutex_lock_t *m) {
    __asm__ __volatile__(""
            "movb $1,%%al;"
            "xchgb %%al,%0;"::"m"(m->mutex), "ax"(0));
}
