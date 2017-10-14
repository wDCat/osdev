//
// Created by dcat on 3/15/17.
//

#ifndef W2_HEAP_H
#define W2_HEAP_H

#include "../../ker/include/system.h"
#include "heap_array_list.h"

#define HOLE_HEADER_MAGIC 0xA6DCA606
#define HOLE_FOOTER_MAGIC 0xD3D3D3A3

typedef struct {
    uint32_t magic;
    uint32_t used;
    uint32_t size;
} hole_header_t;
#define HOLE_HEADER_SIZE sizeof(hole_header_t)
typedef struct {
    uint32_t magic;
    hole_header_t *header;
} hole_footer_t;
#define HOLE_FOOTER_SIZE sizeof(hole_footer_t)
typedef struct {
    uint32_t start_addr;
    uint32_t end_addr;
    uint32_t max_addr;
    heap_array_list_t al;
} heap_t;

heap_t *create_heap(heap_t *ret, uint32_t start_addr, uint32_t end_addr, uint32_t max_addr);

heap_t *clone_heap(heap_t *src, heap_t *target);

void *halloc(heap_t *heap, uint32_t size, bool page_align);

void hfree(heap_t *heap, uint32_t addr);

int destory_heap(heap_t *heap);

#endif //W2_HEAP_H
