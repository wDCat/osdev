//
// Created by dcat on 3/11/17.
//
#include "system.h"
#include "intdef.h"

#ifndef W2_SCREEN_H
#define W2_SCREEN_H

#define SCREEN_MEMORY_BASE 0xB8000
#define BLACK (0x20 | (0x0F << 8))
#define SCREEN_MAX_X 80
#define SCREEN_MAX_Y 25
#define COLOR_BLACK 0x0
#define COLOR_WHITE 0xF
#define COLOR_RED 0x4
//For const string. like "Hello world"
#define puts_const(x) puts_(STR(x))

#define putln_const(x) {puts_const(x);putc('\n');}
#define puterr_const(x) {screenSetColor(COLOR_RED,COLOR_BLACK);puts_const(x);putc('\n');}

//For var. like itoa(12)
#define puts(x) puts_(x)
#define putln(x) {puts(x);putc('\n');}
#define puterr(x) {screenSetColor(COLOR_RED,COLOR_BLACK);puts(x);putc('\n');}
#define putn() putc('\n')
#define dumphex(name, x) {puts_const(name); \
    puthex(x); \
putn(); }
#define dumpint(name, x) {puts_const(name); \
    putdec(x); \
putn(); }

#define putnf(fmt, size, args...){\
 char data[size];\
 strformat(data,fmt,##args);\
puts(data);\
}
#define putf(fmt, args...) putnf(fmt,256,##args)
#define putf_const(fmt, args...) putnf(STR(fmt),256,##args)
typedef unsigned char uint8_t;

void putint(int num);

int putc(char c);

int puts_(const char str[]);

void screenSetColor(uint8_t fc, uint8_t bc);

void screenClear();

void puthex(uint32_t n);

void putdec(long long n);

void pause();

void on_keyboard_event(int kc);

#endif //W2_SCREEN_H
