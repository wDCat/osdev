//
// Created by dcat on 3/12/17.
//

#include "page.h"
#include <str.h>
#include <schedule.h>
#include <signal.h>
#include <page.h>
#include <dynlibs.h>
#include "../proc/swap/include/swap.h"
#include "isrs.h"
#include "idt.h"
#include "include/system.h"
#include "intdef.h"
#include "syscall_handler.h"


void dump_regs(regs_t *r) {
    dprintf("REGS:");
    dprintf("-------------------------------------------");
    dprintf("[EAX:%x][EBX:%x][ECX:%x]", r->eax, r->ebx, r->ecx);
    dprintf("[EDX:%x][ESI:%x][EDI:%x]", r->edx, r->esi, r->edi);
    dprintf("[HA:+1s][EBP:%x][ESP:%x]", r->ebp, r->useresp);
    dprintf("[CS:%x][DS:%x][ES:%x]", r->cs, r->ds, r->es);
    dprintf("[FS:%x][GS:%x][EIP:%x]", r->fs, r->gs, r->eip);
}

void dump_page_dir(pid_t *pid) {
    page_directory_t *dir = getpcb(pid)->page_dir;
    dprintf("PAGEDIR[PID:%x]:", pid);
    dprintf("-------------------------------------------");
    for (int x = 0; x < 1024; x++) {
        if (dir->tables[x]) {
            if (dir->tables[x] != kernel_dir->tables[x]) {
                dprintf("page table[%x]:%x-%x", dir->tables[x], x * 1024 * 0x1000, (x + 1) * 1024 * 0x1000);
                dprintf("---------------------");
                page_table_t *table = dir->tables[x];
                for (int y = 0; y < 1024; y++) {
                    if (table->pages[y].frame) {
                        dprintf("entry:%x frame:%x", y * 0x1000 + x * 1024 * 0x1000, table->pages[y].frame);
                    }
                }
                dprintf("---------------------");
            }
        }
    }
}

void fault_handler(struct regs *r) {
    cli();
    save_cur_proc_reg(r);
    bool umoude_con = false, no_kill = false;
    //TODO:move kernel stack before disable paging:(
    switch (r->int_no) {
        case 0: puterr_const("[-] Division by zero fault.");
            umoude_con = true;
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
        case 6: puterr_const("[-] Invalid Opcode");
            umoude_con = true;
            break;
        case 0xA: puterr_const("");
            putf_const("[-] Invalid TSS");
            dumpint("        ErrorCode:", r->err_code);
            break;
        case 0xD: puterr_const("[-] General Protection Fault Exception");
            dumpint("        ErrorCode:", r->err_code);
            dumphex("        PC:", r->eip);
            umoude_con = true;
            break;
        case 0xE: {
            if (swap_handle_page_fault(r) && dynlibs_handle_page_fault(r)) {
                puterr_const("[-] Page Fault.");
                umoude_con = true;
                page_fault_handler(r);
                dump_page_dir(getpid());
            } else {
                no_kill = true;
                //do_schedule_rejmp(NULL);
            }
        }
            break;
        case 0x60: {
            syscall_handler(r);
        }
            break;
        default: {
            puterr_const("");
            //disable_paging();
            putf_const("[Abort]Unhandled Exception:%x\n", r->int_no);
            putn();
        }
            break;
    }
    if (r->int_no <= 18 && !no_kill) {
        dump_regs(r);
        if (umoude_con) {
            dprintf("Proc %x cause NO.%x fault.Kill it.PC:%x", getpid(), r->int_no, r->eip);
            send_sig(getpcb(getpid()), SIGSEGV);
            if (getpid() == 1) PANIC("Abort!");
            if (getpid() == 0) {
                dprintf("restart idle proc...");
                extern void create_idle_proc();
                create_idle_proc();
            }
            //handle signal~
            do_signal(getpcb(getpid()), r);
            do_schedule(NULL);
        }
        for (;;);
    }
    //handle signal~
    do_signal(getpcb(getpid()), r);
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