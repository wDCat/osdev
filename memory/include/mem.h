//
// Created by dcat on 9/2/17.
//

#ifndef W2_MEM_H
#define W2_MEM_H

#include "intdef.h"

uchar_t *memcpy(void *dest, void *src, int count);

uchar_t *dmemcpy(void *dest, void *src, int count);

uchar_t *memset(void *dest, uint8_t val, int count);

uchar_t memcmp(void *a1, void *a2, uint8_t len);

#endif //W2_MEM_H
