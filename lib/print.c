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
    int8_t fd;
    if (fp < 3)
        fd = (int8_t) fp;
    else fd = fp->fd;
    va_list args;
    va_start(args, fmt);
    int pos = 0;
    while (true) {
        const char *les = &fmt[pos];
        char *nextmk = strchr(les, '%');
        if (nextmk <= 0)break;
        int off = (int) (nextmk - les);
        syscall_write(fd, off, &fmt[pos]);
        pos += off + 1;
        char tok = nextmk[1];
        switch (tok) {
            case 's': {
                pos++;
                char *data = va_arg(args, char*);
                int len = strlen(data);
                syscall_write(fd, len, data);
            }
                break;
            case 'd': {
                pos++;
                int n = va_arg(args, int);
                char c2[32];
                itos(n, c2);
                int len = strlen(c2);
                syscall_write(fd, len, c2);
            }
                break;
            case 'x': {
                pos++;
                int n = va_arg(args, int);
                char c2[32];
                itohexs(n, c2);
                int len = strlen(c2);
                syscall_write(fd, len, c2);
            }
                break;
            case 'c': {
                pos++;
                char n = va_arg(args, char);
                syscall_write(fd, 1, &n);
            }
                break;
            default:
                continue;
        }
    }
    int len = strlen(&fmt[pos]);
    syscall_write(fd, len, &fmt[pos]);
    va_end(args);
}
