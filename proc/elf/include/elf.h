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

#define ET_NONE   0
#define ET_REL    1
#define ET_EXEC   2
#define ET_DYN    3

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

#define DT_NULL      0
#define DT_NEEDED    1
#define DT_PLTGOT    3
#define DT_STRTAB    5
#define DT_SYMTAB    6
#define DT_STRSZ    10
#define DT_SYMENT    11
typedef struct dynamic {
    int32_t d_tag;
    union {
        int32_t d_val;
        uint32_t d_ptr;
    } d_un;
} elf_dynamic_t;
#define ELF32_R_DYNSYM(x) ((x) >> 8)
#define ELF32_R_TYPE(x) ((x) & 0xff)
#define    R_386_NONE        0
#define    R_386_32        1
#define    R_386_PC32        2
#define    R_386_GOT32        3
#define    R_386_PLT32        4
#define    R_386_COPY        5
#define    R_386_GLOB_DAT        6
#define    R_386_JMP_SLOT        7
#define    R_386_RELATIVE        8
#define    R_386_GOTOFF        9
#define    R_386_GOTPC        10
#define    R_386_32PLT        11
typedef struct elf32_sym {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    unsigned char st_info;
    unsigned char st_other;
    uint16_t st_shndx;
} elf_symbol_t;
typedef struct elf_rel {
    uint32_t r_offset;
    uint32_t r_info;
} elf_rel_t;
typedef struct elf_rela {
    uint32_t r_offset;
    uint32_t r_info;
    int32_t r_addend;
} elf_rela_t;
#define AT_NULL		0
#define AT_IGNORE	1
#define AT_EXECFD	2
#define AT_PHDR		3
#define AT_PHENT	4
#define AT_PHNUM	5
#define AT_PAGESZ	6
#define AT_BASE		7
#define AT_FLAGS	8
#define AT_ENTRY	9
#define AT_NOTELF	10
#define AT_UID		11
#define AT_EUID		12
#define AT_GID		13
#define AT_EGID		14
#define AT_PLATFORM	15
#define AT_HWCAP	16
#define AT_FPUCW	18
#define AT_DCACHEBSIZE	19
#define AT_ICACHEBSIZE	20
#define AT_UCACHEBSIZE	21
#define AT_IGNOREPPC	22
#define	AT_SECURE	23
#define AT_BASE_PLATFORM 24
#define AT_RANDOM	25
#define AT_HWCAP2	26
#define AT_EXECFN	31
#define AT_SYSINFO	32
#define AT_SYSINFO_EHDR	33

typedef struct {
    uint32_t a_type;
    union {
        uint32_t a_val;
    } a_un;
} elf_auxv_t;
#endif //W2_ELF_H
