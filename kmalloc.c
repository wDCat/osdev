//
// Created by dcat on 3/14/17.
//

#include <system.h>
#include "include/intdef.h"
#include "include/kmalloc.h"

heap_t *kernel_heap;
uint32_t heap_placement_addr;
extern heap_t *kernel_heap;
void kmalloc_install() {
    heap_placement_addr = (uint32_t) &end;
    //putint(heap_placement_addr);
}

uint32_t kmalloc_internal(uint32_t sz, bool align, uint32_t *phys) {
    ASSERT(heap_placement_addr != NULL);
    ASSERT(heap_placement_addr >= &end);
    if (kernel_heap) {
        void *ret = halloc(kernel_heap, sz, align);
        if (phys != 0) {
            page_t *page = get_page((uint32_t) ret, 0, kernel_dir);
            *phys = page->frame * 0x1000 + (uint32_t) ret & 0xFFF;
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
        /*
        puts_const("\nkmalloc:");
        puthex(tmp);
        puts_const("-->");
        puthex(heap_placement_addr);
        puts_const(" size:");
        puthex(sz);
        putln_const("");*/
        return tmp;
    }
}

void kfree(void *ptr) {
    ASSERT(kernel_heap);
    hfree(kernel_heap, ptr);
}
