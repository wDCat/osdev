//
// Created by dcat on 8/11/17.
//

#include <system.h>
#include <str.h>
#include <proc.h>
#include "schedule.h"

extern void irq_remap();

void do_schedule(regs_t *r) {
    extern bool proc_ready;
    if (!getpcb(2)->present)return;
    dprintf("schedule routine");
    //FIXME some place damaged the irq remap......
    irq_remap();
    pcb_t *cur_pcb = getpcb(getpid());
    cur_pcb->time_slice++;
    //Min proc time slice schedule.
    uint32_t min_ts = (uint32_t) -1;
    pcb_t *choosed = 0;
    dprintf("wait queue[%x]:", proc_ready_queue->count);
    for (uint32_t x = 0, y = 0; y < 1023 && x < proc_ready_queue->count; y++) {
        if (proc_ready_queue->pcbs[y] == 0)continue;
        x++;
        dprintf("[%x]", proc_ready_queue->pcbs[y]->pid);
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
    if (choosed->pid != getpid() && cur_pcb->status == STATUS_RUN) {
        save_proc_state(cur_pcb, r);
        set_proc_status(cur_pcb, STATUS_READY);
    }
    dprintf("choosed proc:%x", choosed->pid);
    switch_to_proc(choosed);
}