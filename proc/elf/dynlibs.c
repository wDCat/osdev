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
#include <swap.h>
#include <page.h>
#include "dynlibs.h"

uint32_t liboffset = 0xA0000000;
mutex_lock_t dynlibs_lock;
dynlib_t *loaded_dynlibs[MAX_LOADED_LIBS];
uint32_t loaded_dynlibs_count = 0;

void dynlibs_install() {
    memset(loaded_dynlibs, 0, sizeof(uint32_t) * MAX_LOADED_LIBS);
    mutex_init(&dynlibs_lock);
}

int dynlibs_find_symbol(pid_t pid, const char *name, uint32_t *out) {
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


bool dynlibs_isloaded_to_memory(const char *path, uint32_t *index_out) {
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

bool dynlibs_isloaded_to_proc(const char *path, pid_t pid) {
    dynlib_inctree_t *roottree = getpcb(pid)->dynlibs;
    squeue_t pre_iter;
    memset(&pre_iter, 0, sizeof(squeue_t));
    squeue_insert(&pre_iter, (uint32_t) roottree);
    while (!squeue_isempty(&pre_iter)) {
        dynlib_inctree_t *tree = SQUEUE_GET(&pre_iter, 0, dynlib_inctree_t*);
        if (tree->next)squeue_insert(&pre_iter, (uint32_t) tree->next);
        if (tree->need)squeue_insert(&pre_iter, (uint32_t) tree->need);
        if (strcmp(tree->lib->path, path))return true;
    }
    return false;
}

int dynlibs_load_section_data(dynlib_t *lib, elf_digested_t *edg,
                              elf_section_t *shdr, squeue_t *finfo) {
    ASSERT(lib && edg && shdr);
    pcb_t *pcb = getpcb(edg->pid);
    uint32_t last_paddr = SQUEUE_GET(finfo, 0, uint32_t);
    if (shdr->sh_addr) {
        if (shdr->sh_addr < edg->global_offset)
            shdr->sh_addr += edg->global_offset;
        if (shdr->sh_addr + shdr->sh_size > edg->elf_end_addr) {
            edg->elf_end_addr = shdr->sh_addr + shdr->sh_size;
        }
        dprintf("load to %x global_offset:%x", shdr->sh_addr, edg->global_offset);
        uint32_t les = shdr->sh_size;
        uint32_t y = shdr->sh_addr;
        klseek(edg->pid, edg->fd, shdr->sh_offset, SEEK_SET);
        while (les > 0) {
            dynlib_frame_t *frameinfo = (dynlib_frame_t *) kmalloc(sizeof(dynlib_frame_t));
            page_t *page = get_page(y, true, pcb->page_dir);
            uint32_t paddr = y - (y % PAGE_SIZE);
            page_typeinfo_t *info = get_page_type(y, pcb->page_dir);
            info->pid = 0;//shared lib
            info->free_on_proc_exit = false;
            uint32_t size = MIN(MIN(PAGE_SIZE, les), PAGE_SIZE - (y % PAGE_SIZE));
            uint32_t foffset = shdr->sh_offset + y - shdr->sh_addr;
            alloc_frame(page, false, true);
            if (shdr->sh_type == SHT_NOBITS) {
                memset(y, 0, size);
            } else {
                klseek(edg->pid, edg->fd, foffset, SEEK_SET);
                if (kread(edg->pid, edg->fd, size, y) != size) {
                    deprintf("cannot read section:%x.I/O error.", y);
                    goto _err;
                }
            }
            if (paddr != last_paddr) {
                last_paddr = paddr;
                CHK(squeue_set(finfo, 0, paddr), "");
                frameinfo->start_addr = paddr;
                frameinfo->dirty = false;
                frameinfo->frameno = get_frame(page);
                dprintf("frame info:%x %x", paddr, frameinfo->frameno);
                CHK(squeue_insert(finfo, frameinfo), "");
            } else
                dprintf("reuse finfo:%x", last_paddr);
            y += size;
            les -= size;
        }
    }
    return 0;
    _err:
    return -1;
}

int dynlibs_load_sections(dynlib_t *lib, elf_digested_t *edg) {
    elf_header_t *ehdr = &edg->ehdr;
    uint32_t shdrsize = ehdr->e_shentsize * ehdr->e_shnum;
    edg->shdrs = (elf_section_t *) kmalloc_paging(shdrsize, NULL);
    elf_section_t *shdr = edg->shdrs;
    klseek(edg->pid, edg->fd, ehdr->e_shoff, SEEK_SET);
    if (kread(edg->pid, edg->fd, shdrsize, shdr) != shdrsize) {
        deprintf("cannot read section info.");
        goto _err;
    }
    squeue_t framesinfo;
    squeue_init(&framesinfo);
    squeue_insert(&framesinfo, 0x0);
    shdr = edg->shdrs;
    for (int x = 0; x < edg->ehdr.e_shnum; x++) {
        dprintf("Section %x(%x) addr:%x size:%x offset:%x type:%x", x, shdr->sh_name, shdr->sh_addr, shdr->sh_size,
                shdr->sh_offset,
                shdr->sh_type);
        CHK(dynlibs_load_section_data(lib, edg, shdr, &framesinfo), "");
        if (shdr->sh_type == SHT_DYNAMIC) {
            if (elsp_dynamic_section(edg, shdr)) {
                deprintf("exception happened when processing dynamic section");
                goto _err;
            }
        }
        shdr = (elf_section_t *) ((uint32_t) shdr + edg->ehdr.e_shentsize);
    }
    shdr = edg->shdrs;
    for (int x = 0; x < edg->ehdr.e_shnum; x++) {
        switch (shdr->sh_type) {
            case SHT_SYMTAB:
                if (shdr->sh_addr == 0) {
                    //.symtab
                    dprintf("set section %x as symtab.", x);
                    edg->symbols = (elf_symbol_t *) kmalloc(shdr->sh_size);
                    klseek(edg->pid, edg->fd, shdr->sh_offset, SEEK_SET);
                    if (kread(edg->pid, edg->fd, shdr->sh_size, (uchar_t *) edg->symbols) != shdr->sh_size) {
                        deprintf("i/o error when read sym section");
                        goto _err;
                    }
                    edg->symbols_size = shdr->sh_size;
                }
                break;
            case SHT_DYNSYM:
                if (edg->dyn_entry.dynsymtab != 0 && edg->dyn_entry.dynsymtab != shdr->sh_offset) {
                    deprintf("Bad dynstmtab address!");
                    goto _err;
                }
                if (shdr->sh_addr == 0) {
                    deprintf("Bad dynstm section.");
                    goto _err;
                }
                edg->dyn_entry.dynsymtab = shdr->sh_addr;
                edg->dyn_entry.dynsymsz = shdr->sh_size;
                break;
            case SHT_STRTAB:
                if (shdr->sh_offset == edg->dyn_entry.dynstrtab) {
                    //.dynstr section
                    edg->dyn_strings = (char *) shdr->sh_addr;
                } else if (edg->strings == NULL) {
                    dprintf("set section %x as strtab.", x);
                    edg->strings = (char *) kmalloc(shdr->sh_size);
                    klseek(edg->pid, edg->fd, shdr->sh_offset, SEEK_SET);
                    if (kread(edg->pid, edg->fd, shdr->sh_size, (uchar_t *) edg->strings) != shdr->sh_size) {
                        deprintf("i/o error when read str section");
                        goto _err;
                    }
                    edg->strings_size = shdr->sh_size;
                } else
                    dprintf("ignore strtab:%x", x);
                break;
            case SHT_REL:
                edg->s_rel = shdr;
                break;
            case SHT_RELA:
                edg->s_rela = shdr;
                break;

        }
        shdr = (elf_section_t *) ((uint32_t) shdr + edg->ehdr.e_shentsize);
    }
    if (edg->s_rel)
        CHK(elsp_relocate(edg, edg->s_rel), "");
    if (edg->s_rela)
        CHK(elsp_relocate(edg, edg->s_rela), "");
    //save frame info to dynlib_t
    uint32_t framec = (uint32_t) (squeue_count(&framesinfo) - 1);
    lib->frames = (dynlib_frame_t *) kmalloc(sizeof(dynlib_frame_t) * framec);
    lib->frames_count = framec;
    dprintf("frame c:%x", framec);
    for (int x = 0; x < framec; x++) {
        uint32_t objaddr;
        CHK(squeue_remove_vout(&framesinfo, 1, &objaddr), "");
        ASSERT(objaddr);
        memcpy(&lib->frames[x], (void *) objaddr, sizeof(dynlib_frame_t));
    }
    CHK(squeue_remove(&framesinfo, 0), "");
    return 0;
    _err:
    return 1;
}

dynlib_t *dynlibs_do_load_to_memory(pid_t pid, const char *path) {
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
    dynlib_t *lib = (dynlib_t *) kmalloc(sizeof(dynlib_t));
    memset(lib, 0, sizeof(dynlib_t));
    elsp_init_edg(&edg, pid, fd);
    edg.global_offset = liboffset;
    CHK(elsp_load_header(&edg), "");
    CHK(dynlibs_load_sections(lib, &edg), "");
    ASSERT(edg.elf_end_addr > liboffset);
    liboffset = edg.elf_end_addr + (PAGE_SIZE - (edg.elf_end_addr % PAGE_SIZE));
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

int dynlibs_do_load_to_proc(pid_t pid, dynlib_t *lib) {
    pcb_t *pcb = getpcb(pid);
    dprintf("frame count:%x", lib->frames_count);
    for (uint32_t x = 0; x < lib->frames_count; x++) {
        dynlib_frame_t *frame = &lib->frames[x];
        page_t *page = get_page(frame->start_addr, true, pcb->page_dir);
        page->present = true;
        page->accessed = false;
        page->user = true;
        page->rw = true;
        page->dirty = false;
        page->frame = frame->frameno;
        dprintf("[pid:%x]remapping %x -> %x", pid, frame->frameno, frame->start_addr);
    }
    CHK(dynlibs_add_to_tree(pid, NULL, lib), "");
    return 0;
    _err:
    return -1;
}

int dynlibs_try_to_load(pid_t pid, const char *path) {
    dynlib_t *lib;
    uint32_t index;
    if (dynlibs_isloaded_to_memory(path, &index)) {
        lib = loaded_dynlibs[index];
        ASSERT(lib);
    } else {
        lib = dynlibs_do_load_to_memory(pid, path);
        if (lib == NULL)return -1;
    }
    CHK(dynlibs_do_load_to_proc(pid, lib), "");
    dprintf("proc %x loaded dynlib %s", pid, path);
    return 0;
    _err:
    return -1;
}