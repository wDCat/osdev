//
// Created by dcat on 8/30/17.
//

#ifndef W2_ELFLOADER_H
#define W2_ELFLOADER_H

#include <squeue.h>
#include "elf.h"

typedef struct {
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
    char *strings;
    uint32_t strings_size;
    elf_symbol_t *symbols;
    uint32_t symbols_size;
    elf_section_t *shdrs;
    uint32_t elf_end_addr;
    squeue_t dynlibs_need_queue;
} elf_digested_t;

int elsp_init_edg(elf_digested_t *edg, pid_t pid, int8_t fd);

uint32_t elsp_get_entry(elf_digested_t *edg);

int elsp_load_header(elf_digested_t *edg);

int elsp_load_sections(elf_digested_t *edg);

int elsp_load_need_dynlibs(elf_digested_t *edg);

bool elf_load(pid_t pid, int8_t fd, uint32_t *entry_point, uint32_t *elf_end);


#endif //W2_ELFLOADER_H
