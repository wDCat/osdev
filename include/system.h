#ifndef __SYSTEM_H
#define __SYSTEM_H

#include "screen.h"
#include "bochs_debug.h"
#include "intdef.h"
#include "serial.h"

#define true 1
#define false 0
//String cannot be used as a argument directly.
#define STR(x) (const char[]) {x}
#define LOOP() for(;;);
#define BIT_GET(x, index) (((x)>>(index))&1)
#define BIT_SET(x, index) ((x)|=1<<(index))
#define BIT_CLEAR(x, index) ((x)&=~(1<<(index)))
#define MAX(a, b) ((a)>(b))?(a):(b)
#define MIN(a, b) ((a)>(b))?(b):(a)
#define ASSERT(x) {\
    if(!(x)){\
screenSetColor(COLOR_RED,COLOR_BLACK);\
puts_const("[ERROR]Assertion Failed:");\
puts_const(#x);\
puts_const(" at ");\
puts_const(__FILE__);\
puts_const(" : ");\
putint(__LINE__);\
putc('\n');\
for(;;);\
}\
}
#define ASSERTM(x, y) {\
    if(!(x)){\
screenSetColor(COLOR_RED,COLOR_BLACK);\
puts_const("[ERROR]Assertion Failed");\
puts_const("(");\
puts_const(y);\
puts_const("):");\
puts_const(#x);\
puts_const(" at ");\
puts_const(__FILE__);\
puts_const(" : ");\
putint(__LINE__);\
putc('\n');\
for(;;);\
}\
}
#define PANIC(x, args...) { \
screenSetColor(COLOR_RED,COLOR_BLACK);\
puts_const("[PANIC]");\
putf_const(x,##args);\
puts_const(" at ");\
puts_const(__FILE__);\
puts_const(" : ");\
putint(__LINE__);\
putc('\n');\
for(;;);\
}
#define cli() __asm__ __volatile__ ("cli")
#define sti() __asm__ __volatile__ ("sti")


typedef struct regs {
    unsigned int gs, fs, es, ds;      /* pushed the segs last */
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by 'pusha' */
    unsigned int int_no, err_code;    /* our 'push byte #' and ecodes do this */
    unsigned int eip, cs, eflags, useresp, ss;   /* pushed by the processor automatically */
} regs_t;
extern uint32_t init_esp;

/* MAIN.C */
extern unsigned char *memcpy(unsigned char *dest, const unsigned char *src, int count);

extern unsigned char *dmemcpy(unsigned char *dest, const unsigned char *src, int count);

extern unsigned char *memset(unsigned char *dest, unsigned char val, int count);

extern unsigned char inportb(unsigned short _port);

extern void outportb(unsigned short _port, unsigned char _data);

extern inline const char *itoa(int i, char result[]);

extern void k_delay(uint32_t time);

extern uint32_t get_eip();

uint8_t memcmp(uint8_t *a1, uint8_t *a2, uint8_t len);

//debug print
void dprint_(const char *str);

#define dprintf(str, args...) ({\
strformatw(serial_write, COM1,"[D][%s] " str "\n",__func__,##args);\
})
#define deprintf(str, args...) ({\
strformatw(serial_write, COM1,"[ERROR][%s] " str "\n",__func__,##args);\
})
#endif