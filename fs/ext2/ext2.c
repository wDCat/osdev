//
// Created by dcat on 8/17/17.
//
#include <str.h>
#include <ext2.h>
#include <superblk.h>
#include "ext2.h"
#include "ide.h"
#include "kmalloc.h"

ext2_t ext2_fs;
blk_dev_t dev;

int ata_section_read(uint32_t offset, uint32_t len, uint8_t *buff) {
    if (offset % SECTION_SIZE != 0) {
        PANIC("offset and len must be block align!");
    }
    uint32_t blk_no = (offset + 0x100000) / SECTION_SIZE;
    uint32_t blk_count = len / SECTION_SIZE;
    uint8_t err;
    uint8_t tbuff[SECTION_SIZE];
    for (uint32_t x = 0; x < blk_count; x++) {
        err = ide_ata_access(ATA_READ, 1, blk_no + x, 1, buff + x * SECTION_SIZE);
        ASSERT(err == 0);
    }
    if (len % SECTION_SIZE != 0) {
        err = ide_ata_access(ATA_READ, 1, blk_no + blk_count, 1, tbuff);
        ASSERT(err == 0);
        putf_const("[+] copy %x to %x size %x", buff + (blk_count) * SECTION_SIZE, buff, len % SECTION_SIZE);
        memcpy(buff + (blk_count) * SECTION_SIZE, tbuff, len % SECTION_SIZE);
        //for (;;);
    }
    return err;
}

int ata_section_write(uint32_t offset, uint32_t len, uint8_t *buff) {
    return 1;
}

int ext2_read_block(ext2_t *fs, uint32_t base, uint32_t count, uint8_t *buff) {
    uint32_t offset = base * fs->super_blk.s_block_size;
    uint32_t size = count * fs->super_blk.s_block_size;
    return fs->dev->read(offset, size, buff);
}

int ext2_find_inote(ext2_t *ext2_fs, uint32_t inode_id, ext2_inode_t *inode_out) {
    ASSERT(inode_id >= 2);
    uint32_t bg_no = (inode_id - 1) / ext2_fs->super_blk.s_inodes_per_group;
    uint32_t index = (inode_id - 1) % ext2_fs->super_blk.s_inodes_per_group;
    uint32_t block = (index * sizeof(ext2_inode_t)) / ext2_fs->super_blk.s_block_size;
    uint32_t offset = block / 2 + ext2_fs->block_group[bg_no].bg_inode_table;
    uint8_t blk[SECTION_SIZE];
    uint32_t off = ext2_fs->super_blk.s_block_size * offset;
    ext2_inode_t *inode;
    //计算inode在哪个section...
    uint32_t off_2 = index * sizeof(ext2_inode_t);
    uint32_t off_sec = off_2 / SECTION_SIZE;
    uint32_t off_les = off_2 % SECTION_SIZE;
    off += off_sec * SECTION_SIZE;
    if (ext2_fs->dev->read(off, SECTION_SIZE, blk)) {
        return -1;
    }
    inode = (ext2_inode_t *) (blk + off_les);
    memcpy(inode_out, inode, sizeof(ext2_inode_t));
    putf_const("[+]neko[%x][%x]", inode_id, offset);
    putf_const("[%x][%x]\n", off, inode);
    return 0;
}

void ext2_dir_iterator_init(ext2_t *fs, ext2_inode_t *inode, ext2_dir_iterator_t *iter) {
    iter->fs = fs;
    iter->inode = inode;
    iter->next_block_id = 0;
    iter->block = (uint8_t *) kmalloc_paging(fs->super_blk.s_block_size, NULL);
    iter->cur_dir = 0;
    return;
}

int ext2_dir_iterator_next(ext2_dir_iterator_t *iter, ext2_dir_t **dir) {
    if (iter->cur_dir == 0 || iter->cur_dir->inode_id == 0) {
        //load next block
        if (iter->next_block_id >= iter->inode->i_blocks_count)//End
            return 0;
        putf_const("[+] switch block:%x[%x]\n", iter->next_block_id, iter->block);
        ext2_read_block(iter->fs, iter->inode->i_block[iter->next_block_id], 1, iter->block);
        iter->next_block_id++;
        iter->cur_dir = (ext2_dir_t *) iter->block;
        if (iter->cur_dir->inode_id == 0)
            return 0;
        *dir = iter->cur_dir;
        return 1;
    }
    iter->cur_dir = (ext2_dir_t *) ((uint32_t) iter->cur_dir + iter->cur_dir->size);
    *dir = iter->cur_dir;
    if (iter->cur_dir->inode_id == 0 ||
        (uint32_t) iter->cur_dir - (uint32_t) iter->block > iter->fs->super_blk.s_block_size)
        return 0;
    return 1;
}

