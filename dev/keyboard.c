//
// Created by dcat on 3/12/17.
//

#include <proc.h>
#include "../ker/include/irqs.h"
#include <str.h>
#include <io.h>
#include "tty/include/tty.h"
#include <keyboard.h>
#include "keyboard.h"

uchar_t kbdus[128] =
        {
                0, 27, '1', '2', '3', '4', '5', '6', '7', '8',    /* 9 */
                '9', '0', '-', '=', '\b',    /* Backspace */
                '\t',            /* Tab */
                'q', 'w', 'e', 'r',    /* 19 */
                't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',    /* Enter key */
                0,            /* 29   - Control */
                'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',    /* 39 */
                '\'', '`', 0,        /* Left shift */
                '\\', 'z', 'x', 'c', 'v', 'b', 'n',            /* 49 */
                'm', ',', '.', '/', 0,                /* Right shift */
                '*',
                0,    /* Alt */
                ' ',    /* Space bar */
                0,    /* Caps lock */
                0,    /* 59 - F1 key ... > */
                0, 0, 0, 0, 0, 0, 0, 0,
                0,    /* < ... F10 */
                0,    /* 69 - Num lock*/
                0,    /* Scroll Lock */
                0,    /* Home key */
                0,    /* Up Arrow */
                0,    /* Page Up */
                '-',
                0,    /* Left Arrow */
                0,
                0,    /* Right Arrow */
                '+',
                0,    /* 79 - End key*/
                0,    /* Down Arrow */
                0,    /* Page Down */
                0,    /* Insert Key */
                0,    /* Delete Key */
                0, 0, 0,
                0,    /* F11 Key */
                0,    /* F12 Key */
                0,    /* All other keys are undefined */
        };
uchar_t kbdusupper[128] =
        {
                0, 27, '!', '@', '#', '$', '%', '^', '&', '*',    /* 9 */
                '(', ')', '_', '+', '\b',    /* Backspace */
                '\t',            /* Tab */
                'Q', 'W', 'E', 'R',    /* 19 */
                'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',    /* Enter key */
                0,            /* 29   - Control */
                'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',    /* 39 */
                '\"', '~', 0,        /* Left shift */
                '|', 'Z', 'X', 'C', 'V', 'B', 'N',            /* 49 */
                'M', '<', '>', '?', 0,                /* Right shift */
                '*',
                0,    /* Alt */
                ' ',    /* Space bar */
                0,    /* Caps lock */
                0,    /* 59 - F1 key ... > */
                0, 0, 0, 0, 0, 0, 0, 0,
                0,    /* < ... F10 */
                0,    /* 69 - Num lock*/
                0,    /* Scroll Lock */
                0,    /* Home key */
                0,    /* Up Arrow */
                0,    /* Page Up */
                '-',
                0,    /* Left Arrow */
                0,
                0,    /* Right Arrow */
                '+',
                0,    /* 79 - End key*/
                0,    /* Down Arrow */
                0,    /* Page Down */
                0,    /* Insert Key */
                0,    /* Delete Key */
                0, 0, 0,
                0,    /* F11 Key */
                0,    /* F12 Key */
                0,    /* All other keys are undefined */
        };
kb_status_t kb_status;

void keyboard_handler(struct regs *r) {
    uint16_t sc;
    sc = (uint8_t) (inportb(0x60) & 0xFF);
    //dprintf("key down:%x", sc);
    switch (sc) {
        case KEY_RSHIFT_P:
        case KEY_LSHIFT_P:
            kb_status.shift = true;
            break;
        case KEY_LSHIFT_R:
        case KEY_RSHIFT_R:
            kb_status.shift = false;
            break;
        case KEY_LCTRL_P:
            kb_status.ctrl = true;
            break;
        case KEY_LCTRL_R:
            kb_status.ctrl = false;
            break;
        default:
            if (sc & 0x80) {
                //Control
            } else {
                tty_kb_handler(sc, &kb_status);
            }
    }
    on_keyboard_event(sc);
}

/* Installs the keyboard handler into IRQ1 */
void keyboard_install() {
    irq_install_handler(1, keyboard_handler);
}