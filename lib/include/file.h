//
// Created by dcat on 9/15/17.
//

#ifndef W2_FILE_H
#define W2_FILE_H

#include "intdef.h"

typedef struct {
    int8_t fd;
} FILE;
#define stdin 0
#define stdout 1
#define stderr 2
#endif //W2_FILE_H
