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

mutex_lock_t dynlibs_lock;
dynlib_t *loaded_dynlibs[MAX_LOADED_LIBS];
uint32_t loaded_dynlibs_count = 0;

void dynlibs_install() {
    memset(loaded_dynlibs, 0, sizeof(uint32_t) * MAX_LOADED_LIBS);
    mutex_init(&dynlibs_lock);
}

int dynlibs_find_symbol(pid_t pid, const char *name, uint32_t *out) {
    dynlib_inctree_t *roottree = getpcb(pid)->dynlibs;
    if (roottree == NULL) {
        dprintf("tree not found!");
        return 1;
    }
    squeue_t pre_iter;
    memset(&pre_iter, 0, sizeof(squeue_t));
    squeue_insert(&pre_iter, (uint32_t) roottree);
    while (!squeue_isempty(&pre_iter)) {
        dynlib_inctree_t *tree = SQUEUE_GET(&pre_iter, 0, dynlib_inctree_t*);
        if (tree->next)squeue_insert(&pre_iter, (uint32_t) tree->next);
        if (tree->need)squeue_insert(&pre_iter, (uint32_t) tree->need);
        dynlib_t *lib = loaded_dynlibs[tree->loadinfo->no];
        elf_digested_t *edg = &lib->edg;
        ASSERT(lib);
        dprintf("searching symbol in %s", lib->path);
        if (elsp_find_symbol(edg, name, out) == 0) {
            dprintf("found %x+%x", tree->loadinfo->start_addr, *out);
            *out = (uint32_t) *out + tree->loadinfo->start_addr;
            return 0;
        }
        squeue_remove(&pre_iter, 0);
    }
    return 1;
}

int dynlibs_try_to_write(pid_t pid, uint32_t addr) {
    int ints;
    scli(&ints);
    int ret = -1;
    pcb_t *pcb = getpcb(pid);
    page_typeinfo_t *info = get_page_type(addr, pcb->page_dir);
    page_t *page = get_page(addr, false, pcb->page_dir);
    uint32_t orig_frame = page->frame;
    if (info == NULL || page == NULL) {
        deprintf("Bad addr:%x", addr);
        return -1;
    }
    switch (info->type) {
        case PT_DYNLIB_ORIG: {
            page->frame = NULL;
            alloc_frame(page, !page->user, page->rw);
            page_typeinfo_t *ninfo = get_page_type(addr, pcb->page_dir);
            ninfo->pid = pid;
            ninfo->free_on_proc_exit = true;
            ninfo->type = PT_DYNLIB_DIRTY;
            ninfo->copy_on_fork = true;
            dprintf("copy on write(%x) %x --> %x", addr, orig_frame, page->frame);
            copy_frame_physical(orig_frame, page->frame);
            break;
        }
        case PT_DYNLIB_DIRTY:
            ret = 0;
            break;
        default:
            deprintf("Unknown page type:%x at %x", info->type, addr);
            ret = 0;
            break;
    }
    _ret:
    srestorei(&ints);
    return ret;
}

