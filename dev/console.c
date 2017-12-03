//
// Created by dcat on 9/8/17.
//

#include <mem.h>
#include "tty/include/tty.h"
#include <console.h>
#include <str.h>
#include <io.h>

console_t consoles[CONSOLE_MAX_COUNT];

void console_install() {
    memset(consoles, 0, sizeof(console_t) * CONSOLE_MAX_COUNT);
    uint32_t base = 0xB8000;
    for (int x = 0; x < CONSOLE_MAX_COUNT; x++) {
        consoles[x].present = 0;
        consoles[x].start_addr = base;
        base += SCREEN_MAX_X * SCREEN_MAX_Y * 2 + 2;
        dprintf("console %x start_addr:%x", x, consoles[x].start_addr);
    }
}

console_t *console_alloc() {
    for (int x = 0; x < CONSOLE_MAX_COUNT; x++) {
        if (!consoles[x].present) {
            consoles[x].present = true;
            return &consoles[x];
        }
    }
    return NULL;
}

void console_switch(console_t *con) {
    //TODO fast switch
    uint16_t offset = (uint16_t) (con->start_addr - SCREEN_MEMORY_BASE);
    uint temp = (con->y) * SCREEN_MAX_X + con->x;
    outportb(0x3D4, 0xA);
    outportb(0x3D5, 0 | 0x0B);
    outportb(0x3D4, 0xB);
    outportb(0x3D5, 0 | 0x0F);
    outportb(0x3D4, 0xC);
    outportb(0x3D5, offset >> 9);
    outportb(0x3D4, 0xD);
    outportb(0x3D5, 0xFF & (offset >> 1));
    outportb(0x3D4, 0xE);
    outportb(0x3D5, temp >> 8);
    outportb(0x3D4, 0xF);
    outportb(0x3D5, temp);

}

void console_clear(console_t *con) {
    memset((void *) con->start_addr, 0, SCREEN_MAX_Y * SCREEN_MAX_X * 2);
    con->x = 0;
    con->y = 0;
    console_switch(con);
}

void console_scroll(console_t *con) {
    if (con->y >= SCREEN_MAX_Y) {
        memcpy((void *) con->start_addr,
               (void *) (con->start_addr + (1 * SCREEN_MAX_X) * 2),
               ((SCREEN_MAX_Y - 1) * SCREEN_MAX_X) * 2);
        memset((void *) (con->start_addr + ((SCREEN_MAX_Y - 1) * SCREEN_MAX_X) * 2), 0, SCREEN_MAX_X * 2);
        con->y -= 1;
    }
}

#define ansi_color_to_vga(c) ((uint8_t) ((int[]) {0, 4, 2, 6, 1, 5, 3, 7})[(c) % 8])

int parse_control_str(console_t *con, char *str) {
    if (str[1] == '\0') {
        switch (str[0]) {
            case '0':
                con->control_mode.on = false;
                return 0;
        }
    }
    char *i = str;
    uint8_t fg = COLOR_WHITE, bg = COLOR_BLACK;
    bool light = false;
    while (*i) {
        if ((i = strchr(str, ';')) == NULL)
            i = &str[strlen(str)];
        else
            *i++ = '\0';
        int c = atoi(str);
        if (c >= 30 && c < 40) {
            fg = ansi_color_to_vga((uint8_t) (c - 30));
        } else if (c >= 40 && c < 50) {
            bg = ansi_color_to_vga((uint8_t) (c - 40));
        } else if (c == 1) light = true;
        str = i;
    }
    if (light) {
        fg += 8;
    }
    con->control_mode.color = (uint8_t) ((bg & 0xF) << 4 | (fg & 0xF));
    //dprintf("set color to %d fg:%d bg:%d", con->control_mode.color, fg, bg);
    con->control_mode.on = true;
    return 0;
}

inline uchar_t console_getc(console_t *con, int x, int y) {
    return *(uchar_t *) (con->start_addr + ((con->y * SCREEN_MAX_X) + con->x) * 2);
}

inline void console_setpos(console_t *con, uint32_t addr_offset) {
    uint32_t offset = addr_offset / 2;
    con->y = offset / SCREEN_MAX_X;
    con->x = offset % SCREEN_MAX_X;
}

void console_putc(console_t *con, const uchar_t c) {
    if (!con->present) {
        deprintf("try to write a not presented console.");
        return;
    }

    uint8_t data;
    bool doprint = false;
    uint8_t *where = (uint8_t *) (con->start_addr + ((con->y * SCREEN_MAX_X) + con->x) * 2);
    switch (c) {
        case '\b'://Backspace
            if (con->x == 0) {
                con->y -= 1;
                con->x = SCREEN_MAX_X;
            }
            con->x -= 1;
            *(where - 2) = 0;
            break;
        case '\n'://Enter
            con->x = 0;
            con->y += 1;
            console_scroll(con);
            break;
        case '\t':
            con->x += 4;
            break;
        case '\033':
            con->escape = (uint32_t) where;
            doprint = true;
            //dprintf("control char \\\\033 ");
            break;
        case '[':
            doprint = true;
            if ((uint32_t) where - con->escape != 2) {
                con->escape = NULL;
            }
            //dprintf("control char [ ");
            break;
        case 'm': {
            if (con->escape == NULL) {
                doprint = true;
                break;
            }
            if ((uint32_t) where - con->escape > 0x20) {
                con->escape = NULL;
                doprint = true;
                break;
            }

            //dprintf("control char m");
            char buff[((uint32_t) where - con->escape) / 2], *obuff = buff;
            for (uint8_t *x = (uint8_t *) (con->escape + 4); x < where; x += 2) {
                *obuff++ = *x;
            }
            *obuff = '\0';
            //dprintf("control str:%s", buff);
            if (parse_control_str(con, buff) == 0) {
                for (uint8_t *x = (uint8_t *) (con->escape); x < where; x++) {
                    *x = '\0';
                }
                console_setpos(con, con->escape - con->start_addr);
            }
            con->escape = NULL;
            break;
        }
        default:
            doprint = true;
    }
    if (doprint) {
        data = (uint8_t) (c & 0xFF);
        *(where) = data;
        if (con->control_mode.on) {
            *(where + 1) = con->control_mode.color;
        } else {
            *(where + 1) = (COLOR_BLACK << 4 | (COLOR_WHITE)) & 0xFF;
        }
        *(where + 3) = (COLOR_BLACK << 4 | (COLOR_WHITE)) & 0xFF;//to show cursor
        con->x += 1;
    }
    if (con->x >= SCREEN_MAX_X) {
        con->x = 0;
        con->y += 1;
        console_scroll(con);
    }
    console_switch(con);
}

void console_putns(console_t *con, const uchar_t *str, int32_t len) {
    for (int x = 0; x < len && str[x] != '\0'; x++) {
        console_putc(con, str[x]);
    }
}

void console_puts(console_t *con, const uchar_t *str) {
    for (int x = 0; x < 0xFF && str[x] != '\0'; x++) {
        console_putc(con, str[x]);
    }
}