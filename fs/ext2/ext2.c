//
// Created by dcat on 8/17/17.
//
#include "include/ext2.h"
#include "ide.h"

ext2_t ext2_fs;

int ata_section_read(uint32_t offset, uint32_t len, uint8_t *buff) {
    if (offset % SECTION_SIZE != 0) {
        PANIC("offset and len must be block align!");
    }
    uint32_t blk_no = (offset + 0x100000) / SECTION_SIZE;
    uint32_t blk_count = len / SECTION_SIZE;
    uint8_t err;
    uint8_t tbuff[SECTION_SIZE];
    if (len % SECTION_SIZE == 0) {
        err = ide_ata_access(ATA_READ, 1, blk_no, blk_count, buff);
        ASSERT(err == 0);
    } else {
        if (blk_count >= 1) {
            err = ide_ata_access(ATA_READ, 1, blk_no, blk_count, buff);
            ASSERT(err == 0);
        }
        err = ide_ata_access(ATA_READ, 1, blk_no + blk_count, 1, tbuff);
        ASSERT(err == 0);
        putf_const("[+] copy %x to %x size %x", buff + (blk_count - 1) * SECTION_SIZE, buff, len % SECTION_SIZE);
        memcpy(buff + (blk_count) * SECTION_SIZE, tbuff, len % SECTION_SIZE);
        //for (;;);
    }
    return err;
}

int ata_section_write(uint32_t offset, uint32_t len, uint8_t *buff) {
    return 1;
}

void ext2_init() {
    //Test code...
    memset(&ext2_fs, 0, sizeof(ext2_t));
    ext2_fs.section_read = &ata_section_read;
    ext2_fs.section_write = &ata_section_write;
    ext2_fs.section_read(1024, 510, &ext2_fs.super_blk);
    uint32_t test;
    ext2_fs.section_read(1024, 4, &test);
    putf_const("[|%x|][|%x|]", test, &test);
    puthex(test);
    ASSERT(ext2_fs.super_blk.s_magic == 0xEF53);
    ext2_super_block_t *super_blk = &ext2_fs.super_blk;

    super_blk->s_block_size = 2 << (super_blk->s_block_size + 10);
    super_blk->s_fragment_size = 2 << (super_blk->s_fragment_size + 10);
    ext2_fs.block_group_count = (super_blk->s_blocks_count - super_blk->s_first_data_block - 1) /
                                super_blk->s_blocks_per_group + 1;
    ext2_fs.block_group = kmalloc_paging(ext2_fs.block_group_count *
                                         sizeof(ext2_group_desc_t) & 0xFFFFF000
                                                                     + 0x1000, NULL);
    ext2_fs.section_read(1024 + sizeof(ext2_super_block_t), ext2_fs.block_group_count * sizeof(ext2_group_desc_t),
                         ext2_fs.block_group);
    putf_const("[+][%x][]\n", super_blk->s_block_size);
    putf_const("[+] [%x][%x][%x]\n\n\n\n", ext2_fs.block_group[0].bg_inode_bitmap,
               ext2_fs.block_group[0].bg_inode_table,
               super_blk->s_inodes_per_group - ext2_fs.block_group[0].bg_free_inodes_count);
    //uint8_t *buff = kmalloc_paging(0x5000);
    //ext2_fs.read(0, 0x5000, buff);
    uint32_t index = (2 - 1) % super_blk->s_inodes_per_group;
    uint32_t block = (index * sizeof(ext2_inode_t)) / super_blk->s_block_size;
    index = index % super_blk->s_inodes_per_group;
    uint32_t offset = block + ext2_fs.block_group[0].bg_inode_table;
    ext2_inode_t inode;
    memset(&inode, 0, sizeof(ext2_inode_t));
    //TODO:Interesting number...
    uint32_t off = ((super_blk->s_block_size / SECTION_SIZE) * (offset - 3)) * SECTION_SIZE;
    ext2_fs.section_read(off,
                         super_blk->s_block_size,
                         &inode);
    putf_const("[+]neko[%x][%x]\n", offset, off);
    putf_const("[+]cat[%x][%x]", inode.i_ctime, inode.i_flags);
    for (;;);
}