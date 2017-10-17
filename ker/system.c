#include <str.h>
#include "include/system.h"


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

uint32_t get_eip() {
    __asm__ __volatile__ (""
            "pop %eax;"
            "jmp %eax;");
}

inline void dprint_(const char *str) {
    for (int x = 0; str[x] != '\0' && x < 0xFF; x++)
        serial_write(COM1, str[x]);
}

inline int get_interrupt_status() {
    extern bool _get_interrupt_status();
    return _get_interrupt_status();
}

inline void set_interrupt_status(int status) {
    if (status == 0)
            cli();
    else
            sti();
}