int ext2_read_file_block(ext2_t *fs, ext2_inode_t *inode, uint32_t block_no, uint8_t *buff) {
    if (block_no < EXT2_SIND_BLOCK) {
        putf_const("[%s] read[%x]:%x\n", __func__, block_no, inode->i_block[block_no]);
        return ext2_read_block(fs, inode->i_block[block_no], 1, buff);
    } else PANIC("//TODO:")
}

int ext2_read_file(ext2_t *fs, ext2_inode_t *inode, uint32_t offset, uint32_t size, uint8_t *buff) {
    uint32_t block_offset = offset / fs->super_blk.s_block_size;
    uint32_t block_count = size / fs->super_blk.s_block_size;
    for (int x = 0; x < block_count; x++) {
        int err = ext2_read_file_block(fs, inode, block_offset + x, buff + x * fs->super_blk.s_block_size);

    }
}

void ext2_test() {
    //Test code...
    dev.read = ata_section_read;
    dev.write = ata_section_write;
    ext2_init(&dev);
    strcpy(dev.name, "DCAT DISK1");
    ext2_inode_t node;
    ext2_find_inote(&ext2_fs, 2, &node);
    ASSERT(IS_DIR(node.i_mode));
    ext2_dir_iterator_t iter;
    ext2_dir_iterator_init(&ext2_fs, &node, &iter);
    ext2_dir_t *d;
    while (ext2_dir_iterator_next(&iter, &d)) {
        char name[256];
        memcpy(name, d->name, d->name_length);
        name[d->name_length] = '\0';
        ext2_inode_t inode;
        ext2_find_inote(&ext2_fs, d->inode_id, &inode);

        putf_const("[+] [%x]fn:%s file_size:%x ", d->inode_id, name, inode.i_size);
        putf_const("uid:%x es:%x next:%x\n", inode.i_uid, d->size, ((uint32_t) iter.cur_dir) + iter.cur_dir->size);
        if (name[0] == 'T') {
            putf_const("[+] try to read file\n");
            uint8_t *buff = (uint8_t *) kmalloc_paging(0x1000, NULL);
            ext2_read_file(&ext2_fs, &inode, 0, 1024, buff);
            buff[20] = '\0';
            putf_const("read data:%s\n", buff);
        }
    }

    putf_const("all done![%x]", sizeof(ext2_inode_t));
    for (;;);
    putf_const("[+]block count [%x]", node.i_blocks_count);
    putf_const("first block addr:[%x][%x]", node.i_block[0], node.i_block[0] * ext2_fs.super_blk.s_block_size / 2);
    ext2_dir_t *dir = kmalloc(ext2_fs.super_blk.s_block_size);
    ext2_read_block(&ext2_fs, node.i_block[0], 1, dir);
    while (dir->inode_id != 0) {
        char name[256];
        memcpy(name, dir->name, dir->name_length);
        name[dir->name_length] = '\0';
        putf_const("[+] [%x]fn:%s esize:%x\n", dir, name, dir->size);
        dir = ((uint32_t) dir) + dir->size;
    }

    for (;;);
}

void ext2_init(blk_dev_t *dev) {
    memset(&ext2_fs, 0, sizeof(ext2_t));
    ext2_fs.dev = dev;
    ext2_fs.dev->read(0x400, 0x200, &ext2_fs.super_blk);
    ASSERT(ext2_fs.super_blk.s_magic == EXT2_MAGIC);
    ext2_super_block_t *super_blk = &ext2_fs.super_blk;
    super_blk->s_block_size = (uint32_t) (1 << (super_blk->s_block_size + 10));//log2(x)-10
    super_blk->s_fragment_size = 1 << (super_blk->s_fragment_size + 10);
    ext2_fs.block_group_count = (super_blk->s_blocks_count - super_blk->s_first_data_block - 1) /
                                super_blk->s_blocks_per_group + 1;
    ext2_fs.block_group = kmalloc_paging(ext2_fs.block_group_count *
                                         sizeof(ext2_group_desc_t), NULL);
    ext2_fs.dev->read(0x800, ext2_fs.block_group_count * sizeof(ext2_group_desc_t),
                      ext2_fs.block_group);
}