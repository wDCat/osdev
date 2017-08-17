//
// Created by dcat on 3/21/17.
//

#include <ide.h>
#include <str.h>
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
    //k_delay(1);
    // (I) Delay 400 nanosecond for BSY to be set:
    // -------------------------------------------------
    for (int i = 0; i < 6; i++)
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
            puts_const(" - ");
            puthex(ide_devices[i].channel);

            puts_const("   ");
            puts(ide_devices[i].model);
            putn();
        }
}

//rewrite a clear version.maybe...
uint8_t ide_ata_access(uint8_t action, uint8_t drive, uint32_t lba, uint32_t block_count, uint8_t *buff) {
    uint8_t channel = ide_devices[drive].channel;
    uint16_t slavebit = ide_devices[drive].drive;
    uint16_t bus = channels[channel].base;
    uint16_t words = 256;
    ASSERT(ide_devices[1].reserved);
    //disable IRQs
    ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = (ide_irq_invoked = 0x0) + 0x02);
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY);//wait for device
    ASSERT(ide_devices[drive].capabilities & 0x200);
    uint8_t lba_reg[6];
    lba_reg[0] = (lba & 0x00000FF) >> 0;
    lba_reg[1] = (lba & 0x000FF00) >> 8;
    lba_reg[2] = (lba & 0x0FF0000) >> 16;
    lba_reg[3] = 0;
    lba_reg[4] = 0;
    lba_reg[5] = 0;
    uint8_t head = (lba & 0xF000000) >> 24;
    ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head);   // Select Drive CHS.
    ide_write(channel, ATA_REG_SECCOUNT0, block_count);
    ide_write(channel, ATA_REG_LBA0, lba_reg[0]);
    ide_write(channel, ATA_REG_LBA1, lba_reg[1]);
    ide_write(channel, ATA_REG_LBA2, lba_reg[2]);
    //Not necessary//
    //ide_write(channel, ATA_REG_LBA3, lba_reg[3]);
    //ide_write(channel, ATA_REG_LBA4, lba_reg[4]);
    //ide_write(channel, ATA_REG_LBA5, lba_reg[5]);
    ide_write(channel, ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    if (action == ATA_READ) {
        uint8_t *wbuff = buff;
        for (uint8_t x = 0; x < block_count; x++) {
            uint8_t err = ide_polling(channel, 1);
            if (err != 0) {
                putf("[-] ATA ErrNo:%x\n", err);
                ide_print_error(1, err);
                return err;
            }
            __asm__ __volatile__ ("rep insw" : : "c"(words), "d"(channels[channel].base), "D"(wbuff)); // Receive Data.
            wbuff += 512;
        }
    } else if (action == ATA_WRITE) {
        PANIC("stub!");
    }
    return 0;
}