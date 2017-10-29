//
// Created by dcat on 10/28/17.
//

#ifndef W2_BLKQUEUE_H
#define W2_BLKQUEUE_H

#include "intdef.h"

#define BLKQUEUE_CHK 0x63FFFF54
typedef struct blkqueue_blk {
    uint32_t hchk;
    struct blkqueue_blk *next;
    //uint32_t entries
    //uint32_t fchk;
} blkqueue_blk_t;
typedef struct {
    uint32_t count;
    int perblk_entries_count;
    struct blkqueue_blk *first;
} blkqueue_t;
typedef struct {
    blkqueue_t *queue;
    blkqueue_blk_t *current;
    int inoff;
} blkqueue_iter_t;
#define blkqueue_count(sq) ((sq)->count)

int blkqueue_init(blkqueue_t *sq, int perblk_entry_count);

int blkqueue_destory(blkqueue_t *sq);

int blkqueue_set(blkqueue_t *ns, int index, uint32_t objaddr);

int blkqueue_insert(blkqueue_t *ns, uint32_t objaddr);

uint32_t blkqueue_get(blkqueue_t *sq, uint32_t index);

int blkqueue_remove(blkqueue_t *sq, int index);

bool blkqueue_isempty(blkqueue_t *sq);

int blkqueue_remove_vout(blkqueue_t *sq, int index, uint32_t *valout);

int blkqueue_iter_begin(blkqueue_iter_t *iter, blkqueue_t *ns);

uint32_t blkqueue_iter_next(blkqueue_iter_t *iter);

int blkqueue_iter_end(blkqueue_iter_t *iter);

#endif //W2_BLKQUEUE_H
