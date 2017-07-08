//
// Created by dcat on 3/12/17.
//

#ifndef W2_GDT_H
#define W2_GDT_H

#include "system.h"
#include "tss.h"

struct gdt_entry {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_middle;
    unsigned char access;
    unsigned char granularity;
    unsigned char base_high;
} __attribute__((packed));
struct gdt_entry_access {
    unsigned int accessed :1;
    unsigned int read_write :1; //readable for code, writable for data
    unsigned int conforming_expand_down :1; //conforming for code, expand down for data
    unsigned int code :1; //1 for code, 0 for data
    unsigned int always_1 :1; //should be 1 for everything but TSS and LDT
    unsigned int DPL :2; //priviledge level 0 = highest (kernel), 3 = lowest (user applications).
    unsigned int present :1;
}__attribute__((packed));

struct gdt_entry_granularity {
    unsigned int limit_high :4;
    unsigned int available :1;
    unsigned int always_0 :1; //should always be 0
    unsigned int big :1; //32bit opcodes for code, uint32_t stack for data
    unsigned int gran :1; //1 to use 4k page addressing, 0 for byte addressing
}__attribute__((packed));
struct gdt_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

typedef struct gdt_entry gdt_entry_t;
typedef struct gdt_ptr gdt_ptr_t;

void gdt_flush(gdt_entry_t *gdt, uint32_t size);

void gdt_install();

#endif //W2_GDT_H
