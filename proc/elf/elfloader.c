//
// Created by dcat on 8/30/17.
//

#include <vfs.h>
#include "kmalloc.h"
#include <ext2.h>
#include <str.h>
#include <elf.h>
#include <proc.h>
#include "../swap/include/swap.h"
#include <page.h>
#include <elfloader.h>
#include <squeue.h>
#include <dynlibs.h>
#include "elfloader.h"

inline const char *elsp_get_string_by_offset(elf_digested_t *edg, uint32_t offset) {
    if (edg->strings == 0) {
        deprintf("strings is not ready or not existed.");
        return NULL;
    }
    if (offset > edg->strings_size)return NULL;
    return edg->strings + offset;
}

inline const char *elsp_get_dynstring_by_offset(elf_digested_t *edg, uint32_t offset) {
    if (edg->dyn_strings == 0) {
        deprintf("dynstrings is not ready or not existed.");
        return NULL;
    }
    if (offset > edg->dyn_entry.dynstrsz)return NULL;
    return (const char *) (edg->dyn_strings + offset);
}

int elsp_dynamic_section(elf_digested_t *edg, elf_section_t *shdr) {
    dprintf("processing dynamic section...");
    ASSERT(shdr->sh_type == SHT_DYNAMIC);
    elf_dynamic_t *dht;
    for (int x = 0; x < shdr->sh_size / sizeof(elf_dynamic_t); x++) {
        dht = (elf_dynamic_t *) (shdr->sh_addr + sizeof(elf_dynamic_t) * x);
        dprintf("dynamic sec entry[%x]:tag[%x] ptr[%x]", x, dht->d_tag, dht->d_un.d_ptr);
        switch (dht->d_tag) {
            case DT_NEEDED: {
                dprintf("need name(%x)", dht->d_un.d_val);
                squeue_insert(&edg->dynlibs_need_queue, dht->d_un.d_val);
            }
                break;
            case DT_STRSZ:
                edg->dyn_entry.dynstrsz = dht->d_un.d_val;
                break;
            case DT_STRTAB:
                edg->dyn_entry.dynstrtab = dht->d_un.d_ptr;
                break;
            case DT_SYMTAB:
                edg->dyn_entry.dynsymtab = dht->d_un.d_ptr;
                break;
            case DT_PLTGOT:
                edg->dyn_entry.pltgot = dht->d_un.d_ptr;
        }
        if (dht->d_tag == 0)break;
    }
    return 0;
}

int elsp_load_strings(elf_digested_t *edg) {
    if (edg->dyn_entry.dynstrtab == NULL) {
        deprintf("strtab is not defined in dyn section");
        goto _err;
    }

    _err:
    return 1;

}

typedef struct {
    uint32_t shdrtype;
    uint32_t r_offset;
    int32_t r_addend;
    uint8_t r_type;
    uint32_t r_dynsym;
    uint32_t global_offset;
} elf_rel_tmp_t;

inline elf_symbol_t *elsp_get_dynsym(elf_digested_t *edg, uint32_t id) {
    if (edg->dyn_entry.dynsymtab == 0) {
        deprintf("dynsym is not ready or not existed.");
        return NULL;
    }
    if (id * sizeof(elf_symbol_t) > edg->dyn_entry.dynsymsz)return NULL;
    return (elf_symbol_t *) (((uint32_t) edg->dyn_entry.dynsymtab) + id * sizeof(elf_symbol_t));
}


inline int elsp_rel_386_add_offset(elf_digested_t *edg, elf_rel_tmp_t *rtmp, uint32_t *ptr) {
    *ptr = ((uint32_t) *ptr) + rtmp->global_offset;
    return 0;
}

