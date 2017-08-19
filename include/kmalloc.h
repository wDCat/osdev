//
// Created by dcat on 3/14/17.
//

#ifndef W2_KMALLOC_H
#define W2_KMALLOC_H

#include "intdef.h"
#include "contious_heap.h"
#include "paging_heap.h"

extern uint32_t end;
extern uint32_t heap_placement_addr;
extern heap_t *kernel_heap;
extern paging_heap_t *kernel_pheap;

void kmalloc_install();

void kfree(void *ptr);

uint32_t kmalloc_paging(uint32_t sz, uint32_t *phys);

uint32_t kmalloc_internal(uint32_t sz, bool align, uint32_t *phys, heap_t *heap);

#define kmalloc(sz) kmalloc_internal(sz,false,NULL,kernel_heap)
#define kmalloc_a(sz) kmalloc_internal(sz,true,NULL,kernel_heap)
#define kmalloc_p(sz, phy) kmalloc_internal(sz,false,phy,kernel_heap)
#define kmalloc_ap(sz, phy) kmalloc_internal(sz,true,phy,kernel_heap)

#endif //W2_KMALLOC_H
