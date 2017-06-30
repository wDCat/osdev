//
// Created by dcat on 3/11/17.
//

#ifndef W2_STR_H
#define W2_STR_H

#include "va.h"
#include "system.h"

uint strlen(const char *str);

int strstr(const char *str, const char *substr);

uint strformat(char *out, const char *fmt, ...);

char *strchr(const char *str, char c);

uint strcpy(char *target, const char *src);

uint strcat(char *target, const char *src);

uint strncpy(char *target, const char *src, int len);

char *strfmt_insspace(char *out, int tok_pos, int tok_len, int buff_len);

#endif //W2_STR_H
