//
// Created by dcat on 8/11/17.
//

#include "system.h"
#include <str.h>
#include <proc.h>
#include <keyboard.h>
#include <signal.h>
#include <isrs.h>
#include <kmalloc.h>
#include "schedule.h"

extern void irq_remap();

proc_queue_iter_t *rriter = NULL;
bool force_print_queue = true;
enum {
    RR,
    MPTS
} schedule_type = RR;

void do_schedule(regs_t *r) {
    extern bool proc_ready;
    if (!getpcb(2)->present)return;
    cli();
    dprintf_begin("schedule routine.cur_pid:%x eip:%x", getpid(), r->eip);
    //FIXME some place damaged the irq remap......
    irq_remap();
    pcb_t *cur_pcb = getpcb(getpid());
    cur_pcb->time_slice++;
    if (proc_queue_count(proc_wait_queue)) {
        dprintf_cont("\n{wait queue[%x]:", proc_queue_count(proc_wait_queue));
        proc_queue_iter_t iter;
        proc_queue_iter_begin(&iter, proc_wait_queue);
        pcb_t *pcb = proc_queue_iter_next(&iter);
        while (pcb != NULL) {
            dprintf_cont("[%x]", pcb->pid);
            if (pcb->signal & ~pcb->blocked) {
                dprintf_cont("[%x wake up by signal]", getpid());
                set_proc_status(pcb, STATUS_READY);
            }
            pcb = proc_queue_iter_next(&iter);
        }
        proc_queue_iter_end(&iter);
        dprintf_cont("}");
    }
    pcb_t *choosed = NULL;
    if (proc_queue_count(proc_ready_queue)) {
        switch (schedule_type) {
            case RR: {
                //RR
                dprintf_cont("\n{ready queue[%x]:", proc_queue_count(proc_ready_queue));
                if (force_print_queue) {//debug
                    proc_queue_iter_t iter;
                    proc_queue_iter_begin(&iter, proc_ready_queue);
                    pcb_t *pcb = proc_queue_iter_next(&iter);
                    while (pcb != NULL) {
                        dprintf_cont("[%x]", pcb->pid);
                        pcb = proc_queue_iter_next(&iter);
                    }
                    proc_queue_iter_end(&iter);
                } else {
                    dprintf_cont("[*]...");
                }
                if (rriter == NULL) {
                    rriter = (proc_queue_iter_t *) kmalloc(sizeof(proc_queue_iter_t));
                    proc_queue_iter_begin(rriter, proc_ready_queue);
                }
                choosed = proc_queue_iter_next(rriter);
                if (choosed == NULL) {
                    proc_queue_iter_end(rriter);
                    proc_queue_iter_begin(rriter, proc_ready_queue);
                    choosed = proc_queue_iter_next(rriter);
                }
                dprintf_cont("}");
            }
                break;
            case MPTS: {
                //Min proc time slice schedule.
                proc_queue_iter_t iter;
                proc_queue_iter_begin(&iter, proc_ready_queue);
                uint32_t min_ts = (uint32_t) -1;
                dprintf_cont("\n{ready queue[%x]:", proc_queue_count(proc_ready_queue));
                pcb_t *pcb = proc_queue_iter_next(&iter);
                while (pcb != NULL) {
                    dprintf_cont("[%x]", pcb->pid);
                    if (pcb == cur_pcb)continue;
                    if (pcb->time_slice < min_ts) {
                        min_ts = pcb->time_slice;
                        choosed = pcb;
                    }
                    pcb = proc_queue_iter_next(&iter);
                }
                proc_queue_iter_end(&iter);
                dprintf_cont("}");
            }
                break;
        }
    }
    dprintf_end();
    if (choosed == NULL) {
        if (cur_pcb->status != STATUS_RUN) {
            choosed = getpcb(0);//idle~
        } else choosed = cur_pcb;
    }
    if (choosed != NULL && choosed->pid != getpid()) {
        if (cur_pcb->status == STATUS_RUN)
            set_proc_status(cur_pcb, STATUS_READY);
        dprintf("\nchoosed proc:%x", choosed->pid);
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

void choose_next_task(regs_t *r) {
    TODO;
    save_cur_proc_reg(r);
    dprintf("pass in pcb:%x no:%d ebx:%x ecx:%x edx:%x", getpid(), r->eax, r->ebx, r->ecx, r->edx);
    pcb_t *pcb = (pcb_t *) r->ebx;
    dprintf("soft task switch to %x lastreg:%x", pcb->pid, pcb->lastreg);
    //r->esp = pcb->lastreg->esp;
    regs_t *nr = pcb->lastreg;
    dump_regs(nr);
    __asm__ __volatile__("mov %%eax,%%esp;"
            "mov %%ebx,%%cr3;"
            "pop %%gs;"
            "pop %%fs;"
            "pop %%es;"
            "pop %%ds;"
            "popa;"
            "add $0x8,%%esp;"
            "iret;"::"a"(pcb->lastreg), "b"(pcb->page_dir->physical_addr));
}