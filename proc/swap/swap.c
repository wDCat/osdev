//
// Created by dcat on 9/18/17.
//

#include <str.h>
#include "include/swap.h"
#include <vfs.h>
#include <signal.h>
#include <page.h>
#include <blk_dev.h>
#include <swap_disk.h>


swap_disk_t sdisk;

void swap_install() {
    memset(&sdisk, 0, sizeof(sdisk));
    swap_disk_init(&sdisk, &swap_disk, 20);
}

spage_info_t *swap_find_empty(pcb_t *pcb) {
    //dprintf("pcb:%x", pcb);
    for (int x = 0; x < MAX_SWAPPED_OUT_PAGE; x++) {
        dprintf("addr:%x index %x type %x", &pcb->spages[x], x, pcb->spages[x].type);
        if (pcb->spages[x].type == SPAGE_TYPE_UNUSED) {
            return &pcb->spages[x];
        }
    }
    return NULL;
}

int swap_insert_empty_page(pcb_t *pcb, uint32_t addr) {
    spage_info_t *info = swap_find_empty(pcb);
    if (!info) {
        deprintf("Too much spage");
        PANIC("debug")
        return 1;
    }
    info->addr = addr;
    info->type = SPAGE_TYPE_EMPTY;
    return 0;

}

int swap_insert_pload_page(pcb_t *pcb, uint32_t addr, int8_t fd,
                           uint32_t offset) {
    dprintf("pload:addr:%x off:%x inoff:0 size:0x1000", addr, offset);
    spage_info_t *info = swap_find_empty(pcb);
    if (!info) {
        deprintf("Too much spage");
        PANIC("debug")
        return 1;
    }
    ASSERT(pcb->fh[fd].present);
    info->type = SPAGE_TYPE_PLOAD;
    info->addr = addr;
    info->fd = fd;
    info->offset = offset;
    pcb->spages_count++;
    return 0;
}

int swap_out(pcb_t *pcb, uint32_t addr) {
    dprintf("swap out page %x pid:%x", addr, pcb->pid);
    spage_info_t *info = swap_find_empty(pcb);
    if (!info) {
        deprintf("Too much spage");
        PANIC("debug")
        return 1;
    }
    page_t *page = get_page(addr, false, pcb->page_dir);
    if (!page) {
        deprintf("try to swap out a non-exist page:%x pid:%x", addr, pcb->pid);
        return 1;
    }
    if (is_kernel_space(pcb->page_dir, addr)) {
        deprintf("try to swap out a kernel page:%x pid:%x", addr, pcb->pid);
        return 1;
    }
    uint32_t id = swap_disk_put(&sdisk, pcb, addr);
    if (id == 0) {
        deprintf("cannot write page");
        return 1;
    }
    info->type = SPAGE_TYPE_SOUT;
    info->addr = addr;
    info->offset = id;
    free_frame(page);
    page->present = 0;
    return 0;
}

int swap_in(pcb_t *pcb, spage_info_t *info) {
    if (getpid() != pcb->pid) {
        TODO;//swap page table.
    }
    dprintf("swap in page %x", info->addr);
    switch (info->type) {
        case SPAGE_TYPE_PLOAD: {
            uint32_t pageno = info->addr - info->addr % 0x1000;
            dprintf("offset:%x write to:%x", info->offset, info->addr);
            page_t *page = get_page(pageno, true, pcb->page_dir);
            ASSERT(page);
            alloc_frame(page, false, true);
            klseek(pcb->pid, info->fd, info->offset, SEEK_SET);
            if (kread(pcb->pid, info->fd, PAGE_SIZE, info->addr) != PAGE_SIZE) {
                deprintf("fail to swap in page.I/O error.");
                return 1;
            }
            return 0;
        }
            break;
        case SPAGE_TYPE_SOUT: {
            TODO;
        }
            break;
        case SPAGE_TYPE_EMPTY: {
            uint32_t pageno = info->addr - info->addr % 0x1000;
            page_t *page = get_page(pageno, true, pcb->page_dir);
            ASSERT(page);
            alloc_frame(page, false, true);
            memset(info->addr, 0, 0x1000);
            return 0;
        }
            break;
        default:
            deprintf("Unknown Type:%x", info->type);
            PANIC("debug")
    }
}

int swap_handle_page_fault(regs_t *r) {
    uint32_t cr2;
    uint32_t pageno;
    bool found = false;
    pcb_t *pcb = getpcb(getpid());
    __asm__ __volatile__("mov %%cr2, %0" : "=r" (cr2));
    pageno = cr2 - cr2 % 0x1000;
    dprintf("process..pid:%x addr:%x", getpid(), cr2);
    if (cr2 / 0x10000 == 0xCCFF) {
        uint32_t les = cr2 % 0x1000;
        //if (!les)cr2 -= 0x1000;
        dprintf("alloc um kstack:%x", cr2);
        alloc_frame(get_page(cr2, true, pcb->page_dir), true, true);
        found = true;
    } else {
        for (int x = 0, y = 0; y < pcb->spages_count && x < MAX_SWAPPED_OUT_PAGE; x++) {
            if (pcb->spages[x].type != SPAGE_TYPE_UNUSED) {
                y++;
                if (pageno == pcb->spages[x].addr) {
                    if (swap_in(pcb, &pcb->spages[x]))return 1;
                    found = true;
                    dprintf("handler done.");
                }\
            }
        }
    }
    if (found)
        dprintf("dump eip:%x %x", pcb->tss.eip, r->eip);
    else
        dwprintf("no suitable page found!");
    return !found;
}