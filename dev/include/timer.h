//
// Created by dcat on 3/12/17.
//

#ifndef W2_TIMER_H
#define W2_TIMER_H

#include "../../ker/include/system.h"

void timer_handler(struct regs *r);

void timer_install();

void timer_phase(int hz);

void delay(unsigned long sec);

#endif //W2_TIMER_H
