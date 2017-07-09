//
// Created by dcat on 3/14/17.
//

#include <system.h>
#include <page.h>
#include "include/intdef.h"
#include "include/kmalloc.h"

heap_t *kernel_heap;
uint32_t heap_placement_addr;
extern heap_t *kernel_heap;

void kmalloc_install() {
    heap_placement_addr = (uint32_t) &end;
    //putint(heap_placement_addr);
}

uint32_t kmalloc_internal(uint32_t sz, bool align, uint32_t *phys, bool with_paging/*for alloc_frame*/) {
    ASSERT(heap_placement_addr != NULL);
    ASSERT(heap_placement_addr >= &end);
    if (kernel_heap && with_paging) {
        void *ret = halloc(kernel_heap, sz, align);
        if (phys != 0) {
            //FIXME  错误的phys返回值  使用 get_physical_address  代替
            /*
            page_t *page = get_page((uint32_t) ret, 0, kernel_dir);
            ASSERT(page->frame);
            *phys = page->frame * 0x1000 + (uint32_t) ret & 0xFFF;*/
            *phys = get_physical_address(ret);
        }
        return ret;
    } else {
        if (align && (heap_placement_addr & 0xFFFFF000)) {
            // Align it.
            heap_placement_addr &= 0xFFFFF000;
            heap_placement_addr += 0x1000;
        }
        if (phys) {
            *phys = heap_placement_addr;
        }
        uint32_t tmp = heap_placement_addr;
        heap_placement_addr += sz;
        return tmp;
    }
}

void kfree(void *ptr) {
    ASSERT(kernel_heap);
    hfree(kernel_heap, ptr);
}
