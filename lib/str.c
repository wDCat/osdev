//
// Created by dcat on 3/11/17.
//

#include "str.h"

uint strlen(const char *str) {
    int retval;
    for (retval = 0; *str != '\0'; str++) retval++;
    return retval;
}

int strstr(const char *str, const char *substr) {
    int lstr = strlen(str);
    int lsubstr = strlen(substr);
    if (lsubstr > lstr)return -1;
    for (int x = 0; x < lstr; x++) {
        if (str[x] == substr[0]) {
            bool match = true;
            for (int y = 0; y < lsubstr; y++) {
                if (str[x + y] != substr[y]) {
                    match = false;
                    break;
                }
            }
            if (match)return x;
        }
    }
    return -1;
}

char *strchr(const char *str, char c) {
    int len = strlen(str);
    for (int x = 0; x < len; x++)
        if (str[x] == c)
            return (char *) (str + x);
    return NULL;
}

uint strcpy(char *target, const char *src) {
    int len = strlen(src);
    for (int x = 0; x < len + 1; x++) {
        target[x] = src[x];
    }
    return len;
}

uint strncpy(char *target, const char *src, int len) {
    for (int x = 0; x < len; x++) {
        target[x] = src[x];
    }
    return len;
}

uint strcat(char *target, const char *src) {
    int len = strlen(target);
    return strcpy(&target[len], src);
}

char *strfmt_insspace(char *out, int tok_pos, int tok_len, int buff_len) {
    int len = strlen(out);
    int movlen = buff_len - tok_len;
    if (movlen < 0) {
        for (int x = tok_pos + tok_len; x < len + 1; x++) {
            out[x + movlen] = out[x];
        }
    } else {
        for (int x = len; x >= tok_pos + tok_len; x--) {
            out[x + movlen] = out[x];
        }
    }
    return &out[tok_pos];
}

uint itos(const int num, char *c2) {
    //FIXME 65536


    if (num == 0) {
        strcpy(c2, STR("0"));
        return 0;
    }
    int acc = (int) num;
    bool neg = false;
    if (acc < 0) {
        acc = -acc;
        neg = true;
    }
    char c[32];
    int i = 0;
    while (acc > 0) {
        c[i] = (char) ('0' + acc % 10);
        acc /= 10;
        i++;
    }
    if (neg)
        c[i++] = '-';
    c[i] = 0;
    c2[i--] = 0;
    int j = 0;
    while (i >= 0) {
        c2[i--] = c[j++];
    }
    return 0;
}

uint itohexs(const int num, char *out) {
    out[0] = '\0';
    uint32_t tmp;
    strcat(out, STR("0x"));
    int len = 2;

    char noZeroes = 1;

    int i;
    for (i = 28; i > 0; i -= 4) {
        tmp = (num >> i) & 0xF;
        if (tmp == 0 && noZeroes != 0) {
            continue;
        }

        if (tmp >= 0xA) {
            noZeroes = 0;
            out[len++] = (tmp - 0xA + 'A');
        } else {
            noZeroes = 0;
            out[len++] = (tmp + '0');
        }
    }

    tmp = num & 0xF;
    if (tmp >= 0xA) {
        out[len++] = (tmp - 0xA + 'A');
    } else {
        out[len++] = (tmp + '0');
    }
    out[len] = '\0';
}

bool strcmp(const char *s1, const char *s2) {
    return strlen(s1) == strlen(s2) && strstr(s1, s2) == 0;
}

uint strformat(char *out, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    strcpy(out, fmt);
    int pos = 0;
    while (true) {
        char *nextmk = strchr(out + pos, '%');
        if (nextmk <= 0)break;
        int off = (int) (nextmk - out);
        pos += off;
        char tok = nextmk[1];
        switch (tok) {
            case 's': {
                char *data = va_arg(args, char*);
                int nlen = strlen(data);
                char *b = strfmt_insspace(out, off, 2, nlen);
                strncpy(b, data, nlen);
            }
                break;
            case 'd': {
                int n = va_arg(args, int);
                char c2[32];
                itos(n, c2);
                int nlen = strlen(c2);
                char *b = strfmt_insspace(out, off, 2, nlen);
                strncpy(b, c2, nlen);
            }
                break;
            case 'x': {
                int n = va_arg(args, int);
                char c2[32];
                itohexs(n, c2);
                int nlen = strlen(c2);
                char *b = strfmt_insspace(out, off, 2, nlen);
                strncpy(b, c2, nlen);
            }
                break;
            default:
                continue;
        }
    }
    va_end(args);
}

uint strformatw(void (*writer)(void *extern_data, char c), void *extern_data, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int pos = 0;
    while (true) {
        const char *les = &fmt[pos];
        char *nextmk = strchr(les, '%');
        if (nextmk <= 0)break;
        int off = (int) (nextmk - les);
        for (int x = pos; x < pos + off; x++)
            writer(extern_data, fmt[x]);
        pos += off + 1;
        char tok = nextmk[1];
        switch (tok) {
            case 's': {
                pos++;
                char *data = va_arg(args, char*);
                for (int x = 0; data[x] != '\0' && x < 0xFF; x++)
                    writer(extern_data, data[x]);
            }
                break;
            case 'd': {
                pos++;
                int n = va_arg(args, int);
                char c2[32];
                itos(n, c2);
                for (int x = 0; c2[x] != '\0' && x < 0x32; x++)
                    writer(extern_data, c2[x]);
            }
                break;
            case 'x': {
                pos++;
                int n = va_arg(args, int);
                char c2[32];
                itohexs(n, c2);
                for (int x = 0; c2[x] != '\0' && x < 0x32; x++)
                    writer(extern_data, c2[x]);
            }
                break;
            case 'c': {
                pos++;
                char n = va_arg(args, char);
                writer(extern_data, n);
            }
                break;
            default:
                continue;
        }
    }
    for (int x = pos; fmt[x] != '\0'; x++)
        writer(extern_data, fmt[x]);
    va_end(args);
}