//
// Created by dcat on 9/2/17.
//

#include "io.h"

inline unsigned char inportb(unsigned short _port) {
    unsigned char rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

inline void outportb(uint16_t portid, uint8_t value) {
    __asm__ __volatile__("outb %%al, %%dx": :"d" (portid), "a" (value));
}

inline void outportw(uint16_t portid, uint16_t value) {
    __asm__ __volatile__("outw %%ax, %%dx": :"d" (portid), "a" (value));
}

inline void outportl(uint16_t portid, uint32_t value) {
    __asm__ __volatile__("outl %%eax, %%dx": :"d" (portid), "a" (value));
}

inline void inportsl(uint16_t port, void *addr, unsigned long count) {
    __asm__ __volatile__ (
    "cld ; rep ; insl "
    : "=D" (addr), "=c" (count)
    : "d"(port), "0"(addr), "1" (count)
    );
}