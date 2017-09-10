//
// Created by dcat on 3/12/17.
//

#include <str.h>
#include <schedule.h>
#include <proc.h>
#include <io.h>
#include "timer.h"
#include "../ker/include/irqs.h"

uint32_t timer_count = 1;

void get_time_count(uint32_t *data) {
    cli();
    *data = timer_count;
    sti();
}

void timer_handler(struct regs *r) {
    cli();
    timer_count++;
    if (timer_count % 36 == 0) {
        do_schedule(r);
    }
    sti();
}

void delay(unsigned long sec) {
    if (getpid() != 0)
        dprintf("proc %x delay %ds.", getpid(), sec);
    uint32_t tm;
    get_time_count(&tm);
    uint32_t target_timer_count = tm + sec * 18;
    unsigned long last_timer = 0;
    while (tm < target_timer_count) {
        get_time_count(&tm);
        if (timer_count % 18 == 0 && last_timer != tm) {
            last_timer = tm;
        }
    }
}

void timer_install() {
    irq_install_handler(0, timer_handler);
    //timer_phase(1);
}

void timer_phase(int hz) {
    int divisor = 1193180 / hz;       /* Calculate our divisor */
    outportb(0x43, 0x36);             /* Set our command byte 0x36 */
    outportb(0x40, divisor & 0xFF);   /* Set low byte of divisor */
    outportb(0x40, divisor >> 8);     /* Set high byte of divisor */
}