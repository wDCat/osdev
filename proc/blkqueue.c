//
// Created by dcat on 10/28/17.
//

#include <blkqueue.h>
#include <system.h>
#include <str.h>
#include <mem.h>
#include <kmalloc.h>

int blkqueue_init(blkqueue_t *sq, int perblk_entries_count) {
    sq->count = 0;
    sq->first = NULL;
    sq->perblk_entries_count = perblk_entries_count;
    sq->fboff = 0;
}

int blkqueue_destory(blkqueue_t *sq) {
    blkqueue_blk_t *blk = sq->first;
    while (blk != NULL) {
        blkqueue_blk_t *tfblk = blk;
        blk = blk->next;
        kfree(tfblk);
    }
    memset(sq, 0, sizeof(blkqueue_t));
    return 0;
}

int blkqueue_set(blkqueue_t *ns, int index, uint32_t objaddr) {

}

inline bool blkqueue_blk_chk(blkqueue_t *ns, blkqueue_blk_t *blk) {
    return blk->hchk == BLKQUEUE_CHK;
}

inline uint32_t blkqueue_blk_size(blkqueue_t *ns) {
    return ((uint32_t) sizeof(blkqueue_blk_t)) + (1 + ns->perblk_entries_count) * sizeof(uint32_t);
}

inline int blkqueue_blk_init(blkqueue_t *ns, blkqueue_blk_t *blk) {
    memset(blk, 0, blkqueue_blk_size(ns));
    blk->next = NULL;
    blk->hchk = BLKQUEUE_CHK;
    return 0;
}

int blkqueue_insert(blkqueue_t *ns, uint32_t objaddr) {
    uint32_t index = ns->count + ns->fboff;
    uint32_t blkno = index / ns->perblk_entries_count;
    uint32_t inoff = index % ns->perblk_entries_count;
    dprintf("index:%x pec:%x bno:%x inoff:%x", index, ns->perblk_entries_count, blkno, inoff);
    blkqueue_blk_t *blk = ns->first;
    for (int x = 0; x < blkno && blk != NULL; x++) {
        if (!blkqueue_blk_chk(ns, blk)) {
            deprintf("Bad blk:%x", blk);
            goto _err;
        }
        if (x == blkno - 1 && blk->next == NULL) {
            blkqueue_blk_t *nblk = (blkqueue_blk_t *) kmalloc(blkqueue_blk_size(ns));
            dprintf("create sub1 blk:%x", nblk);
            CHK(blkqueue_blk_init(ns, nblk), "");
            blk->next = nblk;
            blk = nblk;
            break;
        }
        blk = blk->next;
    }
    if (blk == NULL) {
        if (blkno == 0) {
            blkqueue_blk_t *nblk = (blkqueue_blk_t *) kmalloc(blkqueue_blk_size(ns));
            dprintf("create sub2 blk:%x", nblk);
            CHK(blkqueue_blk_init(ns, nblk), "");
            ns->first = nblk;
            blk = nblk;
        } else {
            deprintf("blk not found!!");
            goto _err;
        }
    }
    uint32_t *ptr = (uint32_t *) &blk->next + 1 + inoff;
    dprintf("ret addr:%x", ptr);
    *ptr = objaddr;
    ns->count++;
    return 0;
    _err:
    return -1;
}

uint32_t blkqueue_get(blkqueue_t *ns, uint32_t index) {
    if (index >= ns->count)
        return NULL;
    index += ns->fboff;
    uint32_t blkno = index / ns->perblk_entries_count;
    uint32_t inoff = index % ns->perblk_entries_count;
    blkqueue_blk_t *blk = ns->first;
    for (int x = 0; x < blkno && blk != NULL; x++) {
        if (!blkqueue_blk_chk(ns, blk)) {
            deprintf("Bad blk:%x", blk);
            goto _err;
        }
        blk = blk->next;
    }
    if (blk == NULL) {
        deprintf("blk not found!!");
        goto _err;
    }
    uint32_t *ptr = (uint32_t *) &blk->next + 1 + inoff;
    dprintf("ret addr:%x", ptr);
    return *ptr;
    _err:
    return NULL;
}

int blkqueue_remove_first(blkqueue_t *ns, uint32_t *valout) {
    blkqueue_blk_t *blk = ns->first;
    if (blk == NULL || ns->count < 1)return -1;
    if (*valout)
        *valout = *((uint32_t *) &blk->next + 1 + ns->fboff);
    ns->fboff++;
    ns->count--;
    if (ns->fboff == ns->perblk_entries_count) {
        ns->first = blk->next;
        ns->fboff = 0;
        kfree(blk);
    }
    return 0;
}

int blkqueue_remove_last(blkqueue_t *ns, uint32_t *valout) {
    uint32_t index = ns->count + ns->fboff - 1;
    uint32_t blkno = index / ns->perblk_entries_count;
    uint32_t inoff = index % ns->perblk_entries_count;
    dprintf("index:%x pec:%x bno:%x inoff:%x", index, ns->perblk_entries_count, blkno, inoff);
    blkqueue_blk_t *blk = ns->first;
    for (int x = 0; x < blkno && blk != NULL; x++) {
        if (!blkqueue_blk_chk(ns, blk)) {
            deprintf("Bad blk:%x", blk);
            goto _err;
        }
        blk = blk->next;
    }
    if (blk == NULL)goto _err;
    if (valout)
        *valout = *((uint32_t *) &blk->next + 1 + inoff);
    ns->count--;
    if (inoff == 0) {
        kfree(blk);
    }
    _err:
    return -1;
}

inline bool blkqueue_isempty(blkqueue_t *sq) {
    return sq->count == 0;
}


int blkqueue_iter_begin(blkqueue_iter_t *iter, blkqueue_t *ns) {
    iter->queue = ns;
    iter->current = ns->first;
    iter->inoff = ns->fboff;
    iter->count = 0;
}

uint32_t blkqueue_iter_next(blkqueue_iter_t *iter) {
    if (iter == NULL || iter->queue == NULL || iter->current == NULL || iter->count >= iter->queue->count)
        return NULL;
    blkqueue_blk_t *blk = iter->current;
    uint32_t ret = *((uint32_t *) &blk->next + 1 + iter->inoff);
    iter->count++;
    iter->inoff++;
    if (iter->inoff == iter->queue->perblk_entries_count) {
        iter->current = iter->current->next;
        iter->inoff = 0;
    }
    return ret;
}

int blkqueue_iter_end(blkqueue_iter_t *iter) {
    memset(iter, 0, sizeof(blkqueue_iter_t));
    return 0;
}