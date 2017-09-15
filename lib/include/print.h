//
// Created by dcat on 9/15/17.
//

#ifndef W2_PRINT_H
#define W2_PRINT_H

#include <intdef.h>
#include <file.h>

void fprintf(FILE *fp, const char *fmt, ...);

#define printf(fmt, args...) fprintf(stdout,fmt,##args);
#endif //W2_PRINT_H
