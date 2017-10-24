//
// Created by dcat on 9/2/17.
//
#ifndef KERNEL
#include "umem.h"

uchar_t *memcpy(void *dest, void *src, int count) {
    for (int x = 0; x < count; x++)
        ((uint8_t *) dest)[x] = ((uint8_t *) src)[x];
    return count;
}

uchar_t *dmemcpy(void *dest, void *src, int count) {
    for (int x = count - 1; x >= 0; x--)
        ((uint8_t *) dest)[x] = ((uint8_t *) src)[x];
    return count;
}

uchar_t *memset(void *dest, uint8_t val, int count) {
    for (int x = 0; x < count; x++)
        ((uint8_t *) dest)[x] = val;
    return NULL;
}

uchar_t memcmp(void *a1, void *a2, uint8_t len) {
    for (uint8_t x = 0; x < len; x++)
        if (((uint8_t *) a1)[x] != ((uint8_t *) a2)[x])
            return x;
    return 0;
}
#endif