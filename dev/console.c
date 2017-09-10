//
// Created by dcat on 9/8/17.
//

#include <mem.h>
#include <tty.h>
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
    uint16_t offset = con->start_addr - SCREEN_MEMORY_BASE;
    uint temp = (con->y) * SCREEN_MAX_X + con->x + 1;
    outportb(0x3D4, 0xC);
    outportb(0x3D5, offset >> 9);
    outportb(0x3D4, 0xD);
    outportb(0x3D5, 0xFF & (offset >> 1));
    outportb(0x3D4, 14);
    outportb(0x3D5, temp >> 8);
    outportb(0x3D4, 15);
    outportb(0x3D5, temp);

}

void console_clear(console_t *con) {
    memset(con->start_addr, 0, SCREEN_MAX_Y * SCREEN_MAX_X * 2);
    con->x = 0;
    con->y = 0;
    console_switch(con);
}

void console_scroll(console_t *con) {
    if (con->y >= SCREEN_MAX_Y) {
        memcpy((unsigned char *) con->start_addr,
               (const unsigned char *) (con->start_addr + (1 * SCREEN_MAX_X) * 2),
               ((SCREEN_MAX_Y - 1) * SCREEN_MAX_X) * 2);
        memset((con->start_addr + ((SCREEN_MAX_Y - 1) * SCREEN_MAX_X) * 2), 0, SCREEN_MAX_X * 2);
        con->y -= 1;
    }
}

void console_putc(console_t *con, const uchar_t c) {
    if (!con->present) {
        deprintf("try to write a not presented console.");
        return;
    }

    uint8_t data;
    uint8_t *where = (con->start_addr + ((con->y * SCREEN_MAX_X) + con->x) * 2);
    switch (c) {
        case '\b'://Backspace
            con->x -= 1;
            if (con->x < 0) {
                con->y -= 1;
                con->y += SCREEN_MAX_X;
            }
            *(where - 2) = 0;
            break;
        case '\n'://Enter
            con->x = 0;
            con->y += 1;
            console_scroll(con);
            break;
        default:
            data = (uint8_t) (c & 0xFF);
            *(where) = data;
            *(where + 1) = (COLOR_BLACK << 4 | (COLOR_WHITE)) & 0xFF;
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