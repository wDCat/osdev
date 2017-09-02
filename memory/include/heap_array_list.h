//
// Created by dcat on 3/16/17.
//

#ifndef W2_HEAP_ARRAY_LIST_H
#define W2_HEAP_ARRAY_LIST_H

#include "../../ker/include/system.h"
#include "intdef.h"


typedef struct {
    uint32_t addr;
    uint32_t size;
    bool used;
} header_info_t;
typedef struct {
    header_info_t *headers;
    uint32_t size;
    uint32_t max_size;
} heap_array_list_t;


extern void dump_al(heap_array_list_t *al);

extern heap_array_list_t *create_heap_array_list(uint32_t max_size);

extern int insert_item_ordered(heap_array_list_t *al, header_info_t *info);

extern void remove_item(heap_array_list_t *al, int index);

extern header_info_t *find_suit_hole(heap_array_list_t *al, uint32_t size, bool page_align, int *index);

#endif //W2_HEAP_ARRAY_LIST_H
