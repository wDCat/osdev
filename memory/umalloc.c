//
// Created by dcat on 10/10/17.
//


#include <str.h>
#include "umalloc.h"

void *umalloc(pid_t pid, uint32_t size) {
    pcb_t *pcb = getpcb(pid);
    if (current_dir != pcb->page_dir) {
        deprintf("wrong page dir....");
        PANIC("")
        return NULL;
    }
    if (!pcb->heap_ready) {
        deprintf("heap is not ready!");
        PANIC("")
        return NULL;
    }
    return halloc(&pcb->heap, size, false);
}

void ufree(pid_t pid, void *ptr) {
    pcb_t *pcb = getpcb(pid);
    if (current_dir != pcb->page_dir) {
        deprintf("wrong page dir....");
        PANIC("")
    }
    if (!pcb->heap_ready) {
        deprintf("heap is not ready!");
        PANIC("")
    }
    hfree(&pcb->heap, ptr);
}

void *sys_malloc(uint32_t size) {
    return umalloc(getpid(), size);
}

void sys_free(void *ptr) {
    return ufree(getpid(), ptr);
}