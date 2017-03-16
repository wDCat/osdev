//
// Created by dcat on 3/12/17.
//

#include "include/timer.h"
#include "include/irqs.h"

unsigned long timer_count = 1;

void timer_handler(struct regs *r) {
    timer_count++;
    //if (timer_count >= 0xFFFFFFFF)timer_count = 0;
    if (timer_count % 18 == 0) {
        //putln("timer routine.");
    }
}

//FIXME not work.
void delay(unsigned long sec) {
    int target_timer_count = timer_count + sec;
    unsigned long last_timer = 0;
    while (timer_count < target_timer_count) {
        if (timer_count % 18 == 0 && last_timer != timer_count) {
            putln("timer routine.");
            last_timer = timer_count;
        }
    }
}

void timer_install() {
    irq_install_handler(0, timer_handler);
}

void timer_phase(int hz) {
    int divisor = 1193180 / hz;       /* Calculate our divisor */
    outportb(0x43, 0x36);             /* Set our command byte 0x36 */
    outportb(0x40, divisor & 0xFF);   /* Set low byte of divisor */
    outportb(0x40, divisor >> 8);     /* Set high byte of divisor */
}