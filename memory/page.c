//
// Created by dcat on 3/14/17.
//

#include "include/kmalloc.h"
#include "../ker/include/idt.h"
#include "../ker/include/irqs.h"
#include "include/contious_heap.h"
#include "include/page.h"
#include <str.h>
#include "include/page.h"

extern heap_t *create_heap(uint32_t start_addr, uint32_t end_addr, uint32_t max_addr, page_directory_t *dir);

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

int32_t get_free_frame() {
    for (uint32_t x = 0; x < frame_max_count; x++) {
        if (frame_status[x] == FRAME_STATUS_FREE) {
            return x;
        }
    }
    return -1;
}

int32_t get_continuous_free_frame(uint32_t count) {
    for (uint32_t x = 1; x < frame_max_count; x++) {
        bool found = true;
        for (uint32_t y = x; y <= x + count; y++) {
            if (frame_status[y] != FRAME_STATUS_FREE) {
                found = false;
                break;
            }
        }
        if (found)return x;
    }
    return -1;
}

uint32_t alloc_continuous_frame(uint32_t count, page_directory_t *dir) {
    int32_t x = get_continuous_free_frame(count);
    if (x < 0) {
        PANIC("No continuous free frame.")
    }
    for (uint32_t y = 0; y < count; y++) {
        page_t *page = get_page((x + y) * 0x1000, true, dir);
        ASSERT(page);
        ASSERT(page->frame == NULL);
        page->present = true;
        page->frame = (uint32_t) x + y;
        page->user = false;
        page->rw = true;
        //For kfree
        set_frame_status((uint32_t) ((x + y) * 0x1000), FRAME_STATUS_USED);
    }
    return (uint32_t) x;
}

void move_frame(uint32_t pm_src, uint32_t pm_target) {
    dprintf("Move frame[%x->%x]", pm_src, pm_target);
    disable_paging();
    ASSERT(pm_src - pm_target > 0x1000 && pm_target - pm_src > 0x10000);
    memcpy(pm_src, pm_target, 0x1000);
    memset(pm_src, 0, 0x1000);
    enable_paging();
}

/**为一个页分配一个frame*/
void alloc_frame(page_t *page, bool is_kernel, bool is_rw) {
    ASSERT(page);
    ASSERT(frame_status != NULL);
    if (!page->frame) {
        int32_t x = get_free_frame();
        if (x < 0) PANIC("No more free frame.");
        //memset(page, 0, sizeof(page_t));
        page->present = true;
        page->frame = (uint32_t) x;
        page->user = is_kernel ? 0 : 1;
        page->rw = is_rw;
        set_frame_status((uint32_t) (x * 0x1000), FRAME_STATUS_USED);
    }
}


void free_frame(page_t *page) {
    ASSERT(page);
    ASSERT(frame_status != NULL);
    if (!page->frame) {
        deprintf("Try to free a null frame");
    } else {
        ASSERT(get_frame_status(page->frame * 0x1000) != FRAME_STATUS_FREE);
        set_frame_status(page->frame * 0x1000, FRAME_STATUS_FREE);
        page->frame = NULL;
    }
}

void paging_install() {
    uint32_t mem_end_page = 0x1000000;
    frame_max_count = mem_end_page / 0x1000;
    dprintf("frame max count:%x", frame_max_count);
    frame_status = (uint32_t *) kmalloc(sizeof(uint32_t) * frame_max_count);
    memset(frame_status, 0, sizeof(uint32_t) * frame_max_count);
    kernel_dir = (page_directory_t *) kmalloc_a(sizeof(page_directory_t));
    memset(kernel_dir, 0, sizeof(page_directory_t));
    memset(kernel_dir->table_physical_addr, 0, sizeof(uint32_t) * 1024);
    memset(kernel_dir->tables, 0, sizeof(page_table_t *) * 1024);
    kernel_dir->physical_addr = (uint32_t) kernel_dir->table_physical_addr;
    uint32_t kernel_area = heap_placement_addr;
    dprintf("pag %x -> %x", KHEAP_START, KHEAP_START + KHEAP_SIZE + 0x1000);
    if (kernel_area < SCREEN_MEMORY_BASE + (SCREEN_MAX_X * SCREEN_MAX_Y) / 2) {
        kernel_area = SCREEN_MEMORY_BASE + (SCREEN_MAX_X * SCREEN_MAX_Y) / 2;
    }
    for (uint32_t i = KHEAP_START; i < KHEAP_START + KHEAP_SIZE + 0x1000; i += 0x1000) {
        get_page(i, true, kernel_dir);
    }
    for (uint32_t i = KPHEAP_START; i < KPHEAP_START + KPHEAP_SIZE + 0x1000; i += 0x1000) {
        get_page(i, true, kernel_dir);
    }
    //多分配10个页
    dprintf("pag %x -> %x\n", 0, kernel_area + 0x30000);
    for (uint32_t i = 0; i < kernel_area + 0x30000; i += 0x1000) {
        alloc_frame(get_page(i, true, kernel_dir), true, false);
    }
    for (uint32_t i = KHEAP_START; i < KHEAP_START + KHEAP_SIZE + 0x1000; i += 0x1000) {
        alloc_frame(get_page(i, true, kernel_dir), true, false);
    }
    for (uint32_t i = KPHEAP_START; i < KPHEAP_START + KPHEAP_SIZE + 0x1000; i += 0x1000) {
        alloc_frame(get_page(i, true, kernel_dir), true, false);
    }

    switch_page_directory(kernel_dir);
    enable_paging();
    flush_TLB();
    kernel_heap = create_heap(KHEAP_START, KHEAP_START + KHEAP_SIZE, KHEAP_START + KHEAP_SIZE * 2, kernel_dir);
    kernel_pheap = pheap_create(KPHEAP_START, KPHEAP_SIZE);
}

