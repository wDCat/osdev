//
// Created by dcat on 3/14/17.
//

#include <kmalloc.h>
#include <idt.h>
#include <irqs.h>
#include <heap.h>
#include <page.h>
#include <str.h>
#include "include/page.h"

extern heap_t *kernel_heap;
uint32_t *frame_status = NULL;//保存frame状态的数组
uint32_t frame_max_count = 0;//frame 计数
page_directory_t *current_dir;
page_directory_t *kernel_dir;

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

/**为一个页分配一个frame*/
void alloc_frame(page_t *page, bool is_kernel, bool is_rw) {
    ASSERT(page);
    ASSERT(frame_status != NULL);
    if (!page->frame) {
        for (uint32_t x = 0; x < frame_max_count; x++) {
            if (frame_status[x] == FRAME_STATUS_FREE) {
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
                return;
                break;
            } else if (x == frame_max_count) {
                //No more frame
                PANIC("No more frame.")
            }
        }
        PANIC("Unknown Error")
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
    kernel_dir->physical_addr = (uint32_t) kernel_dir->table_physical_addr;
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
    //多分配10个页
    for (uint32_t i = 0; i < kernel_area + 0x30000; i += 0x1000) {
        alloc_frame(get_page(i, true, kernel_dir), false, false);
    }
    for (uint32_t i = KHEAP_START; i < KHEAP_START + KHEAP_SIZE + 0x1000; i += 0x1000) {
        alloc_frame(get_page(i, true, kernel_dir), false, false);
    }
    /*
    */
    dumphex("heap_placement_addr:", heap_placement_addr);
    switch_page_directory(kernel_dir);
    enable_paging();
    //FIXME 这条语句之后会导致alloc_frame 失效
    kernel_heap = create_heap(KHEAP_START, KHEAP_START + KHEAP_SIZE, KHEAP_START + KHEAP_SIZE * 2, kernel_dir);

}

void switch_page_directory(page_directory_t *dir) {
    current_dir = dir;
    //_switch_page_dir_internal(&dir->table_physical_addr);
    __asm__ __volatile__("mov %0, %%cr3"::"r"(dir->physical_addr));
    /*
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0":"=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ __volatile__("mov %0, %%cr0"::"r"(cr0));*/

}

page_t *get_page(uint32_t addr, int make, page_directory_t *dir) {
    uint32_t frame_no = addr / 0x1000;//4K
    uint32_t table_idx = frame_no / 1024;//一个page table里有1024个page table entry
    //putf_const("[GETP]%x %x\n", frame_no, table_idx);
    ASSERT(table_idx < 1024);
    if (dir->tables[table_idx]) {
        return &(dir->tables[table_idx]->pages[frame_no % 1024]);
    } else if (make) {
        uint32_t phyaddr;
        //dumpint("sizeof(page_table_t)", sizeof(page_table_t));
        dir->tables[table_idx] = (page_table_t *) kmalloc_internal(sizeof(page_table_t), true, &phyaddr, false);
        memset(dir->tables[table_idx], 0, sizeof(page_table_t));
        dir->table_physical_addr[table_idx] = phyaddr | 0x7;
        return &(dir->tables[table_idx]->pages[frame_no % 1024]);
    } else {
        return NULL;
    }
}

uint32_t get_physical_address(uint32_t va) {
    page_t *page = get_page(va, false, current_dir);
    if (page == NULL) {
        PANIC("No such page.");//Debug
        return NULL;
    }
    if (!page->frame) {
        PANIC("No allocated frame.");//Debug
        return NULL;
    }
    return (page->frame) * 0x1000 + (va & 0xFFF);
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
    puts_const(") at ");
    puthex(faulting_address);
    dumphex("\nEIP:", r->eip);
    putn();
}

void copy_frame_physical(uint32_t target, uint32_t src) {
    __asm__ __volatile__("cli;");
    //disable paging...
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0":"=r"(cr0));
    cr0 ^= 0x80000000;
    __asm__ __volatile__("mov %0, %%cr0"::"r"(cr0));
    uint32_t *src_p = (uint32_t *) (src * 0x1000);
    uint32_t *target_p = (uint32_t *) (target * 0x1000);
    for (int x = 0; x < 1024; x++) {
        target_p[x] = src_p[x];
    }
    cr0 |= 0x80000000;
    __asm__ __volatile__("mov %0, %%cr0"::"r"(cr0));
    __asm__ __volatile__("sti;");
}

page_table_t *clone_page_table(page_table_t *src, uint32_t *phy_out) {
    uint32_t phy_addr;
    page_table_t *target = (page_table_t *) kmalloc_ap(sizeof(page_table_t), &phy_addr);
    *phy_out = phy_addr;
    memset(target, 0, sizeof(page_table_t));
    for (int x = 0; x < 1024; x++) {
        if (src->pages[x].frame) {//Available
            alloc_frame(&target->pages[x], false, false);
            if (src->pages[x].present)target->pages[x].present = true;
            if (src->pages[x].rw)target->pages[x].rw = true;
            if (src->pages[x].user)target->pages[x].user = true;
            if (src->pages[x].accessed)target->pages[x].accessed = true;
            if (src->pages[x].dirty)target->pages[x].dirty = true;
            copy_frame_physical(target->pages[x].frame, src->pages[x].frame);
        }
    }
}

page_directory_t *clone_page_directory(page_directory_t *src) {
    uint32_t phy_addr;
    page_directory_t *target = (page_directory_t *) kmalloc_internal(sizeof(page_directory_t), true, &phy_addr, false);

    memset(target, 0, sizeof(page_directory_t));
    //calc offset 1024*pointer = 1K
    uint32_t offset = (uint32_t) target->table_physical_addr - (uint32_t) target;
    target->physical_addr = phy_addr + offset;
    putf_const("[PT]new table at:%x offset:%x ->phyaddr:%x\n", phy_addr, offset, target->physical_addr);
    uint8_t count[3];
    memset(count, 0, sizeof(uint8_t) * 3);
    for (int x = 0; x < 1024; x++) {
        if (src->tables[x]) {
            count[0]++;
            if (src->tables[x] == kernel_dir->tables[x]) {
                count[1]++;
                target->tables[x] = kernel_dir->tables[x];
                target->table_physical_addr[x] = kernel_dir->table_physical_addr[x];
            } else {
                count[2]++;
                uint32_t table_phy;
                page_table_t *table = clone_page_table(src->tables[x], &table_phy);
                target->tables[x] = table;
                target->table_physical_addr[x] = table_phy | 0x7;
            }
        }
    }
    putf_const("[PT]copied %x PTs [Kernel:%x]\n", count[0], count[1]);
    return target;
}

void flush_TLB() {
    __asm__ __volatile__("push %eax;"
            "mov %cr3,%eax;"
            "mov %eax,%cr3;"
            "pop %eax;");
}