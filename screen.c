//
// Created by dcat on 3/11/17.
//
#include <kmalloc.h>
#include "include/system.h"
#include "include/screen.h"
#include "include/str.h"

static bool key_waiter = 0;
int scrX = 0,
        scrY = 0;
uint8_t frontColor = COLOR_WHITE,
        backColor = COLOR_BLACK;

inline unsigned char *calcPos(int x, int y) {
    return (unsigned char *) (SCREEN_MEMORY_BASE + ((y * SCREEN_MAX_X) + x) * 2);
}

void moveCsr(void) {
    unsigned temp;
    temp = scrY * SCREEN_MAX_X + scrX;
    outportb(0x3D4, 14);
    outportb(0x3D5, temp >> 8);
    outportb(0x3D4, 15);
    outportb(0x3D5, temp);
}

void on_keyboard_event(int kc) {
    //ASSERT(key_waiter);
    key_waiter = true;
}

void pause() {
    key_waiter = false;
    putln_const("Press any key to continue...");
    while (!key_waiter) {
        k_delay(3000);
    }
    key_waiter = false;
}

int putc(char c_arg) {
    uint16_t data;
    char c = c_arg;
    switch (c) {
        case '\b'://Backspace
            scrX -= 1;
            if (scrX < 0) {
                scrY -= 1;
                scrX += SCREEN_MAX_X;
            }
            break;
        case '\n'://Enter
            scrX = 0;
            scrY += 1;
            scroll();
            break;
        default:
            data = (c & 0xFF);
            uint8_t *where = calcPos(scrX, scrY);
            *(where) = data;
            *(where + 1) = (backColor << 4 | (frontColor)) & 0xFF;
            scrX += 1;
    }
    if (scrX > SCREEN_MAX_X) {
        scrX = 0;
        scrY += 1;
        scroll();
    }
    moveCsr();
    return 1;
}

void screenSetColor(uint8_t fc, uint8_t bc) {
    frontColor = fc;
    backColor = bc;
}

int puts_(const char str[]) {
    int len = strlen(str);
    for (int x = 0; x < len; x++) {
        putc(str[x]);
    }
    return len;
}

void putint(int num) {
    putdec(num);
    /*
    char nstr[11];
    itoa(num, nstr);
    puts(nstr);*/
}

void screenClear() {
    memset(SCREEN_MEMORY_BASE, 0, SCREEN_MAX_Y * SCREEN_MAX_X * 2);
    scrY = 0;
    scrX = 0;
    moveCsr();
}

void puthex(uint32_t n) {
    uint32_t tmp;

    puts_const("0x");

    char noZeroes = 1;

    int i;
    for (i = 28; i > 0; i -= 4) {
        tmp = (n >> i) & 0xF;
        if (tmp == 0 && noZeroes != 0) {
            continue;
        }

        if (tmp >= 0xA) {
            noZeroes = 0;
            putc(tmp - 0xA + 'A');
        } else {
            noZeroes = 0;
            putc(tmp + '0');
        }
    }

    tmp = n & 0xF;
    if (tmp >= 0xA) {
        putc(tmp - 0xA + 'A');
    } else {
        putc(tmp + '0');
    }

}

void putdec(long long n) {

    if (n == 0) {
        putc('0');
        return;
    }
    //FIXME 65536
    int acc = (int) n;
    if (acc < 0) {
        putc('-');
        acc = -acc;
    }
    char c[32];
    int i = 0;
    while (acc > 0) {
        c[i] = (char) ('0' + acc % 10);
        acc /= 10;
        i++;
    }
    c[i] = 0;

    char c2[32];
    c2[i--] = 0;
    int j = 0;
    while (i >= 0) {
        c2[i--] = c[j++];
    }
    puts(c2);

}

inline void scroll() {
    if (scrY >= SCREEN_MAX_Y) {
        memcpy((unsigned char *) SCREEN_MEMORY_BASE,
               (const unsigned char *) (SCREEN_MEMORY_BASE + (1 * SCREEN_MAX_X) * 2),
               ((SCREEN_MAX_Y - 1) * SCREEN_MAX_X) * 2);
        memset((SCREEN_MEMORY_BASE + ((SCREEN_MAX_Y - 1) * SCREEN_MAX_X) * 2), 0, SCREEN_MAX_X * 2);
        scrY -= 1;
    }
}