//
// Created by dcat on 8/27/17.
//

#ifndef W2_SERIAL_H
#define W2_SERIAL_H

#include <intdef.h>
#include "../../ker/include/system.h"

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

void serial_install();

void debug_serial_init(uint16_t port);

bool serial_is_transmit_empty(uint16_t port);

void serial_write(uint16_t port, uint8_t _char);

#endif //W2_SERIAL_H
