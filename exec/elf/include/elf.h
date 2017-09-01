//
// Created by dcat on 8/30/17.
//

#ifndef W2_ELF_H
#define W2_ELF_H

#include <intdef.h>

#define EI_NIDENT    16

#define    EI_MAG0        0        /* e_ident[] indexes */
#define    EI_MAG1        1
#define    EI_MAG2        2
#define    EI_MAG3        3
#define    EI_CLASS    4
#define    EI_DATA        5
#define    EI_VERSION    6
#define    EI_OSABI    7
#define    EI_PAD        8

#define    ELFMAG        "\177ELF"
#define    ELFMAG0        0x7f        /* EI_MAG */
#define    ELFMAG1        'E'
#define    ELFMAG2        'L'
#define    ELFMAG3        'F'

#define SHT_NULL    0
#define SHT_PROGBITS    1
#define SHT_SYMTAB    2
#define SHT_STRTAB    3
#define SHT_RELA    4
#define SHT_HASH    5
#define SHT_DYNAMIC    6
#define SHT_NOTE    7
#define SHT_NOBITS    8
#define SHT_REL        9
#define SHT_SHLIB    10
#define SHT_DYNSYM    11
#define SHT_NUM        12
#define SHT_LOPROC    0x70000000
#define SHT_HIPROC    0x7fffffff
#define SHT_LOUSER    0x80000000
#define SHT_HIUSER    0xffffffff

/* sh_flags */
#define SHF_WRITE        0x1
#define SHF_ALLOC        0x2
#define SHF_EXECINSTR        0x4
#define SHF_RELA_LIVEPATCH    0x00100000
#define SHF_RO_AFTER_INIT    0x00200000
#define SHF_MASKPROC        0xf0000000

/* special section indexes */
#define SHN_UNDEF    0
#define SHN_LORESERVE    0xff00
#define SHN_LOPROC    0xff00
#define SHN_HIPROC    0xff1f
#define SHN_LIVEPATCH    0xff20
#define SHN_ABS        0xfff1
#define SHN_COMMON    0xfff2
#define SHN_HIRESERVE    0xffff

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf_header_t;
typedef struct elf_shdr {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
} elf_section_t;
typedef struct elf_phdr {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} elf_program_t;
#endif //W2_ELF_H
