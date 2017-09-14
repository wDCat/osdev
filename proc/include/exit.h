//
// Created by dcat on 9/14/17.
//

#ifndef W2_EXIT_H
#define W2_EXIT_H

#include "system.h"
#include "proc.h"

void do_exit(pcb_t *pcb, uint32_t ret);

void sys_exit(uint32_t ret);

#endif //W2_EXIT_H
