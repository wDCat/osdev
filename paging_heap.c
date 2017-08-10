//
// Created by dcat on 17-8-8.
//

#include "paging_heap.h"
#include "kmalloc.h"

paging_heap_t *pheap_create(uint32_t start_addr, uint32_t size) {
    paging_heap_t *heap = kmalloc(sizeof(paging_heap_t));
    if (start_addr & 0xFFF || size & 0xFFF) PANIC("start_addr and size must be page align.")
    heap->current = heap->start_addr = start_addr;
    heap->size = size;
    return heap;
}

uint32_t *pheap_alloc(paging_heap_t *heap, uint32_t size) {
    ASSERT(heap);
    if (size & 0xFFF)
        size = size & 0xFFFFF000 + 0x1000;
    uint32_t *result = heap->current;
    heap->current += size;
    return result;
}

void pheap_free(paging_heap_t *heap, uint32_t *pointer) {
    PANIC("//TODO");
}