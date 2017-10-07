//
// Created by dcat on 8/11/17.
//

#include "system.h"
#include <str.h>
#include <proc.h>
#include <keyboard.h>
#include <signal.h>
#include "schedule.h"

extern void irq_remap();

uint32_t sch_rr_last = 0;

void do_schedule(regs_t *r) {
    extern bool proc_ready;
    if (!getpcb(2)->present)return;
    if (getpid() != 0)
        dprintf("schedule routine.cur_pid:%x eip:%x", getpid(), r->eip);
    //FIXME some place damaged the irq remap......
    irq_remap();
    pcb_t *cur_pcb = getpcb(getpid());
    cur_pcb->time_slice++;
    dprintf("cur pid:%x status:%x", cur_pcb->pid, cur_pcb->status);
    if (proc_wait_queue->count > 0) {
        dprintf_begin("wait queue[%x]:", proc_wait_queue->count);
        proc_queue_entry_t *e = proc_wait_queue->first;
        uint32_t c = proc_wait_queue->count;
        while (e && c--) {
            dprintf_cont("[%x]", e->pcb->pid);
            pcb_t *pcb = e->pcb;
            if (pcb->signal & ~pcb->blocked) {
                dprintf_cont("[%x wake up by signal]", getpid());
                set_proc_status(pcb, STATUS_READY);
            }
        }
        dprintf_end();
    }
    pcb_t *choosed = NULL;
    if (false) {
        /*
        //RR
        bool sfound = false;
        dprintf_begin("[RR:%x]ready queue[%x]:", sch_rr_last, proc_ready_queue->count);
        if (sch_rr_last >= proc_ready_queue->count)
            sch_rr_last = 0;
        for (uint32_t x = sch_rr_last + 1, y = 0, lmask = false; y < 1023 && x <= proc_ready_queue->count; y++) {
            if (x == proc_ready_queue->count) {
                if (lmask)
                    break;
                lmask = true;
                x = 0;
            }
            if (proc_ready_queue->pcbs[y] == 0)continue;
            x++;
            dprintf_cont("[%d:%x]", y, proc_ready_queue->pcbs[y]->pid);
            if (proc_ready_queue->pcbs[y]->pid == getpid())continue;
            if (!sfound && !(sch_rr_last--)) {
                sfound = true;
                choosed = proc_ready_queue->pcbs[y];
                sch_rr_last = x;
            }
            //break;
        }
        dprintf_end();*/
    } else {
        //Min proc time slice schedule.
        uint32_t min_ts = (uint32_t) -1;
        if (proc_ready_queue->count > 0) {
            dprintf_begin("ready queue[%x]:", proc_ready_queue->count);
            proc_queue_entry_t *e = proc_ready_queue->first;
            uint32_t c = proc_ready_queue->count;
            while (e && c--) {
                dprintf_cont("[%x]", e->pcb->pid);
                if (e->pcb == cur_pcb)continue;
                if (e->pcb->time_slice < min_ts) {
                    min_ts = e->pcb->time_slice;
                    choosed = e->pcb;
                }
            }
            dprintf_end();
        }
    }
    if (choosed == NULL) {
        if (cur_pcb->status != STATUS_RUN) {
            choosed = getpcb(0);//idle~
        } else choosed = cur_pcb;
    }
    if (choosed != NULL && choosed->pid != getpid()) {
        if (cur_pcb->status == STATUS_RUN)
            set_proc_status(cur_pcb, STATUS_READY);
        dprintf("choosed proc:%x", choosed->pid);
    }
    switch_to_proc(choosed);
}

void do_schedule_rejmp(regs_t *r) {
    pcb_t *pcb = getpcb(getpid());
    dprintf("%x schedle_rejmp.new pc:%x", pcb->pid, pcb->tss.eip);
    set_proc_status(getpcb(getpid()), STATUS_READY);
    pcb->rejmp = true;
    do_schedule(r);
}

void proc_rejmp(pcb_t *pcb) {
    if (getpid() == pcb->pid) {
        pcb->rejmp = true;
        set_proc_status(getpcb(getpid()), STATUS_READY);
        switch_to_proc(pcb);
        return;
    }
    dprintf("%x rejmp.new pc:%x", pcb->pid, pcb->tss.eip);
    pcb->rejmp = true;
}

