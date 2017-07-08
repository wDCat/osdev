//
// Created by dcat on 7/5/17.
//

#include "tss.h"

tss_entry_t tss_entry;

void write_tss(uint32_t num, uint16_t ss0, uint32_t esp0) {
    // Firstly, let's compute the base and limit of our entry into the GDT.
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = base + sizeof(tss_entry);

    // Now, add our TSS descriptor's address to the GDT.
    gdt_set_gate(num, base, limit, 0xE9, 0x00);

    // Ensure the descriptor is initially zero.
    memset(&tss_entry, 0, sizeof(tss_entry));

    tss_entry.ss0 = ss0;  // Set the kernel stack segment.
    tss_entry.esp0 = esp0; // Set the kernel stack pointer.

    tss_entry.cs = 0x0b;
    tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x13;
}

void set_kernel_stack(uint32_t addr) {
    tss_entry.esp0 = addr;
}

void tss_flush() {
    extern void _tss_flush();
    //tss_flush();
    __asm__ __volatile__ ("mov $0x2B,%ax;"
            "ltr %ax");
}