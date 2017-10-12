//
// Created by dcat on 8/30/17.
//

#ifndef W2_ELFLOADER_H
#define W2_ELFLOADER_H

#include "elf.h"

typedef struct {
    pid_t pid;
    int8_t fd;
    struct {
        uint32_t strsz;
        uint32_t strtab;
        uint32_t symtab;
    } dyn_entry;
    uint32_t cached_symtab;
} elf_digested_t;

bool elf_load(pid_t pid, int8_t fd, uint32_t *entry_point);

#endif //W2_ELFLOADER_H
