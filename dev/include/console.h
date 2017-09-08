//
// Created by dcat on 9/8/17.
//

#ifndef W2_CONSOLE_H
#define W2_CONSOLE_H
#define CONSOLE_MAX_COUNT 5
typedef struct {
    bool present;
    uint32_t start_addr;
    uint32_t x;
    uint32_t y;
} console_t;

void console_install();

console_t *console_alloc();

void console_putc(console_t *con, uchar_t c);

void console_puts(console_t *con, const uchar_t *str);

void console_putns(console_t *con, const uchar_t *str, int32_t len);

#endif //W2_CONSOLE_H