int dynlibs_add_to_tree(pid_t pid, dynlib_inctree_t *parent, dynlib_load_t *loadinfo) {
    pcb_t *pcb = getpcb(pid);
    dynlib_inctree_t *nitem = (dynlib_inctree_t *) kmalloc(sizeof(dynlib_inctree_t));
    memset(nitem, 0, sizeof(dynlib_inctree_t));
    nitem->loadinfo = loadinfo;
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

int dynlibs_remove_from_tree(pid_t pid, dynlib_inctree_t *parent, dynlib_load_t *loadinfo) {
    pcb_t *pcb = getpcb(pid);
    if (pcb->dynlibs == NULL) {
        deprintf("try to remove a lib from tree but this tree is null.");
        return -1;
    } else {
        dynlib_inctree_t *sp = (parent == NULL) ? pcb->dynlibs : parent;
        if (sp->loadinfo == loadinfo) {
            if (parent != NULL)
                parent->need = sp->next;
            else
                pcb->dynlibs = sp->next;
            kfree(sp->loadinfo);
            kfree(sp);
            return 0;
        }
        while (sp->next != NULL && sp->next->loadinfo != loadinfo) {
            sp = sp->next;
        }
        if (sp == NULL) {
            deprintf("not found:%x", loadinfo);
            return -1;
        }
        dynlib_inctree_t *t = sp->next;
        sp->next = sp->next->next;
        sp->next->parent = sp;
        kfree(t->loadinfo);
        kfree(t);
        return 0;
    }
}

//NOCHK
int dynlibs_clone_tree_inner(pid_t pid, dynlib_inctree_t *p, dynlib_inctree_t **out, int *c) {
    if (p == NULL)return -1;
    dynlib_inctree_t *tree = (dynlib_inctree_t *) kmalloc(sizeof(dynlib_inctree_t));
    dynlib_load_t *nload = (dynlib_load_t *) kmalloc(sizeof(dynlib_load_t));
    nload->no = p->loadinfo->no;
    nload->pid = pid;
    nload->start_addr = p->loadinfo->start_addr;
    tree->loadinfo = nload;
    if (p->next) {
        CHK(dynlibs_clone_tree_inner(pid, p->next, &tree->next, c), "");
    }
    if (p->need) {
        CHK(dynlibs_clone_tree_inner(pid, p->need, &tree->need, c), "");
    }
    *out = tree;
    (*c)++;
    return 0;
    _err:
    return -1;
}

//NOCHK
int dynlibs_clone_tree(pid_t src, pid_t target) {
    pcb_t *spcb = getpcb(src);
    int count = 0;
    dynlib_inctree_t *sr = spcb->dynlibs;
    if (dynlibs_clone_tree_inner(target, sr, &(getpcb(target)->dynlibs), &count)) {
        deprintf("copy dynlibs tree failed!");
        return -1;
    }
    dprintf("clone done.Cloned %x items.", count);
    return 0;
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
        if (strcmp(loaded_dynlibs[tree->loadinfo->no]->path, path))return true;
        squeue_remove(&pre_iter, 0);
    }
    return false;
}

int dynlibs_unload_inner(pid_t pid, dynlib_load_t *loadinfo, bool remove_from_tree) {
    dprintf("unloading %s at %x", loaded_dynlibs[loadinfo->no]->path, loadinfo->start_addr);
    pcb_t *pcb = getpcb(pid);
    if (loadinfo->pid != pid) {
        deprintf("bad dynlib_load_t l->pid:%x pid:%x", loadinfo->pid, pid);
    }
    dynlib_t *lib = loaded_dynlibs[loadinfo->no];
    for (int x = 0; x < lib->frames_count; x++) {
        uint32_t paddr = loadinfo->start_addr + x;
        page_t *page = get_page(paddr, false, pcb->page_dir);
        page_typeinfo_t *pte = get_page_type(paddr, pcb->page_dir);
        switch (pte->type) {
            case PT_DYNLIB_ORIG:
                break;
            case PT_DYNLIB_DIRTY:
                free_frame(page);
                break;
            default:
                deprintf("Unknown page type:%x at %x", pte->type, paddr);
                return -1;
        }
        page->frame = NULL;
    }
    if (remove_from_tree)
        dynlibs_remove_from_tree(pid, NULL, loadinfo);
}

inline int dynlibs_unload(pid_t pid, dynlib_load_t *loadinfo) {
    dynlibs_unload_inner(pid, loadinfo, true);
}

//TODO
int dynlibs_unload_all(pid_t pid) {
    dprintf("unloading...");
    pcb_t *pcb = getpcb(pid);
    dynlib_inctree_t *roottree = getpcb(pid)->dynlibs;
    squeue_t pre_iter;
    memset(&pre_iter, 0, sizeof(squeue_t));
    squeue_insert(&pre_iter, (uint32_t) roottree);
    while (!squeue_isempty(&pre_iter)) {
        dynlib_inctree_t *tree = SQUEUE_GET(&pre_iter, 0, dynlib_inctree_t*);
        if (tree->next)squeue_insert(&pre_iter, (uint32_t) tree->next);
        if (tree->need)squeue_insert(&pre_iter, (uint32_t) tree->need);
        squeue_remove(&pre_iter, 0);
    }
    int count = 0;
    squeue_iter_t siter;
    squeue_iter_begin(&siter, &pre_iter);
    while (true) {
        dynlib_inctree_t *tree = (dynlib_inctree_t *) squeue_iter_next(&siter);
        if (tree == NULL)break;
        CHK(dynlibs_unload_inner(pid, tree->loadinfo, false), "");
        kfree(tree->loadinfo);
        kfree(tree);
        count++;
    }
    pcb->dynlibs_end_addr = 0xA0000000;
    dprintf("unloaded %x dynlibs", count);
    return 0;
    _err:
    return -1;
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
            info->type = PT_DYNLIB_ORIG;
            info->copy_on_fork = false;
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
            case SHT_REL: {
                bool inserted = false;
                for (int i = 0; i < MAX_REL_SECTIONS; i++) {
                    if (edg->s_rel[i] == NULL) {
                        inserted = true;
                        edg->s_rel[i] = shdr;
                        break;
                    }
                }
                if (!inserted) {
                    deprintf("too many rel sections!");
                    return -1;
                }
            }
                break;
            case SHT_RELA: {
                bool inserted = false;
                for (int i = 0; i < MAX_REL_SECTIONS; i++) {
                    if (edg->s_rela[i] == NULL) {
                        inserted = true;
                        edg->s_rela[i] = shdr;
                        break;
                    }
                }
                if (!inserted) {
                    deprintf("too many rela sections!");
                    return -1;
                }
            }
                break;

        }
        shdr = (elf_section_t *) ((uint32_t) shdr + edg->ehdr.e_shentsize);
    }
    /*
    dprintf("relocate elsp's symbols...");
    for (int x = 0; x < MAX_REL_SECTIONS; x++)
        if (edg->s_rel[x] && elsp_relocate(edg, edg->s_rel[x])) {
            deprintf("exception happened when relocating code.(section rel[%x])", x);
            return -1;
        }
    for (int x = 0; x < MAX_REL_SECTIONS; x++)
        if (edg->s_rela[x] && elsp_relocate(edg, edg->s_rela[x])) {
            deprintf("exception happened when relocating code.(section rela[%x])", x);
            return -1;
        }*/
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

