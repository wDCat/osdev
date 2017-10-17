//
// Created by dcat on 9/3/17.
//

#ifndef W2_MUTEX_H
#define W2_MUTEX_H

#include "intdef.h"

typedef struct {
    proc_queue_t wait_queue;
    uint32_t holdpid;
    uint8_t mutex;
} mutex_lock_t;

void mutex_init(mutex_lock_t *m);

void mutex_lock(mutex_lock_t *m);

void mutex_unlock(mutex_lock_t *m);

#endif //W2_MUTEX_H
