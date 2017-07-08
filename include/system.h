#ifndef __SYSTEM_H
#define __SYSTEM_H

#include "screen.h"

#include "intdef.h"

#define true 1
#define false 0
//String cannot be used as a argument directly.
#define STR(x) (const char[]) {x}
#define LOOP() for(;;);
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
#define PANIC(x) { \
screenSetColor(COLOR_RED,COLOR_BLACK);\
puts_const("[PANIC]");\
puts_const(x);\
puts_const(" at ");\
puts_const(__FILE__);\
puts_const(" : ");\
putint(__LINE__);\
putc('\n');\
for(;;);\
}
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

#endif