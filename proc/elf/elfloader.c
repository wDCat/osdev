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
#include "elfloader.h"


bool elf_load(pid_t pid, int8_t fd, uint32_t *entry_point) {
    dprintf("try to load elf pid:%x fd:%x", pid, fd);
    pcb_t *pcb = getpcb(pid);
    elf_header_t ehdr;
    uint32_t shdr_ptr = 0;
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
    shdr_ptr = kmalloc_paging(shdrsize, NULL);
    elf_section_t *shdr = (elf_section_t *) shdr_ptr;
    klseek(pid, fd, ehdr.e_shoff, SEEK_SET);
    if (kread(pid, fd, shdrsize, shdr) != shdrsize) {
        deprintf("cannot read section info.");
        goto _err;
    }
    for (int x = 0; x < ehdr.e_shnum; x++) {
        dprintf("Section %x addr:%x size:%x offset:%x type:%x", x, shdr->sh_addr, shdr->sh_size, shdr->sh_offset,
                shdr->sh_type);
        if (shdr->sh_addr) {
            uint32_t les = shdr->sh_size;
            uint32_t y = shdr->sh_addr;
            klseek(pid, fd, shdr->sh_offset, SEEK_SET);
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
                        swap_insert_pload_page(pcb, y, fd, foffset);
                    } else {
                        alloc_frame(page, false, true);
                        klseek(pid, fd, foffset, SEEK_SET);
                        if (kread(pid, fd, size, y) != size) {
                            deprintf("cannot read section:%x:%x.I/O error.", x, y);
                            goto _err;
                        }
                    }
                }
                y += size;
                les -= size;
            }
        }
        shdr = (elf_section_t *) ((uint32_t) shdr + ehdr.e_shentsize);
    }
    dprintf("elf entry point:%x", ehdr.e_entry);
    if (entry_point)
        *entry_point = ehdr.e_entry;
    dprintf("elf load done.");
    if (shdr_ptr)
        kfree(shdr_ptr);
    return 0;
    _err:
    if (shdr_ptr)
        kfree(shdr_ptr);
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