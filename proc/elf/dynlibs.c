//
// Created by dcat on 10/13/17.
//

#include <proc.h>
#include <str.h>
#include <elfloader.h>
#include <kmalloc.h>
#include <dynlibs.h>
#include <squeue.h>
#include <syscall.h>
#include <mutex.h>
#include "dynlibs.h"

uint32_t liboffset = 0xA0000000;
mutex_lock_t dynlibs_lock;
dynlib_t *loaded_dynlibs[MAX_LOADED_LIBS];
uint32_t loaded_dynlibs_count = 0;

void dynlibs_install() {
    memset(loaded_dynlibs, 0, sizeof(uint32_t) * MAX_LOADED_LIBS);
    mutex_init(&dynlibs_lock);
}

int dynlibs_find_symbol(pid_t *pid, const char *name, uint32_t *out) {
    dynlib_inctree_t *roottree = getpcb(pid)->dynlibs;
    squeue_t pre_iter;
    memset(&pre_iter, 0, sizeof(squeue_t));
    squeue_insert(&pre_iter, (uint32_t) roottree);
    while (!squeue_isempty(&pre_iter)) {
        dynlib_inctree_t *tree = SQUEUE_GET(&pre_iter, 0, dynlib_inctree_t*);
        if (tree->next)squeue_insert(&pre_iter, (uint32_t) tree->next);
        if (tree->need)squeue_insert(&pre_iter, (uint32_t) tree->need);
        dynlib_t *lib = tree->lib;
        elf_digested_t *edg = &lib->edg;
        ASSERT(lib);
        dprintf("searching symbol in %s", lib->path);
        for (uint32_t x = 0;; x++) {
            elf_symbol_t *esym = elsp_get_dynsym(edg, x);
            if (esym == NULL)break;
            const char *symname = elsp_get_dynstring_by_offset(edg, esym->st_name);
            if (symname == NULL)continue;
            if (strcmp(symname, name)) {
                if (out) {
                    *out = edg->global_offset + esym->st_value + 0x0;
                    return 0;
                }
            }
        }
    }
    return 1;
}

int dynlibs_add_to_tree(pid_t pid, dynlib_inctree_t *parent, dynlib_t *lib) {
    pcb_t *pcb = getpcb(pid);
    dynlib_inctree_t *nitem = (dynlib_inctree_t *) kmalloc(sizeof(dynlib_inctree_t));
    memset(nitem, 0, sizeof(dynlib_inctree_t));
    nitem->lib = lib;
    if (parent == NULL) {
        if (pcb->dynlibs == NULL) {
            dprintf("insert into root");
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
    return 0;
}

int dynlibs_remove_from_tree(pid_t pid, dynlib_inctree_t *parent, dynlib_t *lib) {
    dynlibs_add_to_tree(pid, NULL, lib);
}

int dynlibs_freeobj(dynlib_t *lib) {
    if (lib) {
        if (lib->path)kfree(lib->path);
        if (lib->frames)kfree(lib->frames);
        //TODO free lib->edg and lib->used_proc
    }
}


bool dynlibs_isloaded(const char *path, uint32_t *index_out) {
    for (uint32_t x = 0, y = 0; x < MAX_LOADED_LIBS, y < loaded_dynlibs_count; x++) {
        if (loaded_dynlibs[x] != NULL) {
            y++;
            if (strcmp(loaded_dynlibs[x]->path, path)) {
                if (index_out)*index_out = x;
                return true;
            }
        }
    }
    return false;
}

int dynlibs_load_sections(elf_digested_t *edg, dynlib_frame_t **frameinfo_out) {

}

dynlib_t *dynlibs_do_load(pid_t pid, const char *path) {
    if (loaded_dynlibs_count >= MAX_LOADED_LIBS) {
        deprintf("Out of dynlibs table...");
        return NULL;
    }
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
    dynlib_t *lib = (dynlib_t *) kmalloc(sizeof(dynlib_t));
    memset(lib, 0, sizeof(dynlib_t));
    memcpy(&lib->edg, &edg, sizeof(elf_digested_t));
    lib->path = (char *) kmalloc(strlen(path) + 1);
    strcpy(lib->path, path);
    mutex_lock(&dynlibs_lock);
    loaded_dynlibs_count++;
    for (int x = 0; x < MAX_LOADED_LIBS; x++)
        if (loaded_dynlibs[x] == NULL) {
            loaded_dynlibs[x] = lib;
            break;
        }
    mutex_unlock(&dynlibs_lock);
    dprintf("loaded lib %s to %x", lib->path, liboffset);
    return lib;
    _err:
    deprintf("fail to loaded lib %s to %x", path, liboffset);
    return NULL;
}

int dynlibs_try_to_load(pid_t pid, const char *path) {
    dynlib_t *lib;
    uint32_t index;
    if (dynlibs_isloaded(path, &index)) {
        lib = loaded_dynlibs[index];
        ASSERT(lib);
    } else {
        lib = dynlibs_do_load(pid, path);
        if (lib == NULL)return 1;
    }
    CHK(dynlibs_add_to_tree(pid, NULL, lib), "");
    dprintf("proc %x loaded dynlib %s", pid, path);
    return 0;
    _err:
    return -1;
}