//
// Created by dcat on 17-8-8.
//

#ifndef W2_PAGING_HEAP_H
#define W2_PAGING_HEAP_H

#include "intdef.h"
#include "system.h"

typedef struct {
    uint32_t start_addr;
    uint32_t size;
    uint32_t current;
} paging_heap_t;

paging_heap_t *pheap_create(uint32_t start_addr, uint32_t size);

uint32_t *pheap_alloc(paging_heap_t *heap, uint32_t size);

void pheap_free(paging_heap_t *heap, uint32_t *pointer);

#endif //W2_PAGING_HEAP_H
