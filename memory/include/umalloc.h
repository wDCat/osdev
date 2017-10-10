//
// Created by dcat on 10/10/17.
//

#ifndef W2_UMALLOC_H
#define W2_UMALLOC_H

#include <proc.h>

void *umalloc(pid_t pid, uint32_t size);

void ufree(pid_t pid, void *ptr);

void *sys_malloc(uint32_t size);

void sys_free(void *ptr);

#endif //W2_UMALLOC_H
