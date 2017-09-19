//
// Created by dcat on 9/19/17.
//

#include <swap_disk.h>
#include <kmalloc.h>
#include <proc.h>
#include <str.h>

int swap_disk_init(swap_disk_t *sdisk, blk_dev_t *dev, uint32_t max_page_count) {
    sdisk->dev = dev;
    sdisk->max_page_count = max_page_count;
    sdisk->bitset = (uint8_t *) kmalloc(max_page_count / 8 + 1);
    memset(sdisk->bitset, 0, max_page_count / 8 + 1);
    sdisk->p = 1;
    return 0;
}

uint32_t swap_disk_put(swap_disk_t *sdisk, pcb_t *pcb, uint32_t addr) {

    if (getpid() != pcb->pid) {
        //TODO;//swap page dir
    }
    //find free space
    uint32_t page_no = 0;
    bool loop = false;
    for (uint32_t x = sdisk->p; x <= sdisk->max_page_count; x++) {
        if (x == sdisk->max_page_count) {
            if (loop) {
                deprintf("Out of swap disk");
                PANIC("debug...")
                return NULL;
            }
            loop = true;
            x = 1;
        }
        if (!BIT_GET(sdisk->bitset[x / 8], x % 8)) {
            BIT_SET(sdisk->bitset[x / 8], x % 8);
            page_no = x;
            break;
        }
    }

    uint32_t diskoff = 0x1000 * page_no;
    dprintf("put %x's %x to swap disk offset:%x", pcb->pid, addr, diskoff);
    if (sdisk->dev->write(diskoff, 0x1000, (uint8_t *) addr)) {
        deprintf("I/O Error.");
        return NULL;
    }
    return page_no;
}

int swap_disk_get(swap_disk_t *sdisk, pcb_t *pcb, uint32_t addr, uint32_t id) {
    if (id > sdisk->max_page_count) {
        deprintf("Bad sdisk page id:%x", id);
        return 1;
    }
    if (getpid() != pcb->pid) {
        //TODO;//swap page dir
    }
    uint32_t diskoff = 0x1000 * id;
    if (sdisk->dev->read(diskoff, 0x1000, addr)) {
        deprintf("I/O Error.");
        return 1;
    }
    return 0;
}

int swap_disk_clear(swap_disk_t *sdisk, uint32_t id) {
    if (id > sdisk->max_page_count) {
        deprintf("Bad sdisk page id:%x", id);
        return 1;
    }
    BIT_CLEAR(sdisk->bitset[id / 8], id % 8);
    return 0;
}