inline int elsp_rel_386_symtab(elf_digested_t *edg, elf_rel_tmp_t *rtmp, uint32_t *ptr) {
    elf_symbol_t *sym = elsp_get_dynsym(edg, rtmp->r_dynsym);
    pcb_t *pcb = getpcb(edg->pid);
    if (sym == NULL)goto _err;
    const char *symname = elsp_get_dynstring_by_offset(edg, sym->st_name);
    dprintf("glob dat name:%s(%x)(%x)", symname, sym->st_name, symname);
    uint32_t addr = 0x0;
    int from = 0;
    if (dynlibs_find_symbol(edg->pid, symname, &addr)) {
        if (pcb->edg == NULL || elsp_find_symbol(pcb->edg, symname, &addr)) {
            if (elsp_find_symbol(edg, symname, &addr)) {
                dprintf("[WARN]symbol %s not found!", symname);
                goto _err;
            } else {
                addr = addr + rtmp->global_offset;
                from = 2;
            }
        } else {
            addr = addr + pcb->edg->global_offset;
            from = 1;
        }
    }
    dprintf("update symbol %s(at %x) to %x (from:%x)[bedg:%x]",
            symname, ptr, addr, from, pcb->edg);
    *ptr = addr;
    return 0;
    _err:
    return 0;//ignored
}

int elsp_find_symbol(elf_digested_t *edg, const char *name, uint32_t *out) {
    for (uint32_t x = 0;; x++) {
        elf_symbol_t *esym = elsp_get_dynsym(edg, x);
        if (esym == NULL)break;
        const char *symname = elsp_get_dynstring_by_offset(edg, esym->st_name);
        if (symname == NULL)continue;
        if (strcmp(symname, name) && esym->st_value != 0) {
            if (out) {
                *out = esym->st_value + 0x0;
                return 0;
            }
        }
    }
    return -1;
}

int elsp_rel_do(elf_digested_t *edg, elf_rel_tmp_t *rtmp) {
    uint32_t *ptr = (uint32_t *) (rtmp->r_offset + edg->global_offset);
    if (edg->ehdr.e_type == ET_DYN) {
        dynlibs_try_to_write(edg->pid, ptr);
    }
    uint32_t origval = *ptr;
    switch (rtmp->r_type) {
        case R_386_RELATIVE:
            CHK(elsp_rel_386_add_offset(edg, rtmp, ptr), "");
            break;
        case R_386_32:
        case R_386_GLOB_DAT:
        case R_386_JMP_SLOT:
            CHK(elsp_rel_386_symtab(edg, rtmp, ptr), "");
            break;
        default:
            deprintf("Unsupported relocate type:%x at %x", rtmp->r_type, rtmp->r_offset);
            PANIC("debug...")

    }
    dprintf("relocate done [%x] %x ==> %x", ptr, origval, *ptr);
    return 0;
    _err:
    return -1;
}

int elsp_relocate(elf_digested_t *edg, elf_section_t *shdr, uint32_t global_offset) {
    if (shdr->sh_addr < edg->global_offset) {
        deprintf("bad rel(a) section:vaddr:%x offset:%x", shdr->sh_addr, shdr->sh_offset);
        return -1;
    }
    switch (shdr->sh_type) {
        case SHT_REL: {
            elf_rel_t *rel = (elf_rel_t *) shdr->sh_addr;
            while ((uint32_t) rel < shdr->sh_addr + shdr->sh_size) {
                dprintf("rel item: offset:%x", rel->r_offset);
                elf_rel_tmp_t rtmp = {
                        .r_type=SHT_REL,
                        .r_offset=rel->r_offset,
                        .r_type=(uint8_t) ELF32_R_TYPE(rel->r_info),
                        .r_dynsym=ELF32_R_DYNSYM(rel->r_info),
                        .r_addend=NULL,
                        .global_offset=global_offset
                };
                if (elsp_rel_do(edg, &rtmp)) {
                    deprintf("relocated failed.");
                    return -1;
                }
                rel = (elf_rel_t *) (((uint32_t) rel) + sizeof(elf_rel_t));
            }

            return 0;
        }

        case SHT_RELA: {
            elf_rela_t *rel = (elf_rela_t *) shdr->sh_addr;
            while ((uint32_t) rel < shdr->sh_addr + shdr->sh_size) {
                dprintf("rela item: offset:%x", rel->r_offset);
                elf_rel_tmp_t rtmp = {
                        .r_type=SHT_RELA,
                        .r_offset=rel->r_offset,
                        .r_type=(uint8_t) ELF32_R_TYPE(rel->r_info),
                        .r_dynsym=ELF32_R_DYNSYM(rel->r_info),
                        .r_addend=rel->r_addend,
                        .global_offset=global_offset
                };
                if (elsp_rel_do(edg, &rtmp)) {
                    deprintf("relocated failed.");
                    return -1;
                }
                rel = (elf_rela_t *) (((uint32_t) rel) + sizeof(elf_rela_t));
            }
            return 0;
        }

        default:
            return -1;
    }
}

