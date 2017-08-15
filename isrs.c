//
// Created by dcat on 3/12/17.
//

#include <page.h>
#include <str.h>
#include "include/isrs.h"
#include "include/idt.h"
#include "include/system.h"
#include "intdef.h"
#include "include/syscall.h"


void dump_regs(regs_t *r) {
    putf_const("[EAX:%x][EBX:%x][ECX:%x]", r->eax, r->ebx, r->ecx);
    putf_const("[EDX:%x][EBP:%x][ESP:%x]\n", r->edx, r->ebp, r->useresp);
    putf_const("[CS:%x][DX:%x][ES:%x]", r->cs, r->ds, r->es, r->fs, r->gs);
    putf_const("[FS:%x][GX:%x][EIP:%x]", r->fs, r->gs, r->eip);
}

void fault_handler(struct regs *r) {
    int a = 0;
    switch (r->int_no) {
        case 0: puterr_const("[-] Division by zero fault.");
            break;
        case 1: puterr_const("[-] Debug exception.");
            break;
        case 2: puterr_const("[-] Non maskable interrupt exception");
            break;
        case 8: {
            disable_paging();
            puterr_const("[-] Double Fault.");
        }
            break;
        case 6: puterr_const("[-] Invalid Opcode")
            break;
        case 13: puterr_const("[-] General Protection Fault Exception");
            dumpint("        ErrorCode:", r->err_code);
            dumphex("        PC:", r->eip);
            break;
        case 14: {
            disable_paging();
            puterr_const("Page Fault.Paging has been disabled.");
            page_fault_handler(r);
        }
            break;
        case 0x60: {
            syscall_handler(r);
        }
            break;
        default: {
            puterr_const("");
            //disable_paging();
            putf_const("[Abort]Unhandled Exception:%x\n", r->err_code);
            putn();
        }
            break;
    }
    if (r->int_no <= 18) {
        dump_regs(r);
        for (;;);
    }
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