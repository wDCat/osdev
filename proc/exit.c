//
// Created by dcat on 9/14/17.
//

#include <signal.h>
#include <str.h>
#include <schedule.h>
#include <vfs.h>
#include <kmalloc.h>
#include "exit.h"

void sys_exit(int32_t ret) {
    pcb_t *pcb = getpcb(getpid());
    dprintf("[+] proc %x exited with ret:%x", getpid(), ret);
    do_exit(pcb, ret);
    deprintf("proc %x already exited,but still be scheduled!");
    for (;;);
}

void do_exit(pcb_t *pcb, int32_t ret) {
    pcb->exit_val = ret;
    if (pcb->fpcb) {
        dprintf("SIGCHLD sent to father proc %x", pcb->fpcb->pid);
        send_sig(pcb->fpcb, SIGCHLD);
    }

    for (uint8_t x = 0; x < MAX_FILE_HANDLES; x++) {
        if (pcb->fh[x].present)
            sys_close(x);
    }
    set_proc_status(pcb, STATUS_DIED);
    free_page_directory(pcb->page_dir);
    uint32_t reserved_page = pcb->reserved_page;
    pcb->reserved_page = 0;
    kfree(reserved_page);//Stack will not available.
    do_schedule(NULL);
}