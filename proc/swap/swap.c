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
                           uint32_t offset, uint32_t in_offset,
                           uint32_t size) {
    dprintf("pload:addr:%x off:%x inoff:%x size:%x", addr, offset, in_offset, size);
    /*
    page_t *page = get_page(addr, true, pcb->page_dir);
    free_frame(page);
    page->present = 0;*/
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
    info->in_offset = in_offset;
    info->size = size;
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
    info->size = 0x1000;
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
            dprintf("offset:%x inoff:%x size:%x write to:%x", info->offset, info->in_offset, info->size,
                    info->addr + info->in_offset);
            page_t *page = get_page(pageno, true, pcb->page_dir);
            ASSERT(page);
            alloc_frame(page, false, true);
            uint8_t *ptr = (uint8_t *) (info->addr + info->in_offset);
            int32_t rsize = kread(pcb->pid, info->fd, info->offset, info->size, ptr);

            if ((uint32_t) rsize != info->size) {
                deprintf("fail to swap in page.I/O error.expect:%x ret:%x", info->size, rsize);
                send_sig(pcb, SIGABRT);
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
            alloc_frame(page, false, false);
            //memset(info->addr, 0, 0x1000);
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
    for (int x = 0, y = 0; y < pcb->spages_count && x < MAX_SWAPPED_OUT_PAGE; x++) {
        if (pcb->spages[x].type != SPAGE_TYPE_UNUSED) {
            y++;
            if (pageno == pcb->spages[x].addr) {
                if (swap_in(pcb, &pcb->spages[x])) {
                    PANIC("error.")
                }
                found = true;
                dprintf("handler done.");
            }
        }
    }
    if (found)
        dprintf("dump eip:%x %x", pcb->tss.eip, r->eip);
    else
        deprintf("no suitable page found!");
    return !found;
}