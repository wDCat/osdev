//
// Created by dcat on 3/12/17.
//

#include <page.h>
#include "include/isrs.h"
#include "include/idt.h"
#include "include/system.h"
#include "intdef.h"

#define MEMORY(x) ((uint8_t*)(x))

void fault_handler(struct regs *r) {
    int a = 0;
    switch (r->int_no) {
        case 0: puterr("[-] Division by zero fault.");
            break;
        case 1: puterr("[-] Debug exception.");
            break;
        case 2: puterr("[-] Non maskable interrupt exception");
            break;
        case 14: {
            uint32_t cr0;
            __asm__ __volatile__("mov %%cr0, %0":"=r"(cr0));
            cr0 ^= 0x80000000;
            __asm__ __volatile__("mov %0, %%cr0"::"r"(cr0));
            puterr_const("Page Fault.Paging has been disabled.");
            page_fault_handler(r);
        }
            break;
        default: putln_const("Unknown Error.");
            break;
    }
    for (;;);
}

void isrs_install() {
    idt_set_gate(0, (unsigned) _isr0, 0x08, 0x8E);
    idt_set_gate(1, (unsigned) _isr1, 0x08, 0x8E);
    idt_set_gate(2, (unsigned) _isr2, 0x08, 0x8E);
    idt_set_gate(3, (unsigned) _isr3, 0x08, 0x8E);
    idt_set_gate(4, (unsigned) _isr4, 0x08, 0x8E);
    idt_set_gate(5, (unsigned) _isr5, 0x08, 0x8E);
    idt_set_gate(6, (unsigned) _isr6, 0x08, 0x8E);
    idt_set_gate(7, (unsigned) _isr7, 0x08, 0x8E);
    idt_set_gate(8, (unsigned) _isr8, 0x08, 0x8E);
    idt_set_gate(9, (unsigned) _isr9, 0x08, 0x8E);
    idt_set_gate(10, (unsigned) _isr10, 0x08, 0x8E);
    idt_set_gate(11, (unsigned) _isr11, 0x08, 0x8E);
    idt_set_gate(12, (unsigned) _isr12, 0x08, 0x8E);
    idt_set_gate(13, (unsigned) _isr13, 0x08, 0x8E);
    idt_set_gate(14, (unsigned) _isr14, 0x08, 0x8E);
    idt_set_gate(15, (unsigned) _isr15, 0x08, 0x8E);
    idt_set_gate(16, (unsigned) _isr16, 0x08, 0x8E);
    idt_set_gate(17, (unsigned) _isr17, 0x08, 0x8E);
    idt_set_gate(18, (unsigned) _isr18, 0x08, 0x8E);


}