//
// Created by dcat on 3/14/17.
//

#include <system.h>
#include <page.h>
#include <str.h>
#include "include/intdef.h"
#include "include/kmalloc.h"

heap_t *kernel_heap = 0;
paging_heap_t *kernel_pheap = 0;
uint32_t heap_placement_addr;

void kmalloc_install() {
    heap_placement_addr = (uint32_t) &end;
    //putint(heap_placement_addr);
}

uint32_t kmalloc_paging(uint32_t sz, uint32_t *phys) {
    if (kernel_pheap) {
        uint32_t r = (uint32_t) pheap_alloc(kernel_pheap, sz);
        if (phys) {
            *phys = get_physical_address(r);
        }
        dprintf("allocated space:%x", r);
        return r;
    } else {
        dprintf("heap not present.");
        return kmalloc_internal(sz, true, phys, NULL);
    }
}

uint32_t kmalloc_internal(uint32_t sz, bool align, uint32_t *phys, heap_t *heap) {
    ASSERT(heap_placement_addr != NULL);
    ASSERT(heap_placement_addr >= &end);
    if (heap) {
        void *ret = halloc(heap, sz, align);
        if (!ret) PANIC("[kmalloc]Out of heap.");
        if (phys != 0) {
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
    ASSERT(kernel_pheap);
    dprintf("try to free:%x", ptr);
    if (KPHEAP_START < (uint32_t) ptr < KPHEAP_START + KPHEAP_SIZE) {
        pheap_free(kernel_pheap, ptr);
    } else
        deprintf("try to free a unknown[%x] memory space.", ptr);
}
