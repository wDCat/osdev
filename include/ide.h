//
// Created by dcat on 3/21/17.
//

#ifndef W2_IDE_H
#define W2_IDE_H

#include "system.h"
#include "atadef.h"

#define outb(a, b) outportb(a,b)
#define inb(a) inportb(a)

static inline void outl(uint32_t value, uint16_t port);

static inline uint32_t inl(uint16_t port);

typedef struct {
    unsigned short base;  // I/O Base.
    unsigned short ctrl;  // Control Base
    unsigned short bmide; // Bus Master IDE
    unsigned char nIEN;  // nIEN (No Interrupt);
} ide_channel_register_t;

typedef struct {
    unsigned char reserved;    // 0 (Empty) or 1 (This Drive really exists).
    unsigned char channel;     // 0 (Primary Channel) or 1 (Secondary Channel).
    unsigned char drive;       // 0 (Master Drive) or 1 (Slave Drive).
    unsigned short type;        // 0: ATA, 1:ATAPI.
    unsigned short signature;   // Drive Signature
    unsigned short capabilities;// Features.
    unsigned int commandSets; // Command Sets Supported.
    unsigned int size;        // Size in Sectors.
    unsigned char model[41];   // Model in string.
} ide_device_t;

void ide_initialize(unsigned int BAR0, unsigned int BAR1, unsigned int BAR2, unsigned int BAR3,
                    unsigned int BAR4);

unsigned char ide_ata_access(unsigned char direction, unsigned char drive, unsigned int lba,
                             unsigned char numsects, unsigned short selector, unsigned int edi);

unsigned char ide_print_error(unsigned int drive, unsigned char err);

void ide_write(unsigned char channel, unsigned char reg, unsigned char data);

unsigned char ide_read(unsigned char channel, unsigned char reg);

#endif //W2_IDE_H
