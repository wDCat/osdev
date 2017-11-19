//
// Created by dcat on 3/12/17.
//

#include <proc.h>
#include "include/gdt.h"

gdt_entry_t gdt[9];
gdt_ptr_t gp;

/* Setup a descriptor in the Global Descriptor Table */
void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran) {
    /* Setup the descriptor base address */
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    /* Setup the descriptor limits */
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    /* Finally, set up the granularity and access flags */
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

void gdt_flush(gdt_entry_t *gdt_, uint32_t size) {
    gp.base = gdt_;
    gp.limit = (sizeof(gdt_entry_t) * size) - 1;
    extern _gdt_flush();
    _gdt_flush();
}

void gdt_install() {

    /* Our NULL descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(GDT_KERNEL_CODE, 0, 0xFFFFFFFF, 0x9A, 0xCF);// Kernel Code
    gdt_set_gate(GDT_KERNEL_DATA, 0, 0xFFFFFFFF, 0x92, 0xCF);// Kernel Data
    gdt_set_gate(GDT_USER_CODE, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User flags code segment
    gdt_set_gate(GDT_USER_DATA, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User flags data segment
    gdt_set_gate(GDT_TLS_ID, UM_STACK_START - sizeof(uint32_t), 0xFFFFFFFF, 0xF2, 0xCF);
    write_tss(TSS_ID, 0x10, 0);
    extern uint32_t _sys_stack;
    set_kernel_stack(_sys_stack);
    gdt_flush(gdt, 9);
    tss_flush();
}