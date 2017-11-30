//
// Created by dcat on 11/30/17.
//

#ifndef W2_OPEN_H
#define W2_OPEN_H

#include "intdef.h"

int8_t sys_open(const char *name, int flags);

int8_t sys_close(int8_t fd);

int8_t kclose(uint32_t pid, int8_t fd);

int8_t kopen(uint32_t pid, const char *name, int flags);

void kclose_all(uint32_t pid);

#endif //W2_OPEN_H
