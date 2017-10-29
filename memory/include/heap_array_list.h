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
    uint32_t trace_eip;
} header_info_t;
typedef struct {
    header_info_t *headers;
    uint32_t size;
    uint32_t max_size;
} heap_array_list_t;


void dump_al(heap_array_list_t *al);

int create_heap_array_list(heap_array_list_t *src, uint32_t max_size);

int insert_item_ordered(heap_array_list_t *al, header_info_t *info);

int destory_heap_array_list(heap_array_list_t *al);

void remove_item(heap_array_list_t *al, int index);

header_info_t *find_suit_hole(heap_array_list_t *al, uint32_t size, int *index);

int clone_heap_array_list(heap_array_list_t *src, heap_array_list_t *target);

#endif //W2_HEAP_ARRAY_LIST_H
