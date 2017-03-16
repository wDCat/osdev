//
// Created by dcat on 3/14/17.
//

#include <system.h>
#include "include/intdef.h"
#include "include/kmalloc.h"


void kmalloc_install() {
    heap_placement_addr = (uint32_t) &end;
    //putint(heap_placement_addr);
}

uint32_t kmalloc_internal(uint32_t sz, bool align, uint32_t *phys) {
    ASSERT(heap_placement_addr != NULL);
    ASSERT(heap_placement_addr >= &end);

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
