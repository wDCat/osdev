//
// Created by dcat on 8/18/17.
//

#include <str.h>
#include <ide.h>
#include <blk_dev.h>

blk_dev_t disk1;

int disk1_ata_section_read(uint32_t offset, uint32_t len, uint8_t *buff) {
    if (offset % SECTION_SIZE != 0) {
        PANIC("offset and len must be block align!");
    }
    uint32_t blk_no = (offset + 0x100000) / SECTION_SIZE;
    uint32_t blk_count = len / SECTION_SIZE;
    uint8_t err;
    uint8_t tbuff[SECTION_SIZE];
    for (uint32_t x = 0; x < blk_count; x++) {
        err = ide_ata_access(ATA_READ, 1, blk_no + x, 1, buff + x * SECTION_SIZE);
        ASSERT(err == 0);
    }
    if (len % SECTION_SIZE != 0) {
        err = ide_ata_access(ATA_READ, 1, blk_no + blk_count, 1, tbuff);
        ASSERT(err == 0);
        dprintf("copy %x to %x size %x", buff + (blk_count) * SECTION_SIZE, buff, len % SECTION_SIZE);
        memcpy(buff + (blk_count) * SECTION_SIZE, tbuff, len % SECTION_SIZE);
        //for (;;);
    }
    return err;
}

int disk1_ata_section_write(uint32_t offset, uint32_t len, uint8_t *buff) {
    if (offset % SECTION_SIZE != 0 || len % SECTION_SIZE != 0) {
        PANIC("offset and len must be block align!");
    }
    uint32_t blk_no = (offset + 0x100000) / SECTION_SIZE;
    uint32_t blk_count = len / SECTION_SIZE;
    uint8_t err;
    for (uint32_t x = 0; x < blk_count; x++) {
        err = ide_ata_access(ATA_WRITE, 1, blk_no + x, 1, buff + x * SECTION_SIZE);
        ASSERT(err == 0);
    }
    return err;
}

void blk_dev_install() {
    strcpy(disk1.name, "DISK1");
    disk1.read = disk1_ata_section_read;
    disk1.write = disk1_ata_section_write;
}