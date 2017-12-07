#ifndef __SYSTEM_H
#define __SYSTEM_H

#include "debug_print.h"
#include "uproc.h"
#include "screen.h"
#include "bochs_debug.h"
#include "intdef.h"
#include "serial.h"
#include "mem.h"

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
deprintf("Assertion Failed:%s at %s:%d",#x,__FILE__,__LINE__);\
putf("[ERROR]Assertion Failed:" #x " at %s:%d\n",__FILE__,__LINE__);\
for(;;);\
}\
}
#define ASSERTM(x, y) {\
    if(!(x)){\
screenSetColor(COLOR_RED,COLOR_BLACK);\
deprintf("Assertion Failed(%s):%s at %s:%d",#x,y,__FILE__,__LINE__);\
putf("[ERROR]Assertion Failed(%s):" #x " at %s:%d\n",y,__FILE__,__LINE__);\
for(;;);\
}\
}
#define PANIC(x, args...) { \
screenSetColor(COLOR_RED,COLOR_BLACK);\
deprintf("PANIC:%s at %s:%d",#x,__FILE__,__LINE__);\
putf("[PANIC]" #x " at %s:%d\n",##args,__FILE__,__LINE__);\
for(;;);\
}
#define cli() __asm__ __volatile__ ("cli")
#define sti() __asm__ __volatile__ ("sti")


typedef struct regs {
    uint32_t gs, fs, es, ds;      /* pushed the segs last */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by 'pusha' */
    uint32_t int_no, err_code;    /* our 'push byte #' and ecodes do this */
    uint32_t eip, cs, eflags, useresp, ss;   /* pushed by the processor automatically */
} regs_t;
extern uint32_t init_esp;
extern pid_t current_pid;

/* MAIN.C */

const char *itoa(int i, char result[]);

void k_delay(uint32_t time);

uint32_t get_eip();

int get_interrupt_status();

void set_interrupt_status(int status);

#define ssti(iflag) ({*(iflag)=get_interrupt_status();sti();})
#define scli(iflag) ({*(iflag)=get_interrupt_status();cli();})
#define srestorei(iflag) ({set_interrupt_status(*(iflag));})

//debug print

#define CHK(fun, msg, args...) if(fun){\
deprintf("CHK::" #fun " Failed:" msg,##args);\
goto _err;\
}

typedef void *(*malloc_t)(void *extern_data, uint32_t size);

typedef void(*mfree_t)(void *extern_data, void *addr);

#endif