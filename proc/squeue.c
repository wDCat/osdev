//
// Created by dcat on 10/14/17.
//

#include <system.h>
#include <str.h>
#include <kmalloc.h>
#include <squeue.h>
#include "squeue.h"

int squeue_init(squeue_t *sq) {
    memset(sq, 0, sizeof(squeue_t));
    return 0;
}

int squeue_set(squeue_t *ns, int index, uint32_t objaddr) {
    ASSERT(ns && index < ns->count);
    int sc = index;
    squeue_entry_t *e = ns->first;
    while (e && sc--) {
        e = e->next;
    }
    if (e == NULL)return -1;
    e->objaddr = objaddr;
    return 0;
}

int squeue_insert(squeue_t *ns, uint32_t objaddr) {
    ASSERT(ns);
    dprintf("insert item %x to %x current cound:%x", objaddr, ns, ns->count);
    uint32_t sc = 256;
    if (ns->first == NULL) {
        squeue_entry_t *ne = (squeue_entry_t *) kmalloc(sizeof(squeue_entry_t));
        ns->first = ne;
        ne->next = NULL;
        ne->objaddr = objaddr;
        ns->count++;
    } else {
        squeue_entry_t *e = ns->first;
        while (e && sc--) {
            if (e->objaddr == objaddr)break;
            dprintf("iter next:%x", e->next);
            if (e->next == NULL) {
                squeue_entry_t *ne = (squeue_entry_t *) kmalloc(sizeof(squeue_entry_t));
                ne->objaddr = objaddr;
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
    dprintf("count %x", ns->count);
    return 0;
}

bool squeue_isempty(squeue_t *sq) {
    return sq->count == 0;
}


uint32_t squeue_get(squeue_t *sq, int index) {
    ASSERT(sq && index >= 0);
    uint32_t sc = index;
    squeue_entry_t *e = sq->first;
    while (e && sc--) {
        e = e->next;
    }
    if (e == NULL)return NULL;
    return e->objaddr;

}

int squeue_remove_vout(squeue_t *sq, int index, uint32_t *valout) {
    ASSERT(sq && index >= 0);
    if (index >= sq->count)return -1;
    squeue_entry_t *e = sq->first;
    if (index == 0) {
        if (valout)
            *valout = e->objaddr;
        sq->first = sq->first->next;
        sq->count--;
    } else {
        uint32_t sc = (uint32_t) index - 1;
        while (e && sc--) {
            e = e->next;
        }
        if (e == NULL)return -1;
        squeue_entry_t *oe = e->next;
        if (valout)
            *valout = oe->objaddr;
        e->next = e->next->next;
        sq->count--;
        kfree(oe);
    }
    return 0;
}

inline int squeue_remove(squeue_t *sq, int index) {
    return squeue_remove_vout(sq, index, NULL);
}