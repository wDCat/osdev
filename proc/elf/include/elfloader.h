//
// Created by dcat on 8/30/17.
//

#ifndef W2_ELFLOADER_H
#define W2_ELFLOADER_H

#include "elf.h"

typedef struct {
    pid_t pid;
    int8_t fd;
    uint32_t global_offset;
    struct {
        int32_t dynstrsz;
        uint32_t dynstrtab;
        uint32_t dynsymtab;
        uint32_t dynsymsz;
        uint32_t pltgot;
    } dyn_entry;
    char *dyn_strings;
    char* strings;
    uint32_t strings_size;
    elf_symbol_t *symbols;
    uint32_t symbols_size;
    elf_section_t *shdrs;
} elf_digested_t;

bool elf_load(pid_t pid, int8_t fd, uint32_t *entry_point);

#endif //W2_ELFLOADER_H
