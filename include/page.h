//
// Created by dcat on 3/14/17.
//

#ifndef W2_PAGE_H
#define W2_PAGE_H

#include "system.h"
#include "intdef.h"

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

static page_directory_t *current_dir;
static page_directory_t *kernel_dir;

extern void _switch_page_dir_internal(uint32_t addr);

#define FRAME_STATUS_USED 1
#define FRAME_STATUS_FREE 0

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

page_table_t *clone_page_table(page_table_t *src, uint32_t *phy_out);

page_directory_t *clone_page_directory(page_directory_t *src);

void flush_TLB();
#endif //W2_PAGE_H
