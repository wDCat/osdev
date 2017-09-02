//
// Created by dcat on 9/2/17.
//

#ifndef W2_IO_H
#define W2_IO_H

#include "intdef.h"

unsigned char inportb(unsigned short _port);

void outportb(unsigned short _port, unsigned char _data);

void outportw(uint16_t portid, uint16_t value);

void outportl(uint16_t portid, uint32_t value);

void inportsl(uint16_t port, void *addr, unsigned long count);

#define outb(a, b) outportb(a,b)
#define inb(a) inportb(a)
#define insl(p, a, c) inportsl(p,a,c)
#endif //W2_IO_H
