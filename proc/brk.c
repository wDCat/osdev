//
// Created by dcat on 11/22/17.
//

#include <str.h>
#include <page.h>
#include "brk.h"
#include "elfloader.h"

long kbrk(pid_t pid, void *addr) {
    uint32_t naddr = (uint32_t) addr;
    pcb_t *pcb = getpcb(pid);
    if (naddr < 0x8000000 || naddr > 0xA000000) {
        dprintf("return program brk:%x", pcb->program_break);
        return pcb->program_break;
    }
    if (naddr > pcb->program_break) {
        for (uint32_t x = pcb->program_break; x <= naddr; x += PAGE_SIZE) {
            alloc_frame(get_page(x, true, pcb->page_dir), false, true);
            page_typeinfo_t *t = get_page_type(x, pcb->page_dir);
            t->pid = pid;
            t->free_on_proc_exit = true;
            t->copy_on_fork = true;
        }
    } else if (naddr < pcb->program_break) {
        for (uint32_t x = pcb->program_break; x < naddr; x -= PAGE_SIZE) {
            //TODO free
            return -1;
        }
    }
    dprintf("program break set from %x to %x", pcb->program_break, naddr);
    pcb->program_break = naddr;
    return pcb->program_break;
}

long sys_brk(void *addr) {
    return kbrk(getpid(), addr);
}