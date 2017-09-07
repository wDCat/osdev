//
// Created by dcat on 9/7/17.
//

#include <io.h>
#include <system.h>
#include <str.h>
#include "vga.h"

void vga_write_regs(unsigned char *regs) {
    unsigned i;

/* write MISCELLANEOUS reg */
    outportb(VGA_MISC_WRITE, *regs);
    regs++;
/* write SEQUENCER regs */
    for (i = 0; i < VGA_NUM_SEQ_REGS; i++) {
        outportb(VGA_SEQ_INDEX, i);
        outportb(VGA_SEQ_DATA, *regs);
        regs++;
    }
/* unlock CRTC registers */
    outportb(VGA_CRTC_INDEX, 0x03);
    outportb(VGA_CRTC_DATA, inportb(VGA_CRTC_DATA) | 0x80);
    outportb(VGA_CRTC_INDEX, 0x11);
    outportb(VGA_CRTC_DATA, inportb(VGA_CRTC_DATA) & ~0x80);
/* make sure they remain unlocked */
    regs[0x03] |= 0x80;
    regs[0x11] &= ~0x80;
/* write CRTC regs */
    for (i = 0; i < VGA_NUM_CRTC_REGS; i++) {
        outportb(VGA_CRTC_INDEX, i);
        outportb(VGA_CRTC_DATA, *regs);
        regs++;
    }
/* write GRAPHICS CONTROLLER regs */
    for (i = 0; i < VGA_NUM_GC_REGS; i++) {
        outportb(VGA_GC_INDEX, i);
        outportb(VGA_GC_DATA, *regs);
        regs++;
    }
/* write ATTRIBUTE CONTROLLER regs */
    for (i = 0; i < VGA_NUM_AC_REGS; i++) {
        (void) inportb(VGA_INSTAT_READ);
        outportb(VGA_AC_INDEX, i);
        outportb(VGA_AC_WRITE, *regs);
        regs++;
    }
/* lock 16-color palette and unblank display */
    (void) inportb(VGA_INSTAT_READ);
    outportb(VGA_AC_INDEX, 0x20);
}

uchar_t g_text[] =
        {
/* MISC */
                0xE7,
/* SEQ */
                0x03, 0x01, 0x03, 0x00, 0x02,
/* CRTC */
                0x6B, 0x59, 0x5A, 0x82, 0x60, 0x8D, 0x0B, 0x3E,
                0x00, 0x50, 0x06, 0x07, 0x00, 0x00, 0x00, 0x00,
                0xEA, 0x0C, 0xDF, 0x2D, 0x08, 0xE8, 0x05, 0xA3,
                0xFF,
/* GC */
                0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
                0xFF,
/* AC */
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
                0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
                0x0C, 0x00, 0x0F, 0x08, 0x00,
        };

void vga_install() {
    vga_write_regs(g_text);
}