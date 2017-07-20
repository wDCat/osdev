#include "system.h"


unsigned char *memcpy(unsigned char *dest, const unsigned char *src, int count) {
    for (int x = 0; x < count; x++) {
        dest[x] = src[x];
    }
}

unsigned char *dmemcpy(unsigned char *dest, const unsigned char *src, int count) {

    for (int x = count - 1; x >= 0; x--) {
        dest[x] = src[x];
    }
}

unsigned char *memset(unsigned char *dest, unsigned char val, int count) {
    for (int x = 0; x < count; x++) {
        dest[x] = val;
    }
    return NULL;
}

unsigned char inportb(unsigned short _port) {
    unsigned char rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

void outportb(uint16_t portid, uint8_t value) {
    __asm__ __volatile__("outb %%al, %%dx": :"d" (portid), "a" (value));
}

void outportw(uint16_t portid, uint16_t value) {
    __asm__ __volatile__("outw %%ax, %%dx": :"d" (portid), "a" (value));
}

void outportl(uint16_t portid, uint32_t value) {
    __asm__ __volatile__("outl %%eax, %%dx": :"d" (portid), "a" (value));
}

void k_delay(uint32_t time) {
    for (int x = 0; x < time * 1000; x++) {
        for (int y = 0; y < 100000; y++) {
            int al = 2, bl = 4;
            if (al + bl == 1024) {
                break;
            }
            __asm__ __volatile__("nop");
        }
    }
}

inline const char *itoa(int i, char result[]) {
    int po = i;
    int x = 9;
    result[10] = '\0';
    for (; x >= 0; x--) {
        int l = po % 10;
        result[x] = l + 0x30;
        po /= 10;
        if (po == 0) {
            memcpy(result, &result[x], x + 1);
            break;
        }
    }
    return &result[x];
}