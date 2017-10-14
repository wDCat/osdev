//
// Created by dcat on 10/14/17.
//

#include <system.h>
#include <str.h>
#include <kmalloc.h>
#include "squeue.h"

int squeue_init(squeue_t *sq) {
    memset(sq, 0, sizeof(squeue_t));
    return 0;
}

int squeue_insert(squeue_t *ns, uint32_t objaddr) {
    ASSERT(ns);
    uint32_t sc = 256;
    if (ns->first == NULL) {
        squeue_entry_t *ne = (squeue_entry_t *) kmalloc(sizeof(squeue_entry_t));
        ns->first = ne;
        ne->objaddr = objaddr;
        ns->count++;
    } else {
        squeue_entry_t *e = ns->first;
        while (e && sc--) {
            if (e->objaddr == objaddr)break;
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
    return 0;
}

uint32_t squeue_get(squeue_t *sq, int index) {
    ASSERT(sq && index >= 0);
    uint32_t sc = index;
    squeue_entry_t *e = sq->first;
    if (index == 0) {
        return e->objaddr;
    } else {
        while (e && sc--) {
            e = e->next;
            if (e == NULL)return NULL;
        }
        return e->objaddr;
    }
    return NULL;
}

int squeue_remove(squeue_t *sq, int index) {
    ASSERT(sq && index >= 0);
    uint32_t sc = index;
    squeue_entry_t *e = sq->first;
    if (index == 0) {
        sq->count--;
        kfree(sq->first);
        sq->first = sq->first->next;
    } else {
        while (e && sc--) {
            e = e->next;
            if (e == NULL)return -1;
        }
        squeue_entry_t *oe = e->next;
        e->next = e->next->next;
        sq->count--;
        kfree(oe);
    }
    return 0;
}