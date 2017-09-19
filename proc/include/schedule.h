//
// Created by dcat on 8/11/17.
//

#ifndef W2_SCHEDULING_H
#define W2_SCHEDULING_H

#include "system.h"

void do_schedule(regs_t *r);

void do_schedule_rejmp(regs_t *r);

void proc_rejmp(struct pcb_struct *pcb);

#endif //W2_SCHEDULING_H