int dynlibs_load_need_dynlibs(dynlib_t *lib) {
    elf_digested_t *edg = &lib->edg;
    dprintf("count:%x", edg->dynlibs_need_queue.count);
    squeue_entry_t *i = edg->dynlibs_need_queue.first;
    for (int x = 0; x < edg->dynlibs_need_queue.count && i != NULL; x++) {
        const char *libname = elsp_get_dynstring_by_offset(edg, i->objaddr);
        dprintf("dynlib need(%x):%s", i->objaddr,
                libname);
        if (dynlibs_try_to_load(edg->pid, libname))return -1;
        i = i->next;
    }
    return 0;
}

int dynlibs_fix_shdrs_addr() {

}

int dynlibs_relocate_all(dynlib_load_t *loadinfo) {
    int ret;
    dynlib_t *lib = loaded_dynlibs[loadinfo->no];
    elf_digested_t *edg = &lib->edg;
    pid_t orig_pid = edg->pid;
    edg->pid = loadinfo->pid;
    uint32_t oldoffset = edg->global_offset;
    dprintf("now relocate dynlib(%x)'s symbols...", lib->path);
    edg->global_offset = loadinfo->start_addr;
    for (int x = 0; x < MAX_REL_SECTIONS; x++) {
        if (edg->s_rel[x] && edg->s_rel[x]->sh_addr > oldoffset) {
            uint32_t naddr = (uint32_t) edg->s_rel[x]->sh_addr - oldoffset + edg->global_offset;
            dprintf("fix addr %x --> %x", edg->s_rel[x]->sh_addr, naddr);
            edg->s_rel[x]->sh_addr = naddr;
        }
        if (edg->s_rel[x] && elsp_relocate(edg, edg->s_rel[x], loadinfo->start_addr)) {
            deprintf("exception happened when relocating code.(section rel[%x])", x);
            ret = -1;
            goto _ret;
        }
    }
    for (int x = 0; x < MAX_REL_SECTIONS; x++) {
        if (edg->s_rela[x] && edg->s_rela[x]->sh_addr > oldoffset) {
            edg->s_rela[x]->sh_addr = (uint32_t) edg->s_rela[x]->sh_addr - oldoffset + edg->global_offset;
        }
        if (edg->s_rela[x] && elsp_relocate(edg, edg->s_rela[x], loadinfo->start_addr)) {
            deprintf("exception happened when relocating code.(section rela[%x])", x);
            ret = -1;
            goto _ret;
        }
    }
    ret = 0;
    _ret:
    edg->pid = orig_pid;
    return ret;
}

