//
// Created by dcat on 10/13/17.
//

#include <proc.h>
#include <kmalloc.h>
#include <str.h>
#include <dynlibs.h>
#include "proc_queue.h"

//default proc queues
proc_queue_t *proc_avali_queue,
        *proc_ready_queue,
        *proc_wait_queue,
        *proc_died_queue;

int proc_queue_insert(proc_queue_t *ns, pcb_t *pcb) {
    ASSERT(ns && pcb);
    uint32_t sc = 256;
    if (ns->first == NULL) {
        proc_queue_entry_t *ne = (proc_queue_entry_t *) kmalloc(sizeof(proc_queue_entry_t));
        ne->pcb = pcb;
        ne->next = NULL;
        ns->first = ne;
        ns->count++;
        ASSERT(ns->count == 1);
    } else {
        proc_queue_entry_t *e = ns->first;
        while (e && sc--) {
            if (e->pcb == pcb)break;
            if (e->next == NULL) {
                proc_queue_entry_t *ne = (proc_queue_entry_t *) kmalloc(sizeof(proc_queue_entry_t));
                ne->pcb = pcb;
                ne->next = NULL;
                e->next = ne;
                ns->count++;
                break;
            }
            e = e->next;
        }
    }
    if (sc == 0) {
        PANIC("Unknown Exception");
    }
    return 0;
}

pcb_t *proc_queue_next(proc_queue_t *ns) {
    ASSERT(ns);
    return ns->first->pcb;
}

int proc_queue_remove(proc_queue_t *old, pcb_t *pcb) {
    ASSERT(old && pcb);
    uint32_t sc = 256;
    bool removed = false;
    proc_queue_entry_t *e = old->first;
    if (old->first->pcb == pcb) {
        old->count--;
        kfree(old->first);
        old->first = old->first->next;
        removed = true;
    } else
        while (e && sc--)
            if (e->next->pcb == pcb) {
                proc_queue_entry_t *oe = e->next;
                e->next = e->next->next;
                old->count--;
                kfree(oe);
                removed = true;
                break;
            } else e = e->next;
    if (sc == 0) {
        PANIC("Unknown Exception");
    }
    return !removed;
}