//
// Created by dcat on 3/15/17.
//

#ifndef W2_HEAP_H
#define W2_HEAP_H

#include "system.h"
#include "heap_array_list.h"

#define HOLE_HEADER_MAGIC 0xA6DCA606
#define HOLE_FOOTER_MAGIC 0xD3D3D3A3

#define KHEAP_START 0xA0000000
#define KHEAP_SIZE  0x800000
#define KCONTHEAP_SIZE  0x10000
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
    heap_array_list_t *al;
} heap_t;


void *halloc(heap_t *heap, uint32_t size, bool page_align);

void hfree(heap_t *heap, uint32_t addr);

#endif //W2_HEAP_H
