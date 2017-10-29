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
}

int blkqueue_destory(blkqueue_t *sq) {

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
    uint32_t index = ns->count;
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

int blkqueue_remove(blkqueue_t *ns, int index) {
    if (index >= ns->count)
        return -1;
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
    //uint32_t *ptr = (uint32_t *) blk->next + 1 + inoff;
    memcpy((uint32_t) &blk->next + (1 + inoff) * sizeof(uint32_t),
           (uint32_t) &blk->next + (2 + inoff) * sizeof(uint32_t),
           blkqueue_blk_size(ns) - sizeof(blkqueue_blk_t) - (inoff) * sizeof(uint32_t));
    ns->count--;
    return 0;
    _err:
    return -1;
}

inline bool blkqueue_isempty(blkqueue_t *sq) {
    return sq->count == 0;
}

int blkqueue_remove_vout(blkqueue_t *sq, int index, uint32_t *valout) {

}

int blkqueue_iter_begin(blkqueue_iter_t *iter, blkqueue_t *ns) {

}

uint32_t blkqueue_iter_next(blkqueue_iter_t *iter) {

}

int blkqueue_iter_end(blkqueue_iter_t *iter) {

}