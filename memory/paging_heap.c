//
// Created by dcat on 17-8-8.
//

#include <str.h>
#include "include/paging_heap.h"
#include "include/kmalloc.h"

paging_heap_t *pheap_create(uint32_t start_addr, uint32_t size) {
    paging_heap_t *heap = kmalloc(sizeof(paging_heap_t));
    if (start_addr & 0xFFF || size & 0xFFF) PANIC("start_addr and size must be page align.")
    ASSERT(heap->size < 0x1000 * 4 * 0x1000);
    heap->start_addr = start_addr;
    heap->size = size;
    heap->current = start_addr + 0x1000;
    memset(start_addr, 0, 0x1000);
    return heap;
}

void pheap_print_sum(paging_heap_t *heap) {
    int used = 0, free = 0;
    uint8_t *bitptr = (uint8_t *) heap->start_addr;
    for (int x = 0; x < heap->size / 0x1000; x++) {
        if (BIT_GET(bitptr[x / 8], x % 8)) {
            used++;
        } else {
            free++;
        }
    }
    dprintf("Heap Free:%d Used:%d", free, used);
}

uint32_t *pheap_alloc(paging_heap_t *heap, uint32_t size) {
    ASSERT(heap);
    if (size & 0xFFF) {
        size = (size & 0xFFFFF000) + 0x1000;//:<
    }
    uint32_t result;
    uint8_t *bitptr = (uint8_t *) heap->start_addr;
    uint8_t *endbitptr = (uint8_t *) heap->start_addr + 0x800;
    if ((heap->current + size) > (heap->start_addr + heap->size))
        heap->current = 0;
    uint8_t curbit = (heap->current - (heap->start_addr + 0x1000)) / 0x1000;
    uint8_t found = false, ft = false;

    while (!found) {
        found = true;
        if (curbit * 0x1000 + size > heap->size) {
            if (ft)goto out_of_heap;
            else {
                ft = true;
                curbit = 0;
            }
        }
        for (int x = curbit; x < curbit + (size / 0x1000); x++) {
            if (BIT_GET(bitptr[x / 8], x % 8)) {
                curbit = x + 1;
                if (curbit * 0x1000 > heap->size) {
                    if (ft) {
                        goto out_of_heap;
                    }
                    ft = true;
                    curbit = 0;
                }
                found = false;
                break;
            }
        }
    }

    for (int x = curbit; x < curbit + (size / 0x1000); x++) {
        BIT_SET(bitptr[x / 8], x % 8);
        BIT_CLEAR(endbitptr[x / 8], x % 8);
    }
    uint8_t endbitindex = curbit + (size / 0x1000) - 1;
    BIT_SET(endbitptr[endbitindex / 8], endbitindex % 8);
    result = heap->start_addr + 0x1000 + curbit * 0x1000;
    heap->current = heap->start_addr + 0x1000 + curbit * 0x1000 + size;
    dprintf("allocated %x size %x", result, size);
    pheap_print_sum(heap);
    return (uint32_t *) result;
    out_of_heap:
    pheap_print_sum(heap);
    PANIC("Out of pheap.")
    return 0;
}

void pheap_free(paging_heap_t *heap, uint32_t pointer) {
    if ((pointer & 0xFFF)
        || (pointer < heap->start_addr)
        || (pointer > heap->start_addr + heap->size)) PANIC("[pheap free]Bad Addr:%x", pointer);

    uint8_t *bitptr = (uint8_t *) heap->start_addr;
    uint8_t *endbitptr = heap->start_addr + 0x800;
    uint32_t x = (pointer - heap->start_addr - 0x1000) / 0x1000;
    //putf_const("[%x][%x]start x:%x\n", pointer, heap->start_addr, x);
    for (;; x++) {
        BIT_CLEAR(bitptr[x / 8], x % 8);
        if (BIT_GET(endbitptr[x / 8], x % 8)) {
            BIT_CLEAR(endbitptr[x / 8], x % 8);
            dprintf("free %x size %x", pointer, (uint32_t) heap->start_addr + 0x1000 + x * 0x1000 - pointer + 0x1000);
            //putf_const("[+]End Addr:%x\n", heap->start_addr + 0x1000 + x * 0x1000);
            break;
        }
    }
    pheap_print_sum(heap);
}