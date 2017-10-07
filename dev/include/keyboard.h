//
// Created by dcat on 3/12/17.
//

#ifndef W2_KEYBOARD_H
#define W2_KEYBOARD_H

#include "system.h"

#define KEY_LSHIFT_P 0x2A
#define KEY_RSHIFT_P 0x36
#define KEY_LSHIFT_R 0xAA
#define KEY_RSHIFT_R 0xB6
#define KEY_LCTRL_P 0x1D
#define KEY_RCTRL_P TODO
#define KEY_LCTRL_R 0x9D
#define KEY_RCTRL_R TODO
extern uchar_t kbdus[128];
extern uchar_t kbdusupper[128];
typedef struct {
    bool shift;
    bool ctrl;
    bool alt;
} kb_status_t;

void keyboard_handler(struct regs *r);

void keyboard_install();

#endif //W2_KEYBOARD_H
