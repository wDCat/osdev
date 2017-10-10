//
// Created by dcat on 3/14/17.
//

#ifndef W2_PAGE_H
#define W2_PAGE_H

#include "../../ker/include/system.h"
#include "intdef.h"
#define PAGE_SIZE (0x1000)
/*
typedef struct page_struct {
    uint32_t present:1;
    uint32_t rw:1;
    uint32_t user:1;
    uint32_t write_through:1;
    uint32_t cache_disabled:1;
    uint32_t accessed:1;
    uint32_t dirty:1;
    uint32_t page_size:1;
    uint32_t ignored:1;
    uint32_t av:3;
    uint32_t frame:20;
} page_t;
*/
typedef struct {
    uint32_t present    : 1;   // Page present in memory
    uint32_t rw         : 1;   // Read-only if clear, readwrite if set
    uint32_t user       : 1;   // Supervisor level only if clear
    uint32_t accessed   : 1;   // Has the page been accessed since last refresh?
    uint32_t dirty      : 1;   // Has the page been written to since last refresh?
    uint32_t unused     : 7;   // Amalgamation of unused and reserved bits
    uint32_t frame      : 20;  // Frame address (shifted right 12 bits)
} page_t;
typedef struct page_table_struct {
    page_t pages[1024];
}__attribute__((packed)) page_table_t;
typedef struct page_directory_struct {
    page_table_t *tables[1024];
    uint32_t table_physical_addr[1024];//上面tables的物理地址
    uint32_t physical_addr;//上面的物理地址
} page_directory_t;

extern page_directory_t *current_dir;
extern page_directory_t *kernel_dir;


#define FRAME_STATUS_USED 1
#define FRAME_STATUS_FREE 0
#define KHEAP_START 0xA000000
#define KHEAP_SIZE  0x50000
#define KPHEAP_START 0xA500000
#define KPHEAP_SIZE  0x50000
#define KCONTHEAP_SIZE  0x10000
#define disable_paging(){\
uint32_t cr0;\
__asm__ __volatile__("mov %%cr0, %0":"=r"(cr0));\
cr0 ^= 0x80000000;\
__asm__ __volatile__("mov %0, %%cr0"::"r"(cr0));\
}
#define enable_paging(){\
uint32_t cr0;\
__asm__ __volatile__("mov %%cr0, %0":"=r"(cr0));\
cr0 |= 0x80000000;\
__asm__ __volatile__("mov %0, %%cr0"::"r"(cr0));\
}

void set_frame_status(uint32_t frame_addr, uint32_t status);

uint32_t get_frame_status(uint32_t frame_addr);

void alloc_frame(page_t *page, bool is_kernel, bool is_rw);

void free_frame(page_t *page);

void alloc_frame(page_t *page, bool is_kernel, bool is_rw);

void paging_install();

void switch_page_directory(page_directory_t *dir);

page_t *get_page(uint32_t addr, int make, page_directory_t *dir);

void page_fault_handler(regs_t *r);

uint32_t page_to_bit(page_t *p);

//page_table_t *clone_page_table(page_table_t *src, uint32_t *phy_out);

page_directory_t *clone_page_directory(page_directory_t *src);

uint32_t get_physical_address(uint32_t va);

int32_t get_continuous_free_frame(uint32_t count);

uint32_t alloc_continuous_frame(uint32_t count, page_directory_t *dir);

page_directory_t *get_current_page_directory();

bool is_kernel_space(page_directory_t *dir, uint32_t addr);

int free_page_directory(page_directory_t *src);

void flush_TLB();

#endif //W2_PAGE_H
