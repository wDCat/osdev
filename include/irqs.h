//
// Created by dcat on 3/12/17.
//

#ifndef W2_IRQ_H
#define W2_IRQ_H

#include "system.h"
#include "idt.h"

extern void _irq0();

extern void _irq1();

extern void _irq2();

extern void _irq3();

extern void _irq4();

extern void _irq5();

extern void _irq6();

extern void _irq7();

extern void _irq8();

extern void _irq9();

extern void _irq10();

extern void _irq11();

extern void _irq12();

extern void _irq13();

extern void _irq14();

extern void _irq15();

static void *irq_routines[16] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
};

void irq_install_handler(int irq, void (*handler)(struct regs *r));

void irq_uninstall_handler(int irq);

void irq_handler(struct regs *r);

void irq_install();

#endif //W2_IRQ_H
