//
// Created by dcat on 3/15/17.
//

#include "include/contious_heap.h"
#include "include/heap_array_list.h"
#include "include/page.h"
#include <str.h>
#include <contious_heap.h>
#include <proc.h>
#include <heap_array_list.h>
#include "include/contious_heap.h"
#include "include/kmalloc.h"

extern heap_t *kernel_heap;

inline void insert_default_header(heap_t *heap) {
    heap_array_list_t *al = &heap->al;
    header_info_t info;
    uint32_t max_size = heap->end_addr - heap->start_addr;
    info.size = max_size - HOLE_HEADER_SIZE - HOLE_FOOTER_SIZE;
    info.addr = heap->start_addr;
    info.used = false;
    hole_header_t *header = (hole_header_t *) heap->start_addr;
    hole_footer_t *footer = (hole_footer_t *) (heap->end_addr - HOLE_FOOTER_SIZE);
    header->magic = HOLE_HEADER_MAGIC;
    if (header->magic != HOLE_HEADER_MAGIC) {
        PANIC("Memory not writable.")
    }
    header->used = false;
    header->size = info.size;
    //dumphex("create footer addr:", footer);
    footer->magic = HOLE_FOOTER_MAGIC;
    footer->header = header;
    insert_item_ordered(al, &info);
}

heap_t *clone_heap(heap_t *src, heap_t *target) {
    target->start_addr = src->start_addr;
    target->end_addr = src->end_addr;
    target->max_addr = src->max_addr;
    clone_heap_array_list(&src->al, &target->al);
    mutex_init(&target->lock);
    return target;
}

heap_t *create_heap(heap_t *ret, uint32_t start_addr, uint32_t end_addr, uint32_t max_addr) {
    dprintf("create heap:%x -> %x\n", start_addr, end_addr);
    ASSERT(ret);
    ASSERT(end_addr > start_addr);
    ret->start_addr = start_addr;
    ret->end_addr = end_addr;
    ret->max_addr = max_addr;
    create_heap_array_list(&ret->al, 1024);
    insert_default_header(ret);
    mutex_init(&ret->lock);
    return ret;
}

int destory_heap(heap_t *heap) {
    ASSERT(heap);
    destory_heap_array_list(&heap->al);
    return 0;
}

void *halloc_inter(heap_t *heap, uint32_t size, bool page_align, uint32_t trace_eip) {
    if (page_align) PANIC("Use kmalloc_paging instead.");
    mutex_lock(&heap->lock);
    uint32_t ret;
    ASSERT(heap && size >= 0);
    int hinfo_index;
    header_info_t *hinfo = find_suit_hole(&heap->al, size, &hinfo_index);
    if (!hinfo) {
        deprintf("Out of heap.");
        mutex_unlock(&heap->lock);
        return NULL;
    }
    ASSERT(!hinfo->used);
    ASSERT(hinfo->size >= size);
    //dprintf("hole[%x][%x][%x] <== %x", hinfo->addr, hinfo->size, hinfo->used, size);
    hole_header_t *header = (hole_header_t *) hinfo->addr;
    hole_footer_t *footer = (hole_footer_t *) (hinfo->addr + HOLE_HEADER_SIZE + header->size);
    ASSERT(header->magic == HOLE_HEADER_MAGIC);
    ASSERT(footer->magic == HOLE_FOOTER_MAGIC);
    ASSERT(footer->header == header);
    if (hinfo->size == size) {
        header->used = true;
        hinfo->used = true;
        ret = ((uint32_t) hinfo->addr + HOLE_HEADER_SIZE);
        goto _ret;
    }
    //分配新的头和尾
    hole_header_t *new_header = (hole_header_t *) (hinfo->addr + HOLE_HEADER_SIZE + size + HOLE_FOOTER_SIZE);
    hole_footer_t *new_footer = (hole_footer_t *) (hinfo->addr + HOLE_HEADER_SIZE + size);

    new_footer->magic = HOLE_FOOTER_MAGIC;
    new_footer->header = header;
    new_header->magic = HOLE_HEADER_MAGIC;
    new_header->used = false;
    new_header->size = hinfo->size - size - HOLE_HEADER_SIZE - HOLE_FOOTER_SIZE;
    //dprintf("new_header addr:%x size:%x", new_header, new_header->size);
    //dprintf("new_footer addr:%x", new_footer);
    //Update
    header->size = size;
    footer->header = new_header;
    //插入到header表
    remove_item(&heap->al, hinfo_index);
    header_info_t h1info, h2info;//h1 for header,h2 for new_header
    h1info.addr = (uint32_t) header;
    h1info.size = header->size;
    h1info.used = true;
    h1info.trace_eip = trace_eip;
    h2info.addr = (uint32_t) new_header;
    h2info.size = new_header->size;
    h2info.used = false;
    h2info.trace_eip = 0;
    insert_item_ordered(&heap->al, &h1info);
    insert_item_ordered(&heap->al, &h2info);
    header->used = true;
    //memset(hinfo->addr + HOLE_HEADER_SIZE, 0, size);
    ret = ((uint32_t) header + HOLE_HEADER_SIZE);
    _ret:
    //dprintf("ret:%x", ret);
    mutex_unlock(&heap->lock);
    return (void *) ret;
}

