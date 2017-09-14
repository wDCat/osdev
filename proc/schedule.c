//
// Created by dcat on 8/11/17.
//

#include "system.h"
#include <str.h>
#include <proc.h>
#include <keyboard.h>
#include "schedule.h"

extern void irq_remap();

void do_schedule(regs_t *r) {
    extern bool proc_ready;
    if (!getpcb(2)->present)return;
    if (getpid() != 0)
        dprintf("schedule routine.cur_pid:%x eip:%x", getpid(), r->eip);
    //FIXME some place damaged the irq remap......
    irq_remap();
    pcb_t *cur_pcb = getpcb(getpid());
    cur_pcb->time_slice++;

    //Min proc time slice schedule.
    uint32_t min_ts = (uint32_t) -1;
    pcb_t *choosed = 0;

    dprintf("wait queue[%x]:", proc_wait_queue->count);
    for (uint32_t x = 0, y = 0; y < 1023 && x < proc_wait_queue->count; y++) {
        pcb_t *pcb = proc_wait_queue->pcbs[y];
        if (pcb == 0)continue;
        x++;
        dprintf("[%x]", pcb->pid);
        if (pcb->signal & ~pcb->status) {
            dprintf("%x wake up by signal.", getpid());
            set_proc_status(pcb, STATUS_READY);
        }

    }
    dprintf("ready queue[%x]:", proc_ready_queue->count);
    for (uint32_t x = 0, y = 0; y < 1023 && x < proc_ready_queue->count; y++) {
        if (proc_ready_queue->pcbs[y] == 0)continue;
        x++;
        dprintf("[%x]", proc_ready_queue->pcbs[y]->pid);
        if (proc_ready_queue->pcbs[y]->pid == getpid())continue;
        if (proc_ready_queue->pcbs[y]->time_slice < min_ts) {
            choosed = proc_ready_queue->pcbs[y];
            min_ts = choosed->time_slice;
        }
    }
    if (choosed == 0)
        if (cur_pcb->status != STATUS_RUN) {
            choosed = getpcb(0);//idle~
        } else {
            return;
        }
    if (choosed->pid != getpid()) {
        if (cur_pcb->status == STATUS_RUN)
            set_proc_status(cur_pcb, STATUS_READY);
        if (r != NULL) {
            save_proc_state(cur_pcb, r);
        }
        dprintf("choosed proc:%x", choosed->pid);
    }
    switch_to_proc(choosed);
}

void do_schedule_rejmp(regs_t *r) {
    dprintf("%x rejmp.new pc:%x", getpid(), getpcb(getpid())->tss.eip);
    set_proc_status(getpcb(getpid()), STATUS_READY);
    setpid(0);
    do_schedule(r);
}