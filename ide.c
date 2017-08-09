//
// Created by dcat on 3/21/17.
//

#include <ide.h>
#include "ide.h"

ide_channel_register_t channels[2];
ide_device_t ide_devices[4];
unsigned char ide_buf[2048] = {0};
unsigned static char ide_irq_invoked = 0;
unsigned static char atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static inline void outl(uint32_t value, uint16_t port) {
    __asm__ __volatile__ ("outl %0, %w1" : : "a" (value), "Nd" (port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    __asm__ __volatile__ ("inl %w1, %0" : "=a"(value) : "Nd" (port));
    return value;
}

void ide_write(unsigned char channel, unsigned char reg, unsigned char data) {
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    if (reg < 0x08)
        outb(channels[channel].base + reg - 0x00, data);
    else if (reg < 0x0C)
        outb(channels[channel].base + reg - 0x06, data);
    else if (reg < 0x0E)
        outb(channels[channel].ctrl + reg - 0x0A, data);
    else if (reg < 0x16)
        outb(channels[channel].bmide + reg - 0x0E, data);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

unsigned char ide_read(unsigned char channel, unsigned char reg) {
    unsigned char result;
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    if (reg < 0x08)
        result = inb(channels[channel].base + reg - 0x00);
    else if (reg < 0x0C)
        result = inb(channels[channel].base + reg - 0x06);
    else if (reg < 0x0E)
        result = inb(channels[channel].ctrl + reg - 0x0A);
    else if (reg < 0x16)
        result = inb(channels[channel].bmide + reg - 0x0E);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
    return result;
}

static inline void insl(uint16_t port, void *addr, unsigned long count) {
    __asm__ __volatile__ (
    "cld ; rep ; insl "
    : "=D" (addr), "=c" (count)
    : "d"(port), "0"(addr), "1" (count)
    );
}

void ide_read_buffer(unsigned char channel, unsigned char reg, unsigned int buffer,
                     unsigned int quads) {
    /* WARNING: This code contains a serious bug. The inline assembly trashes ES and
     *           ESP for all of the code the compiler generates between the inline
     *           assembly blocks.
     */
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    //__asm__ __volatile__ ("pushw %es; movw %ds, %ax; movw %ax, %es");
    if (reg < 0x08)
        insl(channels[channel].base + reg - 0x00, buffer, quads);
    else if (reg < 0x0C)
        insl(channels[channel].base + reg - 0x06, buffer, quads);
    else if (reg < 0x0E)
        insl(channels[channel].ctrl + reg - 0x0A, buffer, quads);
    else if (reg < 0x16)
        insl(channels[channel].bmide + reg - 0x0E, buffer, quads);
    //__asm__ __volatile__ ("popw %es;");
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

unsigned char ide_polling(unsigned char channel, unsigned int advanced_check) {

    // (I) Delay 400 nanosecond for BSY to be set:
    // -------------------------------------------------
    for (int i = 0; i < 4; i++)
        ide_read(channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.

    // (II) Wait for BSY to be cleared:
    // -------------------------------------------------
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY); // Wait for BSY to be zero.

    if (advanced_check) {
        unsigned char state = ide_read(channel, ATA_REG_STATUS); // Read Status Register.

        // (III) Check For Errors:
        // -------------------------------------------------
        if (state & ATA_SR_ERR)
            return 2; // Error.

        // (IV) Check If Device fault:
        // -------------------------------------------------
        if (state & ATA_SR_DF)
            return 1; // Device Fault.

        // (V) Check DRQ:
        // -------------------------------------------------
        // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
        if ((state & ATA_SR_DRQ) == 0)
            return 3; // DRQ should be set

    }

    return 0; // No Error.

}

unsigned char ide_print_error(unsigned int drive, unsigned char err) {
    if (err == 0)
        return err;

    puts_const("IDE:");
    if (err == 1) {
        puts_const("- Device Fault\n     ");
        err = 19;
    } else if (err == 2) {
        unsigned char st = ide_read(ide_devices[drive].channel, ATA_REG_ERROR);
        if (st & ATA_ER_AMNF) {
            putln_const("- No Address Mark Found\n     ");
            err = 7;
        }
        if (st & ATA_ER_TK0NF) {
            putln_const("- No Media or Media Error\n     ");
            err = 3;
        }
        if (st & ATA_ER_ABRT) {
            putln_const("- Command Aborted\n     ");
            err = 20;
        }
        if (st & ATA_ER_MCR) {
            putln_const("- No Media or Media Error\n     ");
            err = 3;
        }
        if (st & ATA_ER_IDNF) {
            putln_const("- ID mark not Found\n     ");
            err = 21;
        }
        if (st & ATA_ER_MC) {
            putln_const("- No Media or Media Error\n     ");
            err = 3;
        }
        if (st & ATA_ER_UNC) {
            putln_const("- Uncorrectable Data Error\n     ");
            err = 22;
        }
        if (st & ATA_ER_BBK) {
            putln_const("- Bad Sectors\n     ");
            err = 13;
        }
    } else if (err == 3) {
        putln_const("- Reads Nothing\n     ");
        err = 23;
    } else if (err == 4) {
        putln_const("- Write Protected\n     ");
        err = 8;
    }
    /*
    printk("- [%s %s] %s\n",
           (const char *[]) {"Primary",
                             "Secondary"}[ide_devices[drive].channel], // Use the channel as an index into the array
            (const char *[]) {"Master", "Slave"}[ide_devices[drive].drive], // Same as above, using the drive
    ide_devices[drive].model);*/

    return err;
}

void ide_initialize(unsigned int BAR0, unsigned int BAR1, unsigned int BAR2, unsigned int BAR3,
                    unsigned int BAR4) {

    int j, k, count = 0;

    // 1- Detect I/O Ports which interface IDE Controller:
    channels[ATA_PRIMARY].base = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0);
    channels[ATA_PRIMARY].ctrl = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1);
    channels[ATA_SECONDARY].base = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2);
    channels[ATA_SECONDARY].ctrl = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3);
    channels[ATA_PRIMARY].bmide = (BAR4 & 0xFFFFFFFC) + 0; // Bus Master IDE
    channels[ATA_SECONDARY].bmide = (BAR4 & 0xFFFFFFFC) + 8; // Bus Master IDE
    // 2- Disable IRQs:
    ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
    ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);
    // 3- Detect ATA-ATAPI Devices:
    for (int i = 0; i < 2; i++)
        for (j = 0; j < 2; j++) {
            unsigned char err = 0, type = IDE_ATA, status;
            ide_devices[count].reserved = 0; // Assuming that no drive here.
            // (I) Select Drive:
            ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.
            k_delay(1); // Wait 1ms for drive select to work.

            // (II) Send ATA Identify Command:
            ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            k_delay(1); // This function should be implemented in your OS. which waits for 1 ms.
            // it is based on System Timer Device Driver.

            // (III) Polling:
            if (ide_read(i, ATA_REG_STATUS) == 0) continue; // If Status = 0, No Device.

            while (1) {
                status = ide_read(i, ATA_REG_STATUS);
                if ((status & ATA_SR_ERR)) {
                    err = 1;
                    break;
                } // If Err, Device is not ATA.
                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Everything is right.
            }
            // (IV) Probe for ATAPI Devices:

            if (err != 0) {
                unsigned char cl = ide_read(i, ATA_REG_LBA1);
                unsigned char ch = ide_read(i, ATA_REG_LBA2);

                if (cl == 0x14 && ch == 0xEB)
                    type = IDE_ATAPI;
                else if (cl == 0x69 && ch == 0x96)
                    type = IDE_ATAPI;
                else
                    continue; // Unknown Type (may not be a device).

                ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                k_delay(1);
            }
            // (V) Read Identification Space of the Device:
            ide_read_buffer(i, ATA_REG_DATA, (unsigned int) ide_buf, 128);
            // (VI) Read Device Parameters:
            ide_devices[count].reserved = 1;
            ide_devices[count].type = type;
            ide_devices[count].channel = i;
            ide_devices[count].drive = j;
            ide_devices[count].signature = *((unsigned short *) (ide_buf + ATA_IDENT_DEVICETYPE));
            ide_devices[count].capabilities = *((unsigned short *) (ide_buf + ATA_IDENT_CAPABILITIES));
            ide_devices[count].commandSets = *((unsigned int *) (ide_buf + ATA_IDENT_COMMANDSETS));

            // (VII) Get Size:
            if (ide_devices[count].commandSets & (1 << 26))
                // Device uses 48-Bit Addressing:
                ide_devices[count].size = *((unsigned int *) (ide_buf + ATA_IDENT_MAX_LBA_EXT));
            else
                // Device uses CHS or 28-bit Addressing:
                ide_devices[count].size = *((unsigned int *) (ide_buf + ATA_IDENT_MAX_LBA));

            // (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
            for (k = 0; k < 40; k += 2) {
                ide_devices[count].model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
                ide_devices[count].model[k + 1] = ide_buf[ATA_IDENT_MODEL + k];
            }
            ide_devices[count].model[40] = 0; // Terminate String.

            count++;
        }

    // 4- Print Summary:
    for (int i = 0; i < 4; i++)
        if (ide_devices[i].reserved == 1) {
            puts_const("[");
            putint(i);
            puts_const("]Found Drive - ");
            putint(ide_devices[i].type);
            puts_const(" - ");
            putint(ide_devices[i].size);
            puts_const("   ");
            puts(ide_devices[i].model);
            putn();
        }
}