inline void *halloc(heap_t *heap, uint32_t size, bool page_align) {
    return halloc_inter(heap, size, page_align, NULL);
}

hole_header_t *combine_two_hole(heap_t *heap, hole_header_t *h1, hole_header_t *h2) {
    ASSERT(heap);
    ASSERT(h1 < h2);
    ASSERT(!h1->used && !h2->used);
    hole_footer_t *footer2 = (hole_footer_t *) (((uint32_t) h2) + HOLE_HEADER_SIZE + h2->size);
    ASSERT(footer2->magic == HOLE_FOOTER_MAGIC);
    footer2->header = h1;
    h1->size = h1->size + h2->size + HOLE_HEADER_SIZE + HOLE_FOOTER_SIZE;
    h2->magic = NULL;
    //从索引中移除
    for (int x = 0; x < heap->al.size; x++) {
        if (heap->al.headers[x].addr == (uint32_t) h1 || heap->al.headers[x].addr == (uint32_t) h2) {
            remove_item(&heap->al, x);
            x--;
        }
    }
    return h1;

}

bool heap_check_hole(heap_t *heap, uint32_t start_addr) {
    for (int x = 0; x < heap->al.size; x++) {
        if (heap->al.headers[x].addr == (uint32_t) start_addr) {
            return true;
        }
    }
    return false;
}

void hfree(heap_t *heap, uint32_t addr) {
    ASSERT(heap);
    mutex_lock(&heap->lock);
    hole_header_t *header = (hole_header_t *) (addr - HOLE_HEADER_SIZE);
    hole_footer_t *footer = (hole_footer_t *) (addr + header->size);
    ASSERT(header->magic == HOLE_HEADER_MAGIC);
    ASSERT(footer->magic == HOLE_FOOTER_MAGIC);
    if (!header->used) {
        PANIC("try to free a unused hole:%x", addr);
        mutex_unlock(&heap->lock);
        return;
    }
    header->used = false;
    bool header_removed = false;
    //检查前一个hole
    if (addr - HOLE_HEADER_SIZE - HOLE_FOOTER_SIZE > heap->start_addr) {
        hole_footer_t *forward_footer = (hole_footer_t *) (addr - HOLE_HEADER_SIZE - HOLE_FOOTER_SIZE);
        ASSERT(forward_footer->magic == HOLE_FOOTER_MAGIC);
        hole_header_t *forward_header = forward_footer->header;
        ASSERT(forward_header->magic == HOLE_HEADER_MAGIC);
        if (!forward_header->used) {
            dprintf("combine forward header:%x", (uint32_t) forward_header);
            header = combine_two_hole(heap, forward_header, header);
            header_removed = true;
            footer = NULL;
        }
    }

    if ((uint32_t) header + header->size + HOLE_HEADER_SIZE + HOLE_FOOTER_SIZE < heap->end_addr) {
        hole_header_t *next_header = (hole_header_t *) ((uint32_t) header + HOLE_HEADER_SIZE + header->size +
                                                        HOLE_FOOTER_SIZE);
        hole_footer_t *next_footer = (hole_footer_t *) ((uint32_t) next_header + HOLE_HEADER_SIZE + next_header->size);
        if (next_header->magic == HOLE_HEADER_MAGIC && next_footer->magic == HOLE_FOOTER_MAGIC) {
            if (!next_header->used) {
                dprintf("combine next header:%x", next_header);
                header = combine_two_hole(heap, header, next_header);

                header_removed = true;
            }
        }
    }
    if (!header_removed) {
        for (int x = 0; x < heap->al.size; x++) {
            if (heap->al.headers[x].addr == (uint32_t) header) {
                remove_item(&heap->al, x);
                break;
            }
        }
    }
    header_info_t info;
    info.used = false;
    info.size = header->size;
    info.addr = (uint32_t) header;
    insert_item_ordered(&heap->al, &info);
    //dprintf("new size:%x", info.size);
    mutex_unlock(&heap->lock);
}