int elsp_load_segment_data(elf_digested_t *edg, elf_program_t *phdr) {
    TODO;
    ASSERT(edg && phdr);
    pcb_t *pcb = getpcb(edg->pid);
    if (phdr->p_vaddr) {
        if (phdr->p_vaddr + phdr->p_filesz > edg->elf_end_addr) {
            edg->elf_end_addr = phdr->p_vaddr + phdr->p_filesz;
        }
        dprintf("load segment to %x global_offset:%x", phdr->p_filesz,
                edg->global_offset);
        uint32_t les = phdr->p_filesz;
        uint32_t y = phdr->p_vaddr;
        klseek(edg->pid, edg->fd, phdr->p_offset, SEEK_SET);
        while (les > 0) {
            page_t *page = get_page(y, true, pcb->page_dir);
            page_typeinfo_t *info = get_page_type(y, pcb->page_dir);
            info->pid = pcb->pid;
            info->free_on_proc_exit = true;
            uint32_t size = MIN(MIN(PAGE_SIZE, les), PAGE_SIZE - (y % PAGE_SIZE));
            uint32_t foffset = phdr->p_offset + y - phdr->p_vaddr;
            if (size == PAGE_SIZE) {
                swap_insert_pload_page(pcb, y, edg->fd, foffset);
            } else {
                alloc_frame(page, false, true);
                klseek(edg->pid, edg->fd, foffset, SEEK_SET);
                if (kread(edg->pid, edg->fd, size, y) != size) {
                    deprintf("cannot read section:%x.I/O error.", y);
                    goto _err;
                }
            }
            y += size;
            les -= size;
        }
    }
    return 0;
    _err:
    return -1;
}

int elsp_load_segments(elf_digested_t *edg) {
    elf_header_t *ehdr = &edg->ehdr;
    uint32_t psize = ehdr->e_phentsize * ehdr->e_phnum;
    elf_program_t *phdr = (elf_program_t *) kmalloc(psize);
    edg->phdrs = phdr;
    klseek(edg->pid, edg->fd, ehdr->e_phoff, SEEK_SET);
    if (kread(edg->pid, edg->fd, psize, phdr) != psize) {
        deprintf("cannot read program headers.");
        goto _err;
    }
    for (int x = 0; x < ehdr->e_phnum; x++) {
        dprintf("Segment %x vaddr:%x off:%x size:%x", x, phdr->p_vaddr,
                phdr->p_offset,
                phdr->p_filesz);
        if (phdr->p_vaddr != 0) {
            phdr->p_vaddr = (uint32_t) phdr->p_vaddr + edg->global_offset;
            phdr->p_paddr = phdr->p_vaddr;
            CHK(elsp_load_segment_data(edg, phdr), "fail to load segment %x", x);
        }

        phdr = (elf_program_t *) ((uint32_t) phdr + ehdr->e_phentsize);
    }
    return 0;
    _err:
    return 1;
}

