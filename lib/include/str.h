//
// Created by dcat on 3/11/17.
//

#ifndef W2_STR_H
#define W2_STR_H

#include "va.h"
#include "intdef.h"

#define sprintf(out, fmt, arg...) strformat(out,fmt,##arg)
#define STR(x) (x)

uint strlen(const char *str);

int strstr(const char *str, const char *substr);

uint strformat(char *out, const char *fmt, ...);

char *strchr(const char *str, char c);

uint strcpy(char *target, const char *src);

int strcat(char *target, const char *src);

int strcats(char *target, const char *src, int maxlen);

uint strncpy(char *target, const char *src, int len);

bool str_compare(const char *s1, const char *s2);

char *strfmt_insspace(char *out, int tok_pos, int tok_len, int buff_len);

uint strformatw(void (*writer)(void *extern_data, char c), void *extern_data, const char *fmt, ...);

uint itohexs(uint32_t num, char *out);

uint itos(const int num, char *c2);

int strnxtok(const char *str, const char c, int start);

int atoi(const char *s);

#endif //W2_STR_H
