//
// Created by dcat on 3/11/17.
//

#include "include/str.h"

uint strlen(const char *str) {
    int retval;
    for (retval = 0; *str != '\0'; str++) retval++;
    return retval;
}
