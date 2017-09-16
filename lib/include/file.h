//
// Created by dcat on 9/15/17.
//

#ifndef W2_FILE_H
#define W2_FILE_H

#include "intdef.h"

typedef struct {
    int8_t fd;
} FILE;
extern FILE *stdin, *stdout, *stderr;
#endif //W2_FILE_H
