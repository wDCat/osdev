//
// Created by dcat on 3/16/17.
//

#include "include/heap_array_list.h"
#include "include/heap_array_list.h"
#include "include/kmalloc.h"

heap_array_list_t *create_heap_array_list(uint32_t max_size) {
    heap_array_list_t *alist = kmalloc(sizeof(heap_array_list_t));
    alist->headers = kmalloc(sizeof(header_info_t) * max_size);
    alist->size = 0;
    alist->max_size = max_size;
    return alist;
}

int insert_item_ordered(heap_array_list_t *al, header_info_t *info) {
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
header_info_t *find_suit_hole(heap_array_list_t *al, uint32_t size, bool page_align, int *index) {
    ASSERT(al);
    for (int x = 0; x < al->size; x++) {
        if (size < al->headers[x].size && !al->headers[x].used) {
            if (page_align && ((0x1000 - al->headers[x].addr & 0xFFF) + size >= al->headers[x].size)) {
                continue;
            }
            *index = x;
            return &al->headers[x];
        }
    }
    return NULL;
}
