//
// Created by dcat on 7/9/17.
//

#ifndef W2_BOCHS_DEBUG_H
#define W2_BOCHS_DEBUG_H
//outputs a character to the debug console
#define bochs_putc(c) outportb(0xe9, c)
//stops simulation and breaks into the debug console
#define bochs_break() outportw(0x8A00,0x8A00); outportw(0x8A00,0x08AE0);
#endif //W2_BOCHS_DEBUG_H
