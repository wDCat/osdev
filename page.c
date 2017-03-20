//
// Created by dcat on 3/14/17.
//

#include <kmalloc.h>
#include <idt.h>
#include <irqs.h>
#include <heap.h>
#include "include/page.h"


static uint32_t *frame_status = NULL;//保存frame状态的数组
static uint32_t frame_max_count = 0;//frame 计数
void set_frame_status(uint32_t frame_addr, uint32_t status) {
    uint32_t frame_no = frame_addr / 0x1000;//4k=0x1000
    ASSERT(frame_status != NULL);
    ASSERT(frame_max_count > frame_no);
    frame_status[frame_no] = status;
}

uint32_t get_frame_status(uint32_t frame_addr) {
    uint32_t frame_no = frame_addr / 0x1000;//4k=0x1000
    ASSERT(frame_status != NULL);
    ASSERT(frame_max_count > frame_no);
    return frame_status[frame_no];
}


uint32_t page_to_bit(page_t *p) {
    uint32_t r = 0;
    /*
    r |= p->present;
    r |= p->rw << 1;
    r |= p->user << 2;
    r |= p->write_through << 3;
    r |= p->cache_disabled << 4;
    r |= p->accessed << 5;
    r |= p->page_size << 7;
    r |= p->ignored << 8;
    r |= p->frame << 11;*/
    return r;
}

void alloc_frame(page_t *page, bool is_kernel, bool is_rw) {
    ASSERT(page);
    ASSERT(frame_status != NULL);
    if (!page->frame) {
        for (uint32_t x = 0; x < frame_max_count; x++) {
            if (frame_status[x] == FRAME_STATUS_FREE) {
                //dumpint("avaliable frame:", x);
                memset(page, 0, sizeof(page_t));
                page->present = true;
                page->frame = x;
                page->user = is_kernel ? 0 : 1;
                page->rw = is_rw;
                //dumphex("page addr", page);
                //uint32_t r = page_to_bit(page);
                //(*(uint32_t *) page) = r;
                //dumphex("page data:", (*(uint32_t *) page));
                set_frame_status(x * 0x1000, FRAME_STATUS_USED);
                break;
            } else if (x == frame_max_count) {
                //No more frame
                PANIC("No more frame.")
            }
        }
    }
}

void free_frame(page_t *page) {
    ASSERT(page);
    ASSERT(frame_status != NULL);
    if (!page->frame) {
        puterr_const("Try to free a null frame");
    } else {
        ASSERT(get_frame_status(page->frame * 0x1000) == FRAME_STATUS_USED);
        set_frame_status(page->frame * 0x1000, FRAME_STATUS_FREE);
        page->frame = NULL;
    }
}

void paging_install() {
    uint32_t mem_end_page = 0x1000000;
    frame_max_count = mem_end_page / 0x1000;
    dumpint("frame max count:", frame_max_count);
    frame_status = (uint32_t *) kmalloc(sizeof(uint32_t) * frame_max_count);
    memset(frame_status, 0, sizeof(uint32_t) * frame_max_count);
    kernel_dir = (page_directory_t *) kmalloc_a(sizeof(page_directory_t));
    memset(kernel_dir, 0, sizeof(page_directory_t));
    memset(kernel_dir->table_physical_addr, 0, sizeof(uint32_t) * 1024);
    memset(kernel_dir->tables, 0, sizeof(page_table_t *) * 1024);
    kernel_dir->physical_addr = 0;
    current_dir = kernel_dir;
    putln_const("");
    dumphex("heap placement start:", &end);
    dumphex("heap placement now:", heap_placement_addr);
    int i = 0;
    uint32_t kernel_area = heap_placement_addr;
    if (kernel_area < SCREEN_MEMORY_BASE + (SCREEN_MAX_X * SCREEN_MAX_Y) / 2) {
        kernel_area = SCREEN_MEMORY_BASE + (SCREEN_MAX_X * SCREEN_MAX_Y) / 2;
    }
    for (uint32_t i = KHEAP_START; i < KHEAP_START + KHEAP_SIZE + 0x1000; i += 0x1000) {
        get_page(i, true, kernel_dir);
    }
    for (uint32_t i = 0; i < kernel_area + 0x1000; i += 0x1000) {
        alloc_frame(get_page(i, true, kernel_dir), false, false);
    }
    for (uint32_t i = KHEAP_START; i < KHEAP_START + KHEAP_SIZE + 0x1000; i += 0x1000) {
        alloc_frame(get_page(i, true, kernel_dir), false, false);
    }
    dumphex("place:", heap_placement_addr);
/*
    for (uint32_t i = SCREEN_MEMORY_BASE & 0xFFFFF000;
         i < SCREEN_MEMORY_BASE + (SCREEN_MAX_X * SCREEN_MAX_Y) / 2; i += 0x1000) {
        alloc_frame(get_page(i, true, kernel_dir), false, false);
    }*/
    switch_page_directory(kernel_dir);
    kmalloc(1024);
}

void switch_page_directory(page_directory_t *dir) {
    current_dir = dir;
    //_switch_page_dir_internal(&dir->table_physical_addr);
    __asm__ __volatile__("mov %0, %%cr3"::"r"(&dir->table_physical_addr));
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0":"=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ __volatile__("mov %0, %%cr0"::"r"(cr0));

}

page_t *get_page(uint32_t addr, int make, page_directory_t *dir) {
    uint32_t frame_no = addr / 0x1000;//4K
    uint32_t table_idx = frame_no / 1024;//一个page table里有1024个page table entry
    if (dir->tables[table_idx]) {
        return &(dir->tables[table_idx]->pages[frame_no % 1024]);
    } else if (make) {
        uint32_t phyaddr;
        //dumpint("sizeof(page_table_t)", sizeof(page_table_t));
        dir->tables[table_idx] = (page_table_t *) kmalloc_ap(sizeof(page_table_t), &phyaddr);
        memset(dir->tables[table_idx], 0, 0x1000);
        dir->table_physical_addr[table_idx] = phyaddr | 0x7;
        return &(dir->tables[table_idx]->pages[frame_no % 1024]);
    } else {
        return NULL;
    }
}

void page_fault_handler(regs_t *r) {
    uint32_t faulting_address;
    __asm__ __volatile__("mov %%cr2, %0" : "=r" (faulting_address));

    // The error code gives us details of what happened.
    int present = !(r->err_code & 0x1); // Page not present
    int rw = r->err_code & 0x2;           // Write operation?
    int us = r->err_code & 0x4;           // Processor was in user-mode?
    int reserved = r->err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
    int id = r->err_code & 0x10;          // Caused by an instruction fetch?

    // Output an error message.
    puts_const("Page Fault! ( ");
    if (present) { puts_const("present "); }
    if (rw) { puts_const("read-only "); }
    if (us) { puts_const("user-mode "); }
    if (reserved) { puts_const("reserved "); }
    puts_const(") at 0x");
    puthex(faulting_address);
    putn();
}