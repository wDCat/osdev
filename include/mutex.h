//
// Created by dcat on 9/3/17.
//

#ifndef W2_MUTEX_H
#define W2_MUTEX_H

#include "intdef.h"

typedef struct {
    uint8_t mutex;
} mutex_lock_t;

void mutex_init(mutex_lock_t *m);

void mutex_unlock(mutex_lock_t *m);

#endif //W2_MUTEX_H
