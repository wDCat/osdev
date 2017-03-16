//
// Created by dcat on 3/15/17.
//

#ifndef W2_HEAP_H
#define W2_HEAP_H

#include "system.h"
#include "kmalloc.h"

#define HOLE_MAGIC 0xA6DCA606
typedef struct {
    uint32_t magic;
    uint32_t used;
    uint32_t size;
} hole_header_t;

typedef struct {
    uint32_t magic;
    hole_header_t *header;
} hole_footer_t;

typedef struct {
    uint32_t start_addr;
    uint32_t end_addr;
    uint32_t max_addr;
} heap_t;


#endif //W2_HEAP_H
