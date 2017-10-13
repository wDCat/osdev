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
#include "elfloader.h"

int elsp_dynamic_section(elf_digested_t *edg, elf_section_t *shdr) {
    dprintf("processing dynamic section...");
    ASSERT(shdr->sh_type == SHT_DYNAMIC);
    elf_dynamic_t *dht;
    for (int x = 0; x < shdr->sh_size / sizeof(elf_dynamic_t); x++) {
        dht = (elf_dynamic_t *) (shdr->sh_addr + sizeof(elf_dynamic_t) * x);
        dprintf("dynamic sec entry[%x]:tag[%x] ptr[%x]", x, dht->d_tag, dht->d_un.d_ptr);
        switch (dht->d_tag) {
            case DT_NEEDED:
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
} elf_rel_tmp_t;

inline elf_symbol_t *elsp_get_dynsym(elf_digested_t *edg, uint32_t id) {
    if (edg->dyn_entry.dynsymtab == 0) {
        deprintf("dynsym is not ready or not existed.");
        return NULL;
    }
    if (id * sizeof(elf_symbol_t) > edg->dyn_entry.dynsymsz)return NULL;
    return (elf_symbol_t *) (((uint32_t) edg->dyn_entry.dynsymtab) + id * sizeof(elf_symbol_t));
}

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

inline int elsp_rel_386_relative(elf_digested_t *edg, elf_rel_tmp_t *rtmp, uint32_t *ptr) {
    *ptr = ((uint32_t) *ptr) + edg->global_offset;
    return 0;
}

inline int elsp_rel_386_glob_dat(elf_digested_t *edg, elf_rel_tmp_t *rtmp, uint32_t *ptr) {
    elf_symbol_t *sym = elsp_get_dynsym(edg, rtmp->r_dynsym);
    if (sym == NULL)goto _err;
    const char *symname = elsp_get_dynstring_by_offset(edg, sym->st_name);
    dprintf("glob dat name:%s(%x)(%x)", symname, sym->st_name, symname);
    return 0;
    _err:
    return -1;
}

int elsp_rel_do(elf_digested_t *edg, elf_rel_tmp_t *rtmp) {
    uint32_t *ptr = (uint32_t *) (rtmp->r_offset + edg->global_offset);
    switch (rtmp->r_type) {
        case R_386_RELATIVE:
            CHK(elsp_rel_386_relative(edg, rtmp, ptr), "");
            break;
        case R_386_GLOB_DAT:
            CHK(elsp_rel_386_glob_dat(edg, rtmp, ptr), "");
            break;
        default:
            deprintf("Unsupported relocate type:%x at %x", rtmp->r_type, rtmp->r_offset);
            return -1;
    }
    return 0;
    _err:
    return -1;
}

int elsp_relocate(elf_digested_t *edg, elf_section_t *shdr) {
    if (shdr->sh_addr < edg->global_offset) {
        deprintf("bad rel(a) section:%x", shdr->sh_offset);
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
                        .r_addend=NULL
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
                        .r_addend=rel->r_addend
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

int elsp_load_section_data(elf_digested_t *edg, elf_section_t *shdr) {
    ASSERT(edg && shdr);
    pcb_t *pcb = getpcb(edg->pid);
    if (shdr->sh_addr) {
        if (shdr->sh_addr < edg->global_offset)
            shdr->sh_addr += edg->global_offset;
        dprintf("load to %x", shdr->sh_addr);
        uint32_t les = shdr->sh_size;
        uint32_t y = shdr->sh_addr;
        klseek(edg->pid, edg->fd, shdr->sh_offset, SEEK_SET);
        while (les > 0) {
            page_t *page = get_page(y, true, pcb->page_dir);
            uint32_t size = MIN(MIN(PAGE_SIZE, les), PAGE_SIZE - (y % PAGE_SIZE));
            uint32_t foffset = shdr->sh_offset + y - shdr->sh_addr;

            if (shdr->sh_type == SHT_NOBITS) {
                alloc_frame(page, false, true);
                memset(y, 0, size);
                //swap_insert_empty_page(pcb, pno + y);
            } else {
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

bool elf_load(pid_t pid, int8_t fd, uint32_t *entry_point) {
    dprintf("try to load elf pid:%x fd:%x", pid, fd);
    elf_digested_t edg;
    memset(&edg, 0, sizeof(elf_digested_t));
    edg.pid = pid;
    edg.fd = fd;
    edg.global_offset = 0x8000000;
    pcb_t *pcb = getpcb(pid);
    elf_header_t ehdr;
    klseek(pid, fd, 0, SEEK_SET);
    if (kread(pid, fd, sizeof(elf_header_t), &ehdr) != sizeof(elf_header_t)) {
        deprintf("cannot read elf header.");
        goto _err;
    }
    if (!(ehdr.e_ident[EI_MAG0] == ELFMAG0 &&
          ehdr.e_ident[EI_MAG1] == ELFMAG1 &&
          ehdr.e_ident[EI_MAG2] == ELFMAG2 &&
          ehdr.e_ident[EI_MAG3] == ELFMAG3)) {
        deprintf("not a elf file.");
        goto _err;
    }
    if (ehdr.e_machine != 0x3) {
        deprintf("Unsupported ELF type:machine[%x]", ehdr.e_machine);
        goto _err;
    }
    uint32_t shdrsize = ehdr.e_shentsize * ehdr.e_shnum;
    edg.shdrs = (elf_section_t *) kmalloc_paging(shdrsize, NULL);
    elf_section_t *shdr = edg.shdrs;
    klseek(pid, fd, ehdr.e_shoff, SEEK_SET);
    if (kread(pid, fd, shdrsize, shdr) != shdrsize) {
        deprintf("cannot read section info.");
        goto _err;
    }
    for (int x = 0; x < ehdr.e_shnum; x++) {
        dprintf("Section %x(%x) addr:%x size:%x offset:%x type:%x", x, shdr->sh_name, shdr->sh_addr, shdr->sh_size,
                shdr->sh_offset,
                shdr->sh_type);
        elsp_load_section_data(&edg, shdr);
        if (shdr->sh_type == SHT_DYNAMIC) {
            if (elsp_dynamic_section(&edg, shdr)) {
                deprintf("exception happened when processing dynamic section");
                goto _err;
            }
        }
        shdr = (elf_section_t *) ((uint32_t) shdr + ehdr.e_shentsize);
    }
    shdr = edg.shdrs;
    elf_section_t *s_rel = NULL, *s_rela = NULL;
    for (int x = 0; x < ehdr.e_shnum; x++) {
        switch (shdr->sh_type) {
            case SHT_SYMTAB:
                if (shdr->sh_addr == 0) {
                    //.symtab
                    dprintf("set section %x as symtab.", x);
                    edg.symbols = (elf_symbol_t *) kmalloc(shdr->sh_size);
                    klseek(pid, fd, shdr->sh_offset, SEEK_SET);
                    if (kread(pid, fd, shdr->sh_size, (uchar_t *) edg.symbols) != shdr->sh_size) {
                        deprintf("i/o error when read sym section");
                        goto _err;
                    }
                    edg.symbols_size = shdr->sh_size;
                }
                break;
            case SHT_DYNSYM:
                if (edg.dyn_entry.dynsymtab != 0 && edg.dyn_entry.dynsymtab != shdr->sh_offset) {
                    deprintf("Bad dynstmtab address!");
                    goto _err;
                }
                if (shdr->sh_addr == 0) {
                    deprintf("Bad dynstm section.");
                    goto _err;
                }
                edg.dyn_entry.dynsymtab = shdr->sh_addr;
                edg.dyn_entry.dynsymsz = shdr->sh_size;
                break;
            case SHT_STRTAB:
                if (shdr->sh_offset == edg.dyn_entry.dynstrtab) {
                    //.dynstr section
                    edg.dyn_strings = (char *) shdr->sh_addr;
                } else if (edg.strings == NULL) {
                    dprintf("set section %x as strtab.", x);
                    edg.strings = (char *) kmalloc(shdr->sh_size);
                    klseek(pid, fd, shdr->sh_offset, SEEK_SET);
                    if (kread(pid, fd, shdr->sh_size, (uchar_t *) edg.strings) != shdr->sh_size) {
                        deprintf("i/o error when read str section");
                        goto _err;
                    }
                    edg.strings_size = shdr->sh_size;
                } else
                    dprintf("ignore strtab:%x", x);
                break;
            case SHT_REL:
                s_rel = shdr;
                break;
            case SHT_RELA:
                s_rela = shdr;
                break;

        }
        shdr = (elf_section_t *) ((uint32_t) shdr + ehdr.e_shentsize);
    }
    if ((s_rel && elsp_relocate(&edg, s_rel)) || (s_rela && elsp_relocate(&edg, s_rela))) {
        deprintf("exception happened when relocating code.");
        goto _err;
    }
    dprintf("elf entry point:%x", ehdr.e_entry);
    if (entry_point)
        *entry_point = ehdr.e_entry + edg.global_offset;
    dprintf("elf load done.");
    return 0;
    _err:
    //TODO free edg
    deprintf("error happened during elf loading..");
    return 1;
}

void elf_test() {
    extern fs_t ext2_fs;
    extern fs_t catmfs;
    extern blk_dev_t dev;
    ext2_create_fstype();
    vfs_t *vfs = (vfs_t *) kmalloc(sizeof(vfs_t));
    vfs_init(vfs);
    vfs_mount(vfs, "/", &ext2_fs, &dev);
    int fd = sys_open("/a.elf", 0);
    uint32_t entry;
    elf_load(1, fd, &entry);
    __asm__ __volatile__("jmp %0;"::"m"(entry));
    putf_const("elf test done.");
    return;
    /*
    file = (uint8_t *) kmalloc_paging(0x2000, NULL);
    int32_t ret = sys_read(fd, 0x2000, file);
    putf_const("read %x bytes.\n", ret);
    putf_const("file addr:%x\n", file);
    elf_header_t *ehdr = (elf_header_t *) file;
    if (ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
        ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
        ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
        ehdr->e_ident[EI_MAG3] == ELFMAG3) {
        putf_const("good elf file\n");
    } else PANIC("bad elf file.\n");
    putf("entry point:%x\n", ehdr->e_entry);
    elf_program_t *ephdr = (elf_program_t *) ((uint32_t) file + ehdr->e_phoff);
    for (int x = 0; x < ehdr->e_phnum; x++) {
        putf_const("PH addr:%x offset:%x size:%x\n", ephdr->p_vaddr, ephdr->p_offset, ephdr->p_memsz);
        ephdr = (elf_program_t *) ((uint32_t) ephdr + ehdr->e_phentsize);
    }
    putln_const("");
    elf_section_t *eshdr = (elf_section_t *) ((uint32_t) file + ehdr->e_shoff);
    for (int x = 0; x < ehdr->e_shnum; x++) {
        putf_const("SH addr:%x offset:%x size:%x\n", eshdr->sh_addr, eshdr->sh_addralign, eshdr->sh_size);
        if (eshdr->sh_addr) {
            for (uint32_t y = 0; y < eshdr->sh_size; y += 0x1000) {
                alloc_frame(get_page(eshdr->sh_addr + x, true, current_dir), false, false);
            }
            memcpy(eshdr->sh_addr, (uint32_t) file + eshdr->sh_offset, eshdr->sh_size);
        }
        eshdr = (elf_section_t *) ((uint32_t) eshdr + ehdr->e_shentsize);

    }
    if (file)kfree(file);
    __asm__ __volatile__("jmp %0;"::"m"(ehdr->e_entry));*/


}