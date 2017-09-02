//
// Created by dcat on 3/12/17.
//

#include <proc.h>
#include "../ker/include/irqs.h"
#include <str.h>
#include "keyboard.h"

//FIXME No work too... :(
void keyboard_handler(struct regs *r) {

    //putln("A BIGGGGGG MASK");
    int scancode;
    scancode = inportb(0x60);
    scancode = scancode & 0xFF;
    dprintf("key down:%x", scancode);
    if (scancode & 0x80) {
        //Control
    } else {
        //No work:
        putc(kbdus[scancode]);
        //No work v2:
        /*
        for (int x = 0; x < 0xFF; x++) {
            if (x == scancode) {
                putint(x);
                if (x == 2) {
                    putc(kbdus[x]);//Output right value
                } else {
                    putc(kbdus[x]);//Output holy shit code.
                }

                break;
            }
        }*/
        //For screen's pause
        on_keyboard_event(scancode);
        /*
        if (scancode < 10) {
            putint(scancode - 1);
        } else {
            switch (scancode) {
                case 10:
                    putint(9);
                    break;
                case 11:
                    putint(0);
                    break;
                case 14:
                    //putc('\b');
                    screenClear();
                    break;
                case 28:
                    putc('\n');
                    break;
                default:
                    putc('*');
            }
        }*/
    }
}

/* Installs the keyboard handler into IRQ1 */
void keyboard_install() {
    irq_install_handler(1, keyboard_handler);
}