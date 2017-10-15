//
// Created by dcat on 9/14/17.
//

#include <signal.h>
#include <str.h>
#include <schedule.h>
#include <vfs.h>
#include <kmalloc.h>
#include <page.h>
#include "exit.h"

void sys_exit(int32_t ret) {
    pcb_t *pcb = getpcb(getpid());
    dprintf("[+] proc %x exited with ret:%x", getpid(), ret);
    do_exit(pcb, ret);
    deprintf("proc %x already exited,but still be scheduled!");
    for (;;);
}

int free_proc_frames(pcb_t *pcb) {
    page_directory_t *dir = pcb->page_dir;
    int count = 0;
    for (int x = 0; x < 1024; x++) {
        if (dir->tables[x]) {
            if (dir->tables[x] != kernel_dir->tables[x]) {
                count++;
                dprintf("free space %x-%x", x * 1024 * 0x1000, (x + 1) * 1024 * 0x1000);
                page_table_t *pt = dir->tables[x];
                bool is_empty = true;
                for (int y = 0; y < 1024; y++) {
                    if (pt->pages[x].frame) {
                        if (pt->typeinfo[x].pid == pcb->pid && pt->typeinfo[x].free_on_proc_exit) {
                            dprintf("freeing frame:%x", pt->pages[x].frame);
                            count++;
                            free_frame(&pt->pages[x]);
                        } else is_empty = false;
                    }
                }
                if (is_empty)
                    kfree(pt);
            }
        }
    }
    dprintf("free %x page table(s).", count);
    return 0;
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
    free_proc_frames(pcb);
    uint32_t reserved_page = pcb->reserved_page;
    pcb->reserved_page = 0;
    kfree(reserved_page);//Stack will not available.
    do_schedule(NULL);
}