//
// Created by dcat on 10/14/17.
//

#ifndef W2_SQUEUE_H
#define W2_SQUEUE_H

#include <umem.h>

typedef struct squeue_entry {
    uint32_t objaddr;
    struct squeue_entry *next;
} squeue_entry_t;
typedef struct {
    int count;
    struct squeue_entry *first;
} squeue_t;
#define squeue_count(sq) ((sq)->count)

int squeue_init(squeue_t *sq);

int squeue_set(squeue_t *ns, int index, uint32_t objaddr);

int squeue_insert(squeue_t *ns, uint32_t objaddr);

uint32_t squeue_get(squeue_t *sq, int index);

int squeue_remove(squeue_t *sq, int index);

bool squeue_isempty(squeue_t *sq);

int squeue_remove_vout(squeue_t *sq, int index, uint32_t *valout);

#define SQUEUE_GET(sq, index, type) ((type)squeue_get(sq,index))
#endif //W2_SQUEUE_H
