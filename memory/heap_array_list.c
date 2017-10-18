//
// Created by dcat on 3/16/17.
//

#include <str.h>
#include <heap_array_list.h>
#include "include/heap_array_list.h"
#include "include/heap_array_list.h"
#include "include/kmalloc.h"

int clone_heap_array_list(heap_array_list_t *src, heap_array_list_t *target) {
    target->headers = (header_info_t *) kmalloc(sizeof(header_info_t) * src->max_size);
    target->size = src->size;
    target->max_size = src->max_size;
    memcpy(target->headers, src->headers, sizeof(header_info_t) * src->max_size);
    return 0;
}

int create_heap_array_list(heap_array_list_t *al, uint32_t max_size) {
    al->headers = (header_info_t *) kmalloc(sizeof(header_info_t) * max_size);
    al->size = 0;
    al->max_size = max_size;
    return 0;
}

int destory_heap_array_list(heap_array_list_t *al) {
    kfree(al->headers);
    return 0;
}

int insert_item_ordered(heap_array_list_t *al, header_info_t *info) {
    dprintf("insert %x %x %x", info->addr, info->used, info->size);
    ASSERT(al && info);
    bool inserted = false;
    int x = 0;
    for (; x < al->size + 1; x++) {
        if (x == al->size || info->size < al->headers[x].size) {
            //dumpint("insert into:", x);
            dmemcpy(&al->headers[x + 1], &al->headers[x], sizeof(header_info_t) * (al->size - x));
            memset(&al->headers[x], 0, sizeof(header_info_t));
            memcpy(&al->headers[x], info, sizeof(header_info_t));
            al->size++;
            if (al->size > al->max_size) {
                PANIC("Out of array.");
            }
            inserted = true;
            break;
        }
    }
    ASSERT(inserted);
    return x;
}

void remove_item(heap_array_list_t *al, int index) {
    ASSERT(al);
    ASSERT(index < al->size);
    //dumpint("remove:", index);
    if (index == al->size - 1) {
        memset(&al->headers[index], 0, sizeof(header_info_t));
    } else {
        memcpy(&al->headers[index], &al->headers[index + 1], sizeof(header_info_t) * (al->size - index - 1));
    }
    al->size--;
}

//寻找合适的ass hole
header_info_t *find_suit_hole(heap_array_list_t *al, uint32_t size, int *index) {
    ASSERT(al);
    for (int x = 0; x < al->size; x++) {
        if (!al->headers[x].used && size + HOLE_HEADER_SIZE + HOLE_FOOTER_SIZE < al->headers[x].size) {
            *index = x;
            return &al->headers[x];
        }
    }
    return NULL;
}
