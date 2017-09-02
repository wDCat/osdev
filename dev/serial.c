//
// Created by dcat on 8/27/17.
//

#include "io.h"
#include "serial.h"

void serial_install() {
    debug_serial_init(COM1);//COM1 for Qemu debug output.
}

void debug_serial_init(uint16_t port) {
    outportb(port + 1, 0x00);    // Disable all interrupts
    outportb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outportb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outportb(port + 1, 0x00);    //                  (hi byte)
    outportb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
    outportb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outportb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

bool serial_is_transmit_empty(uint16_t port) {
    return (bool) (inportb(port + 5) & 0x20);
}

void serial_write(uint16_t port, uint8_t _char) {
    while (serial_is_transmit_empty(port) == 0);
    outportb(port, _char);
}