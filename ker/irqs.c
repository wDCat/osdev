//
// Created by dcat on 3/12/17.
//

#include <io.h>
#include "include/irqs.h"

void irq_remap(void) {
    /* Normally, IRQs 0 to 7 are mapped to entries 8 to 15. This
*  is a problem in protected mode, because IDT entry 8 is a
*  Double Fault! Without remapping, every time IRQ0 fires,
*  you get a Double Fault Exception, which is NOT actually
*  what's happening. We send commands to the Programmable
*  Interrupt Controller (PICs - also called the 8259's) in
*  order to make IRQ0 to 15 be remapped to IDT entries 32 to
*  47 */
    outportb(0x20, 0x11);
    outportb(0xA0, 0x11);
    outportb(0x21, 0x20);
    outportb(0xA1, 0x28);
    outportb(0x21, 0x04);
    outportb(0xA1, 0x02);
    outportb(0x21, 0x01);
    outportb(0xA1, 0x01);
    outportb(0x21, 0x0);
    outportb(0xA1, 0x0);
}

void irq_install() {
    irq_remap();
    idt_set_gate(32, (unsigned) _irq0, 0x08, 0x8E);
    idt_set_gate(33, (unsigned) _irq1, 0x08, 0x8E);
    idt_set_gate(34, (unsigned) _irq2, 0x08, 0x8E);
    idt_set_gate(35, (unsigned) _irq3, 0x08, 0x8E);
    idt_set_gate(36, (unsigned) _irq4, 0x08, 0x8E);
    idt_set_gate(37, (unsigned) _irq5, 0x08, 0x8E);
    idt_set_gate(38, (unsigned) _irq6, 0x08, 0x8E);
    idt_set_gate(39, (unsigned) _irq7, 0x08, 0x8E);
    idt_set_gate(40, (unsigned) _irq8, 0x08, 0x8E);
    idt_set_gate(41, (unsigned) _irq9, 0x08, 0x8E);
    idt_set_gate(42, (unsigned) _irq10, 0x08, 0x8E);
    idt_set_gate(43, (unsigned) _irq11, 0x08, 0x8E);
    idt_set_gate(44, (unsigned) _irq12, 0x08, 0x8E);
    idt_set_gate(45, (unsigned) _irq13, 0x08, 0x8E);
    idt_set_gate(46, (unsigned) _irq14, 0x08, 0x8E);
    idt_set_gate(47, (unsigned) _irq15, 0x08, 0x8E);
}

void irq_handler(struct regs *r) {
/* This is a blank function pointer */
    //putln("irq_handler called.")
    void (*handler)(struct regs *r);

    /* Find out if we have a custom handler to run for this
    *  IRQ, and then finally, run it */
    handler = irq_routines[r->int_no - 32];
    //putf_const("[%x*]", r->int_no - 32);
    if (handler) {
        handler(r);
    }

    /* If the IDT entry that was invoked was greater than 40
    *  (meaning IRQ8 - 15), then we need to send an EOI to
    *  the slave controller */
    if (r->int_no >= 40) {
        outportb(0xA0, 0x20);
    }

    /* In either case, we need to send an EOI to the master
    *  interrupt controller too */
    outportb(0x20, 0x20);
}

void irq_install_handler(int irq, void (*handler)(struct regs *r)) {
    irq_routines[irq] = handler;
}

void irq_uninstall_handler(int irq) {
    irq_routines[irq] = 0;
}