unsigned char ide_ata_access(unsigned char direction, unsigned char drive, unsigned int lba,
                             unsigned char numsects, unsigned short selector, unsigned int edi) {
    unsigned char lba_mode /* 0: CHS, 1:LBA28, 2: LBA48 */, dma /* 0: No DMA, 1: DMA */, cmd;
    unsigned char lba_io[6];
    unsigned int channel = ide_devices[drive].channel; // Read the Channel.
    unsigned int slavebit = ide_devices[drive].drive; // Read the Drive [Master/Slave]
    unsigned int bus = channels[channel].base; // Bus Base, like 0x1F0 which is also data port.
    unsigned int words = 256; // Almost every ATA drive has a sector-size of 512-byte.
    unsigned short cyl, i;
    unsigned char head, sect, err;
    ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = (ide_irq_invoked = 0x0) + 0x02);
    // (I) Select one from LBA28, LBA48 or CHS;
    if (lba >= 0x10000000) { // Sure Drive should support LBA in this case, or you are
        // giving a wrong LBA.
        // LBA48:
        lba_mode = 2;
        lba_io[0] = (lba & 0x000000FF) >> 0;
        lba_io[1] = (lba & 0x0000FF00) >> 8;
        lba_io[2] = (lba & 0x00FF0000) >> 16;
        lba_io[3] = (lba & 0xFF000000) >> 24;
        lba_io[4] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        lba_io[5] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        head = 0; // Lower 4-bits of HDDEVSEL are not used here.
    } else if (ide_devices[drive].capabilities & 0x200) { // Drive supports LBA?
        // LBA28:
        lba_mode = 1;
        lba_io[0] = (lba & 0x00000FF) >> 0;
        lba_io[1] = (lba & 0x000FF00) >> 8;
        lba_io[2] = (lba & 0x0FF0000) >> 16;
        lba_io[3] = 0; // These Registers are not used here.
        lba_io[4] = 0; // These Registers are not used here.
        lba_io[5] = 0; // These Registers are not used here.
        head = (lba & 0xF000000) >> 24;
    } else {
        // CHS:
        lba_mode = 0;
        sect = (lba % 63) + 1;
        cyl = (lba + 1 - sect) / (16 * 63);
        lba_io[0] = sect;
        lba_io[1] = (cyl >> 0) & 0xFF;
        lba_io[2] = (cyl >> 8) & 0xFF;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = (lba + 1 - sect) % (16 * 63) / (63); // Head number is written to HDDEVSEL lower 4-bits.
    }
    // (II) See if drive supports DMA or not;
    dma = 0; // We don't support DMA
    // (III) Wait if the drive is busy;
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY); // Wait if busy.
    if (lba_mode == 0)
        ide_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head); // Drive & CHS.
    else
        ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head); // Drive & LBA
    if (lba_mode == 0 && dma == 0 && direction == 0) cmd = ATA_CMD_READ_PIO;
    if (lba_mode == 1 && dma == 0 && direction == 0) cmd = ATA_CMD_READ_PIO;
    if (lba_mode == 2 && dma == 0 && direction == 0) cmd = ATA_CMD_READ_PIO_EXT;
    if (lba_mode == 0 && dma == 1 && direction == 0) cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 1 && dma == 1 && direction == 0) cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 2 && dma == 1 && direction == 0) cmd = ATA_CMD_READ_DMA_EXT;
    if (lba_mode == 0 && dma == 0 && direction == 1) cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 1 && dma == 0 && direction == 1) cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 2 && dma == 0 && direction == 1) cmd = ATA_CMD_WRITE_PIO_EXT;
    if (lba_mode == 0 && dma == 1 && direction == 1) cmd = ATA_CMD_WRITE_DMA;
    if (lba_mode == 1 && dma == 1 && direction == 1) cmd = ATA_CMD_WRITE_DMA;
    if (lba_mode == 2 && dma == 1 && direction == 1) cmd = ATA_CMD_WRITE_DMA_EXT;
    ide_write(channel, ATA_REG_COMMAND, cmd);               // Send the Command.
    if (dma) {
        PANIC("stub!");
        if (direction == 0);
            // DMA Read.
        else;
        // DMA Write.
    } else if (direction == 0) {
        // PIO Read.
        for (i = 0; i < numsects; i++) {

            if (err = ide_polling(channel, 1))
                return err; // Polling, set error and exit if there is.
            __asm__ __volatile__ ("pushw %es");
            __asm__ __volatile__ ("mov %%ax, %%es" : : "a"(selector));
            __asm__ __volatile__ ("rep insw" : : "c"(words), "d"(bus), "D"(edi)); // Receive Data.
            __asm__ __volatile__ ("popw %es");
            if (err = ide_polling(channel, 1))
                return err; // Polling, set error and exit if there is.
            k_delay(3);
            for (int x = 0; x < 10; x++) {
                puthex(((uint8_t *) edi)[x]);
                //puts_const(" ");
            }
            //edi += (words * 2);
            /*
            puts_const("read ");
            char *data = ide_read(channel, ATA_REG_DATA);
            puts_const("data:");
            putc(data);
            putn();*/
        }

    } else {
        // PIO Write.
        for (i = 0; i < numsects; i++) {
            ide_polling(channel, 0); // Polling.
            __asm__ __volatile__ ("pushw %ds");
            __asm__ __volatile__ ("mov %%ax, %%ds"::"a"(selector));
            __asm__ __volatile__ ("rep outsw"::"c"(words), "d"(bus), "S"(edi)); // Send Data
            __asm__ __volatile__ ("popw %ds");
            edi += (words * 2);
        }
        ide_write(channel, ATA_REG_COMMAND, ((char[]) {ATA_CMD_CACHE_FLUSH,
                                                       ATA_CMD_CACHE_FLUSH,
                                                       ATA_CMD_CACHE_FLUSH_EXT})[lba_mode]);
        ide_polling(channel, 0); // Polling.
    }

    return 0; // Easy, isn't it?
}

void ide_read_sectors(unsigned char drive, unsigned char numsects, unsigned int lba,
                      unsigned short es, unsigned int edi) {

    // 1: Check if the drive presents:
    // ==================================
    if (drive > 3 || ide_devices[drive].reserved == 0) {
        PANIC("Drive Not Found!");
    } else if (((lba + numsects) > ide_devices[drive].size) && (ide_devices[drive].type == IDE_ATA)) {
        PANIC("Seeking to invalid position.");
    } else {
        unsigned char err;
        err = ide_ata_access(ATA_READ, drive, lba, numsects, es, edi);
    }
}