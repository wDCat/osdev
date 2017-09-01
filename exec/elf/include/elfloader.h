//
// Created by dcat on 8/30/17.
//

#ifndef W2_ELFLOADER_H
#define W2_ELFLOADER_H

#include "elf.h"

void elf_load(pid_t pid, int8_t fd, uint32_t *entry_point);

#endif //W2_ELFLOADER_H
