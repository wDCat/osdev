//
// Created by dcat on 10/13/17.
//

#include <proc.h>
#include <str.h>
#include <elfloader.h>
#include <kmalloc.h>
#include <dynlibs.h>
#include "dynlibs.h"

uint32_t liboffset = 0xA0000000;

void dynlibs_install() {

}

int dynlibs_find_symbol(pid_t *pid, const char *name, uint32_t *out) {
    dynlib_inctree_t *tree = getpcb(pid)->dynlibs;
}

int dynlibs_add_to_tree(pid_t pid, dynlib_inctree_t *parent, dynlib_t *lib) {
    pcb_t *pcb = getpcb(getpid());
    dynlib_inctree_t *nitem = (dynlib_inctree_t *) kmalloc(sizeof(dynlib_inctree_t));
    memset(nitem, 0, sizeof(dynlib_inctree_t));
    nitem->lib = lib;
    if (parent == NULL) {
        if (pcb->dynlibs == NULL) {
            pcb->dynlibs = nitem;
        } else {
            dynlib_inctree_t *sp = pcb->dynlibs;
            while (sp->next != NULL)
                sp = sp->next;
            sp->next = nitem;
        }
    } else {
        if (parent->need == NULL) {
            parent->need = nitem;
        } else {
            dynlib_inctree_t *sp = parent->need;
            while (sp->next != NULL)
                sp = sp->next;
            sp->next = nitem;
        }
    }
}

int dynlibs_remove_from_tree(pid_t pid, dynlib_inctree_t *parent, dynlib_t *lib) {

}

int dynlibs_load(pid_t pid, const char *path) {
    int8_t fd = kopen(pid, path, 0);
    if (fd < 0) {
        deprintf("fail to open file:%s", path);
        goto _err;
    }
    elf_digested_t edg;
    elsp_init_edg(&edg, pid, fd);
    edg.global_offset = liboffset;

    CHK(elsp_load_header(&edg), "");
    CHK(elsp_load_sections(&edg), "");
    ASSERT(edg.elf_end_addr > liboffset);
    liboffset = edg.elf_end_addr + (PAGE_SIZE - (edg.elf_end_addr % PAGE_SIZE));
    dprintf("loaded lib %s to %x", path, liboffset);
    dynlib_t *lib = (dynlib_t *) kmalloc(sizeof(dynlib_t));
    memset(lib, 0, sizeof(dynlib_t));
    memcpy(&lib->edg, &edg, sizeof(elf_digested_t));
    lib->path = (char *) kmalloc(strlen(path) + 1);
    strcpy(lib->path, path);
    dynlibs_add_to_tree(pid, NULL, lib);
    return 0;
    _err:
    deprintf("fail to loaded lib %s to %x", path, liboffset);
    return -1;
}