dynlib_t *dynlibs_do_load_to_memory(pid_t pid, const char *path, dynlib_load_t **loadinfo_out) {
    if (loaded_dynlibs_count >= MAX_LOADED_LIBS) {
        deprintf("Out of dynlibs table...");
        return NULL;
    }
    int8_t fd = kopen(pid, path, 0);
    if (fd < 0) {
        deprintf("fail to open file:%s", path);
        goto _err;
    }
    pcb_t *pcb = getpcb(pid);
    dynlib_t *lib = (dynlib_t *) kmalloc(sizeof(dynlib_t));
    elf_digested_t *edg = &lib->edg;
    memset(lib, 0, sizeof(dynlib_t));
    elsp_init_edg(edg, pid, fd);
    uint32_t liboffset = pcb->dynlibs_end_addr;
    ASSERT(pcb->dynlibs_end_addr >= 0xA0000000);
    edg->global_offset = liboffset;
    CHK(elsp_load_header(edg), "");
    CHK(dynlibs_load_sections(lib, edg), "");
    CHK(dynlibs_load_need_dynlibs(lib), "");
    ASSERT(edg->elf_end_addr > liboffset);
    pcb->dynlibs_end_addr = edg->elf_end_addr + (PAGE_SIZE - (edg->elf_end_addr % PAGE_SIZE));
    memcpy(&lib->edg, edg, sizeof(elf_digested_t));
    lib->path = (char *) kmalloc(strlen(path) + 1);
    strcpy(lib->path, path);
    mutex_lock(&dynlibs_lock);
    loaded_dynlibs_count++;
    uint32_t loadno = 0;
    for (uint32_t x = 0; x < MAX_LOADED_LIBS; x++)
        if (loaded_dynlibs[x] == NULL) {
            loaded_dynlibs[x] = lib;
            loadno = x;
            break;
        }
    mutex_unlock(&dynlibs_lock);
    dynlib_load_t *loadinfo = (dynlib_load_t *) kmalloc(sizeof(dynlib_load_t));
    loadinfo->pid = pid;
    loadinfo->no = loadno;
    loadinfo->start_addr = liboffset;
    if (loadinfo_out)*loadinfo_out = loadinfo;
    CHK(dynlibs_add_to_tree(pid, NULL, loadinfo), "");
    dprintf("loaded lib %s to %x", lib->path, liboffset);
    return lib;
    _err:
    deprintf("fail to loaded lib %s to %x", path, liboffset);
    return NULL;
}

int dynlibs_do_load_to_proc(pid_t pid, uint32_t loadno, dynlib_load_t **loadinfo_out) {
    //TODO check is loaded.
    pcb_t *pcb = getpcb(pid);
    dynlib_t *lib = loaded_dynlibs[loadno];
    dprintf("frame count:%x gf:%x", lib->frames_count, lib->edg.global_offset);
    dynlib_load_t *loadinfo = (dynlib_load_t *) kmalloc(sizeof(dynlib_load_t));
    uint32_t startaddr = pcb->dynlibs_end_addr;
    loadinfo->pid = pid;
    loadinfo->no = loadno;
    loadinfo->start_addr = startaddr;
    for (uint32_t x = 0; x < lib->frames_count; x++) {
        dynlib_frame_t *frame = &lib->frames[x];
        page_t *page = get_page(startaddr + x * PAGE_SIZE, true, pcb->page_dir);
        page_typeinfo_t *tinfo = get_page_type(startaddr + x * PAGE_SIZE, pcb->page_dir);
        page->present = true;
        page->accessed = false;
        page->user = true;
        page->rw = true;
        page->dirty = false;
        page->frame = frame->frameno;
        tinfo->pid = pid;
        tinfo->free_on_proc_exit = false;
        tinfo->type = PT_DYNLIB_ORIG;
        dprintf("[pid:%x]remapping %x -> %x", pid, frame->frameno, startaddr + x * PAGE_SIZE);
    }
    CHK(dynlibs_add_to_tree(pid, NULL, loadinfo), "");
    pcb->dynlibs_end_addr = startaddr + (lib->frames_count + 1) * PAGE_SIZE;
    if (loadinfo_out)*loadinfo_out = loadinfo;
    return 0;
    _err:
    return -1;
}

int dynlibs_try_to_load(pid_t pid, const char *path) {
    dynlib_t *lib;
    uint32_t index;
    dynlib_load_t *loadinfo = NULL;
    if (dynlibs_isloaded_to_memory(path, &index)) {
        lib = loaded_dynlibs[index];
        ASSERT(lib);
        CHK(dynlibs_do_load_to_proc(pid, index, &loadinfo), "");
    } else {
        lib = dynlibs_do_load_to_memory(pid, path, &loadinfo);
        if (lib == NULL)return -1;
    }
    ASSERT(loadinfo != NULL);
    CHK(dynlibs_relocate_all(loadinfo), "");
    dprintf("proc %x loaded dynlib %s", pid, path);
    return 0;
    _err:
    return -1;
}