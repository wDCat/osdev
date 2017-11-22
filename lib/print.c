//
// Created by dcat on 9/15/17.
//


#include <va.h>
#include <str.h>
#include <syscall.h>
#include <file.h>
#include "print.h"
#include "file.h"

void fprintf(FILE *fp, const char *fmt, ...) {
    int8_t fd = fp->fd;
    va_list args;
    va_start(args, fmt);
    int pos = 0;
    while (true) {
        const char *les = &fmt[pos];
        char *nextmk = strchr(les, '%');
        if (nextmk <= 0)break;
        int off = (int) (nextmk - les);
        syscall_write(fd, &fmt[pos], off);
        pos += off + 1;
        char tok = nextmk[1];
        switch (tok) {
            case 's': {
                pos++;
                char *data = va_arg(args, char*);
                int len = strlen(data);
                syscall_write(fd, data, len);
            }
                break;
            case 'd': {
                pos++;
                int n = va_arg(args, int);
                char c2[12];
                itos(n, c2);
                int len = strlen(c2);
                syscall_write(fd, c2, len);
            }
                break;
            case 'x': {
                pos++;
                long n = va_arg(args, long);
                char c2[12];
                itohexs(n, c2);
                int len = strlen(c2);
                syscall_write(fd, c2, len);
            }
                break;
            case 'c': {
                pos++;
                char n = va_arg(args, char);
                syscall_write(fd, &n, 1);
            }
                break;
            default:
                continue;
        }
    }
    int len = strlen(&fmt[pos]);
    syscall_write(fd, &fmt[pos], len);
    va_end(args);
}