void switch_page_directory(page_directory_t *dir) {
    current_dir = dir;
    __asm__ __volatile__("mov %0, %%cr3"::"r"(dir->physical_addr));

}

page_t *get_page(uint32_t addr, int make, page_directory_t *dir) {
    uint32_t frame_no = addr / 0x1000;//4K
    uint32_t table_idx = frame_no / 1024;//一个page table里有1024个page table entry
    if (dir->tables[table_idx]) {
        return &(dir->tables[table_idx]->pages[frame_no % 1024]);
    } else if (make) {
        uint32_t phyaddr;
        //Use a empty page instead.
        //dir->tables[table_idx] = (page_table_t *) kmalloc_internal(sizeof(page_table_t), true, &phyaddr, kernel_heap);
        dir->tables[table_idx] = (page_table_t *) kmalloc_paging(sizeof(page_table_t), &phyaddr);
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
    char emsg[256];
    memset(emsg, 0, 256);
    // Output an error message.
    strcat(emsg, "Page Fault! ( ");
    if (present) { strcat(emsg, "present "); }
    if (rw) { strcat(emsg, "read-only "); }
    if (us) { strcat(emsg, "user-mode "); }
    if (reserved) { strcat(emsg, "reserved "); }
    strcat(emsg, ") at");
    deprintf("%s %x", emsg, faulting_address);
}

void copy_frame_physical(uint32_t src, uint32_t target) {
    extern void _copy_frame_physical(uint32_t, uint32_t);
    _copy_frame_physical(src * 0x1000, target * 0x1000);
    //Maybe it will work someday...:(
    //disable_paging();
    /*
    uint8_t *src_p = (uint8_t *) (src * 0x1000);
    uint8_t *target_p = (uint8_t *) (target * 0x1000);
    for (int x = 0; x < 0x1000; x++) {
        target_p[x] = src_p[x];
    }*/
    /*
    __asm__ ("cpy:"
            "mov (%%ebx),%%eax;"
            "mov %%eax,(%%edx);"
            "add $0x4,%%ebx;"
            "add $0x4,%%edx;"
            "loop cpy;"::"b"(src * 0x1000), "d"(target * 0x1000), "c"(1024)); */
    // enable_paging();
}

page_table_t *clone_page_table(page_table_t *src, uint32_t *phy_out, uint32_t num) {

    uint32_t phy_addr;
    page_table_t *target = (page_table_t *) kmalloc_paging(sizeof(page_table_t), &phy_addr);
    *phy_out = phy_addr;
    memset(target, 0, sizeof(page_table_t));
    for (int x = 0; x < 1024; x++) {

        if (src->pages[x].frame) {//Available
            alloc_frame(&target->pages[x], false, true);
            if (src->pages[x].present)target->pages[x].present = true;
            if (src->pages[x].rw)target->pages[x].rw = true;
            if (src->pages[x].user)target->pages[x].user = true;
            if (src->pages[x].accessed)target->pages[x].accessed = true;
            if (src->pages[x].dirty)target->pages[x].dirty = true;
            dprintf("copy[%x][%x]", num * 0x1000 * 0x1000 + x * 0x1000, target->pages[x].frame);
            copy_frame_physical(src->pages[x].frame, target->pages[x].frame);
        }
    }
    return target;
}

page_directory_t *clone_page_directory(page_directory_t *src) {
    uint32_t phy_addr;
    page_directory_t *target = (page_directory_t *) kmalloc_paging(sizeof(page_directory_t), &phy_addr);
    memset(target, 0, sizeof(page_directory_t));
    //calc offset 1024*pointer = 1K
    uint32_t offset = (uint32_t) target->table_physical_addr - (uint32_t) target;
    target->physical_addr = phy_addr + offset;
    dprintf("[PT]new table at:%x offset:%x ->phyaddr:%x", phy_addr, offset, target->physical_addr);
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
                page_table_t *table = clone_page_table(src->tables[x], &table_phy, x);
                target->tables[x] = table;
                target->table_physical_addr[x] = table_phy | 0x7;
            }
        }
    }
    dprintf("[PT]copied %x PTs [Kernel:%x]", count[0], count[1]);
    return target;
}

void flush_TLB() {
    __asm__ __volatile__(
    "mov %%cr3,%%eax;"
            "mov %%eax,%%cr3;"
    ::"a"(0));
}