int elsp_load_section_data(elf_digested_t *edg, elf_section_t *shdr) {
    ASSERT(edg && shdr);
    pcb_t *pcb = getpcb(edg->pid);
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
            page_t *page = get_page(y, true, pcb->page_dir);
            page_typeinfo_t *info = get_page_type(y, pcb->page_dir);
            info->pid = pcb->pid;
            info->free_on_proc_exit = true;
            info->copy_on_fork = true;
            uint32_t size = MIN(MIN(PAGE_SIZE, les), PAGE_SIZE - (y % PAGE_SIZE));
            uint32_t foffset = shdr->sh_offset + y - shdr->sh_addr;
            if (shdr->sh_type == SHT_NOBITS) {
                alloc_frame(page, false, true);
                memset(y, 0, size);
                //swap_insert_empty_page(pcb, pno + y);
            } else {
                if (size == PAGE_SIZE && false) {//blocked for debug
                    swap_insert_pload_page(pcb, y, edg->fd, foffset);
                } else {
                    alloc_frame(page, false, true);
                    klseek(edg->pid, edg->fd, foffset, SEEK_SET);
                    if (kread(edg->pid, edg->fd, size, y) != size) {
                        deprintf("cannot read section:%x.I/O error.", y);
                        goto _err;
                    }
                }
            }
            y += size;
            les -= size;
        }
    }
    return 0;
    _err:
    return -1;
}

int elsp_relocate_got(elf_digested_t *edg) {
    //rewrite _GOT_
    uint32_t *got = NULL;
    if (edg->dyn_entry.pltgot == NULL) {
        if (edg->symbols == NULL) {
            deprintf("symbols table not found!");
            goto _err;
        }
        elf_symbol_t *symbol = edg->symbols;
        for (int x = 0; x * sizeof(elf_symbol_t) < edg->symbols_size; x++) {
            dprintf("name[%d]:%d %x %x", x, symbol->st_name, symbol->st_value, symbol);
            char *name = edg->strings + symbol->st_name;
            if (strcmp(name, "_GLOBAL_OFFSET_TABLE_")) {
                got = (uint32_t *) symbol->st_value;
                dprintf("found GOT:%x", got);
                break;
            }
            symbol = (elf_symbol_t *) (((uint32_t) symbol) + sizeof(elf_symbol_t));
        }
    } else {
        got = (uint32_t *) (edg->global_offset + edg->dyn_entry.pltgot);
    }

    if (got == NULL) {
        deprintf("_GLOBAL_OFFSET_TABLE_ not found.");
        goto _err;
    }
    got = (uint32_t *) ((uint32_t) got + edg->global_offset);
    dprintf("_GOT_ addr:%x val:%x", got, *got);
    uint32_t oldg = *got;
    oldg = oldg + edg->global_offset;
    *got = oldg;
    return 0;
    _err:
    return -1;
}

int elsp_load_need_dynlibs(elf_digested_t *edg) {
    dprintf("count:%x", edg->dynlibs_need_queue.count);
    squeue_entry_t *i = edg->dynlibs_need_queue.first;
    for (int x = 0; x < edg->dynlibs_need_queue.count && i != NULL; x++) {
        const char *libname = elsp_get_dynstring_by_offset(edg, i->objaddr);
        dprintf("dynlib need(%x):%s", i->objaddr,
                libname);
        if (dynlibs_try_to_load(edg->pid, libname))return -1;
        i = i->next;
    }
    dprintf("now relocate symbols...");
    for (int x = 0; x < MAX_REL_SECTIONS; x++)
        if (edg->s_rel[x] && elsp_relocate(edg, edg->s_rel[x], edg->global_offset)) {
            deprintf("exception happened when relocating code.(section rel[%x])", x);
            return -1;
        }
    for (int x = 0; x < MAX_REL_SECTIONS; x++)
        if (edg->s_rela[x] && elsp_relocate(edg, edg->s_rela[x], edg->global_offset)) {
            deprintf("exception happened when relocating code.(section rela[%x])", x);
            return -1;
        }
    return 0;
}

