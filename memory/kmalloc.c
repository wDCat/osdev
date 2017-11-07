//
// Created by dcat on 3/14/17.
//

#include "../ker/include/system.h"
#include "include/page.h"
#include <str.h>
#include "intdef.h"
#include "include/kmalloc.h"

heap_t *kernel_heap = 0;
paging_heap_t *kernel_pheap = 0;
uint32_t heap_placement_addr;

void kmalloc_install() {
    //heap_placement_addr = (uint32_t) &end;
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
        uint32_t ret = kmalloc_internal(sz, true, phys, NULL, NULL);
        dprintf("heap not present.call kmalloc instead,ret:%x", ret);
        return ret;
    }
}

void *kmalloc_type_impl(void *nouse, uint32_t sz) {
    return kmalloc_internal(sz, false, NULL, kernel_heap, 0x2333);
}

void kfree_type_impl(void *nouse, void *addr) {
    kfree(addr);
}

void *kmalloc_internal(uint32_t sz, bool align,
                       uint32_t *phys, heap_t *heap, uint32_t trace_eip) {
    ASSERT(heap_placement_addr != NULL);
    ASSERT(heap_placement_addr >= &end);
    if (heap) {
        void *ret = halloc_inter(heap, sz, align, trace_eip);
        if (!ret) {
            dump_al(&heap->al);
            PANIC("[kmalloc]Out of kernel heap.");
        }

        if (phys != 0) {
            *phys = get_physical_address(ret);
        }
        dprintf("kmalloc size:%x ret:%x", sz, ret);
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

    dprintf("try to free:%x", ptr);
    if ((uint32_t) KPHEAP_START < (uint32_t) ptr && (uint32_t) ptr < (uint32_t) KPHEAP_START + KPHEAP_SIZE) {
        ASSERT(kernel_pheap);
        pheap_free(kernel_pheap, ptr);
    } else if ((uint32_t) KHEAP_START < (uint32_t) ptr && ptr < KHEAP_START + KHEAP_SIZE) {
        ASSERT(kernel_heap);
        hfree(kernel_heap, ptr);
    } else
        deprintf("try to free a unknown[%x] memory space.", ptr);
}
