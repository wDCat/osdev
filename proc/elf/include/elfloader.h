//
// Created by dcat on 8/30/17.
//

#ifndef W2_ELFLOADER_H
#define W2_ELFLOADER_H

#include <squeue.h>
#include "elf.h"

#define MAX_REL_SECTIONS 2
typedef struct elf_digested {
    uint32_t type;
    pid_t pid;
    int8_t fd;
    uint32_t global_offset;
    elf_header_t ehdr;
    struct {
        int32_t dynstrsz;
        uint32_t dynstrtab;
        uint32_t dynsymtab;
        uint32_t dynsymsz;
        uint32_t pltgot;
    } dyn_entry;
    char *dyn_strings;
    char *strings;//free
    uint32_t strings_size;
    elf_symbol_t *symbols;//free
    uint32_t symbols_size;
    elf_section_t *shdrs;//free
    elf_program_t *phdrs;//free
    uint32_t elf_end_addr;
    squeue_t dynlibs_need_queue;
    elf_section_t *s_rel[MAX_REL_SECTIONS],
            *s_rela[MAX_REL_SECTIONS];
    struct elf_digested *base;
} elf_digested_t;

int elsp_init_edg(elf_digested_t *edg, pid_t pid, int8_t fd);

uint32_t elsp_get_entry(elf_digested_t *edg);

int elsp_load_segments(elf_digested_t *edg);

int elsp_dynamic_section(elf_digested_t *edg, elf_section_t *shdr);

const char *elsp_get_string_by_offset(elf_digested_t *edg, uint32_t offset);

elf_symbol_t *elsp_get_dynsym(elf_digested_t *edg, uint32_t id);

const char *elsp_get_dynstring_by_offset(elf_digested_t *edg, uint32_t offset);

int elsp_load_header(elf_digested_t *edg);

int elsp_load_sections(elf_digested_t *edg);

int elsp_load_need_dynlibs(elf_digested_t *edg);

int elsp_free_edg(elf_digested_t *edg);

int elsp_relocate(elf_digested_t *edg, elf_section_t *shdr, uint32_t global_offset);

int elsp_find_symbol(elf_digested_t *edg, const char *name, uint32_t *out);

bool elf_load(pid_t pid, int8_t fd, uint32_t *entry_point, uint32_t *elf_end);

int elsp_find_dynsymbol(elf_digested_t *edg, const char *name, uint32_t *out);

#endif //W2_ELFLOADER_H