int elsp_load_sections(elf_digested_t *edg) {
    elf_header_t *ehdr = &edg->ehdr;
    uint32_t shdrsize = ehdr->e_shentsize * ehdr->e_shnum;
    edg->shdrs = (elf_section_t *) kmalloc_paging(shdrsize, NULL);
    elf_section_t *shdr = edg->shdrs;
    klseek(edg->pid, edg->fd, ehdr->e_shoff, SEEK_SET);
    if (kread(edg->pid, edg->fd, shdrsize, shdr) != shdrsize) {
        deprintf("cannot read section info.");
        goto _err;
    }
    for (int x = 0; x < edg->ehdr.e_shnum; x++) {
        dprintf("Section %x(%x) addr:%x size:%x offset:%x type:%x", x, shdr->sh_name, shdr->sh_addr, shdr->sh_size,
                shdr->sh_offset,
                shdr->sh_type);
        elsp_load_section_data(edg, shdr);
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
    if ((edg->s_rel && elsp_relocate(edg, edg->s_rel)) ||
        (edg->s_rela && elsp_relocate(edg, edg->s_rela))) {
        deprintf("exception happened when relocating code.");
        goto _err;
    }*/
    return 0;
    _err:
    return 1;
}

int elsp_load_header(elf_digested_t *edg) {
    elf_header_t *ehdr = &edg->ehdr;
    klseek(edg->pid, edg->fd, 0, SEEK_SET);
    if (kread(edg->pid, edg->fd, sizeof(elf_header_t), ehdr) != sizeof(elf_header_t)) {
        deprintf("cannot read elf header.");
        goto _err;
    }
    if (!(ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
          ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
          ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
          ehdr->e_ident[EI_MAG3] == ELFMAG3)) {
        deprintf("not a elf file.");
        goto _err;
    }
    if (ehdr->e_machine != 0x3) {
        deprintf("Unsupported ELF type:machine[%x]", ehdr->e_machine);
        goto _err;
    }
    return 0;
    _err:
    return -1;
}

int elsp_init_edg(elf_digested_t *edg, pid_t pid, int8_t fd) {
    memset(edg, 0, sizeof(elf_digested_t));
    edg->pid = pid;
    edg->fd = fd;
    squeue_init(&edg->dynlibs_need_queue);
}

int elsp_free_edg(elf_digested_t *edg) {
    if (edg->shdrs) {
        kfree(edg->shdrs);
        edg->shdrs = NULL;
    }
    if (edg->phdrs) {
        kfree(edg->phdrs);
        edg->phdrs = NULL;
    }
    if (edg->symbols) {
        kfree(edg->symbols);
        edg->symbols = NULL;
    }
    if (edg->strings) {
        kfree(edg->strings);
        edg->strings = NULL;
    }
    return 0;
}

uint32_t elsp_get_entry(elf_digested_t *edg) {
    return (uint32_t) edg->global_offset + edg->ehdr.e_entry;
}


bool elf_load(pid_t pid, int8_t fd, uint32_t *entry_point, uint32_t *elf_end) {
    dprintf("try to load elf pid:%x fd:%x", pid, fd);
    elf_digested_t edg;
    elsp_init_edg(&edg, pid, fd);
    edg.global_offset = 0x8000000;
    pcb_t *pcb = getpcb(pid);
    CHK(elsp_load_header(&edg), "");
    CHK(elsp_load_sections(&edg), "");
    elf_header_t *ehdr = &edg.ehdr;
    dprintf("elf entry point:%x", ehdr->e_entry);
    if (entry_point)
        *entry_point = ehdr->e_entry + edg.global_offset;
    dprintf("elf load done.");
    return 0;
    _err:
    //TODO free edg
    deprintf("error happened during elf loading..");
    return 1;
}
