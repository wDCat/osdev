//
// Created by dcat on 3/12/17.
//

#ifndef W2_KEYBOARD_H
#define W2_KEYBOARD_H

#include "system.h"

extern uchar_t kbdus[128];

void keyboard_handler(struct regs *r);

void keyboard_install();

#endif //W2_KEYBOARD_H
