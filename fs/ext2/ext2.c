//
// Created by dcat on 8/17/17.
//
#include <str.h>
#include <ext2.h>
#include <superblk.h>
#include "ext2.h"
#include "ide.h"
#include "kmalloc.h"
#include "vfs.h"

ext2_t ext2_fs_test;
blk_dev_t dev;
fs_t ext2_fs;

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
    if (offset % SECTION_SIZE != 0 || len % SECTION_SIZE != 0) {
        PANIC("offset and len must be block align!");
    }
    uint32_t blk_no = (offset + 0x100000) / SECTION_SIZE;
    uint32_t blk_count = len / SECTION_SIZE;
    uint8_t err;
    for (uint32_t x = 0; x < blk_count; x++) {
        err = ide_ata_access(ATA_WRITE, 1, blk_no + x, 1, buff + x * SECTION_SIZE);
        ASSERT(err == 0);
    }
    return err;
}

int ext2_read_block(ext2_t *fs, uint32_t base, uint32_t count, uint8_t *buff) {
    uint32_t offset = base * fs->block_size;
    uint32_t size = count * fs->block_size;
    putf_const("[%x] <==> [%x]\n", offset, buff);
    return fs->dev->read(offset, size, buff);
}

int ext2_write_block(ext2_t *fs, uint32_t base, uint32_t count, uint8_t *buff) {
    uint32_t offset = base * fs->block_size;
    uint32_t size = count * fs->block_size;
    return fs->dev->write(offset, size, buff);
}

int ext2_get_inode_block(ext2_t *fs, uint32_t inode_id, uint32_t *index_out, uint32_t *block_no_out,
                         ext2_inode_t **inode_out, uint8_t *buff) {
    ASSERT(inode_id >= 2);
    uint32_t bg_no = (inode_id - 1) / fs->super_blk.s_inodes_per_group;
    uint32_t index = (inode_id - 1) % fs->super_blk.s_inodes_per_group;
    uint32_t block = (index * sizeof(ext2_inode_t)) / fs->block_size;
    uint32_t offset = block + fs->block_group[bg_no].bg_inode_table;
    if (fs->block_group[bg_no].bg_inode_table == 0) {
        TODO;//Alloc a table
    }
    ext2_read_block(fs, offset, 1, buff);
    index = index % (fs->block_size / sizeof(ext2_inode_t));
    if (index_out)
        *index_out = index;
    if (block_no_out)
        *block_no_out = offset;
    if (inode_out) {
        *inode_out = (ext2_inode_t *) ((uint32_t) buff + sizeof(ext2_inode_t) * index);
    }
    return 0;
}

int ext2_find_inote(ext2_t *ext2_fs, uint32_t inode_id, ext2_inode_t *inode_out) {
    ASSERT(inode_id >= 2);
    uint32_t bg_no = (inode_id - 1) / ext2_fs->super_blk.s_inodes_per_group;
    uint32_t index = (inode_id - 1) % ext2_fs->super_blk.s_inodes_per_group;
    uint32_t block = (index * sizeof(ext2_inode_t)) / ext2_fs->block_size;
    uint32_t offset = block + ext2_fs->block_group[bg_no].bg_inode_table;
    index = index % (ext2_fs->block_size / sizeof(ext2_inode_t));
    uint8_t blk[SECTION_SIZE];
    uint32_t off = ext2_fs->block_size * offset;
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
    putf_const("[+]neko[%x][%x]", inode_id, off + off_les);
    putf_const("[%x][%x]\n", off_les, inode);
    return 0;
}

void ext2_dir_iterator_init(ext2_t *fs, ext2_inode_t *inode, ext2_dir_iterator_t *iter) {
    iter->fs = fs;
    iter->inode = inode;
    iter->next_block_id = 0;
    iter->block = (uint8_t *) kmalloc_paging(fs->block_size, NULL);
    iter->cur_dir = 0;
    return;
}

int ext2_dir_iterator_next(ext2_dir_iterator_t *iter, ext2_dir_t **dir) {
    if (iter->cur_dir == 0 || iter->cur_dir->inode_id == 0) {
        //load next block
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
        (uint32_t) iter->cur_dir - (uint32_t) iter->block > iter->fs->block_size)
        return 0;
    return 1;
}

int ext2_dir_iterator_exit(ext2_dir_iterator_t *iter) {
    //kfree(iter->block);
}


int ext2_update_sblk_and_bg(ext2_t *fs) {
    //putf_const("[+] last mount:%s\n", fs->super_blk.s_last_mounted);
    strcpy(fs->super_blk.s_last_mounted, "$ROOTFS/");
    int err = fs->dev->write(EXT2_SUPER_BLK_OFFSET, sizeof(fs->super_blk), &fs->super_blk);
    if (err)return err;
    return fs->dev->write(EXT2_BLOCK_GROUP_OFFSET, sizeof(ext2_group_desc_t) * fs->block_group_count, fs->block_group);
}

int ext2_alloc_inodeid(ext2_t *fs, uint16_t type, uint32_t *new_inode_id) {
    uint8_t *bitmap = (uint8_t *) kmalloc_paging(fs->block_size, NULL);
    if (fs->super_blk.s_free_inodes_count <= 0) {
        puterr_const("[-] out of inode!");
        return -1;
    }
    for (uint32_t x = 0; x < fs->block_group_count; x++) {
        if (fs->block_group[x].bg_free_inodes_count > 0) {
            if (ext2_read_block(fs, fs->block_group[x].bg_inode_bitmap, 1, bitmap)) {
                PANIC("[-] fail to read block.")
            }
            for (uint32_t y = 0; y < fs->super_blk.s_inodes_per_group; y++) {
                if (BIT_GET(bitmap[y / 8], y % 8) == 0) {
                    BIT_SET(bitmap[y / 8], y % 8);
                    *new_inode_id = x * fs->super_blk.s_inodes_per_group + y + 1;//inode id begin at 1
                    putf_const("[+][%x] found empty inode:%x\n", bitmap, *new_inode_id);
                    fs->block_group[x].bg_free_inodes_count--;
                    fs->super_blk.s_free_inodes_count--;
                    if (IS_DIR(type))
                        fs->block_group[x].bg_dirs_count++;
                    ext2_write_block(fs, fs->block_group[x].bg_inode_bitmap, 1, bitmap);
                    ext2_update_sblk_and_bg(fs);
                    return 0;
                    break;
                }
            }

        }
    }
    PANIC("[-] Out of inode! but fs->super_blk.s_free_inodes_count>0");
    return 1;
}

int ext2_alloc_block(ext2_t *fs, uint32_t *new_block_id) {
    uint8_t *bitmap = (uint8_t *) kmalloc_paging(fs->block_size, NULL);
    if (fs->super_blk.s_free_blocks_count <= 0) {
        puterr_const("[-] out of block!");
        return -1;
    }
    for (uint32_t x = 0; x < fs->block_group_count; x++) {
        if (fs->block_group[x].bg_free_blocks_count > 0) {
            putf_const("[search:%x]", x);
            if (ext2_read_block(fs, fs->block_group[x].bg_block_bitmap, 1, bitmap)) {
                PANIC("[-] fail to read block.")
            }
            for (uint32_t y = 0; y < fs->super_blk.s_blocks_per_group; y++) {
                if (BIT_GET(bitmap[y / 8], y % 8) == 0) {
                    putf_const("[+][%x] found empty block:=%x\n", bitmap, y);
                    BIT_SET(bitmap[y / 8], y % 8);
                    *new_block_id = (uint32_t) fs->super_blk.s_blocks_per_group * x + y;
                    fs->block_group[x].bg_free_blocks_count--;
                    fs->super_blk.s_free_blocks_count--;
                    //Write back
                    ext2_write_block(fs, fs->block_group[x].bg_block_bitmap, 1, bitmap);
                    ext2_update_sblk_and_bg(fs);
                    return 0;
                    break;
                }
            }

        }
    }
    PANIC("[-] Out of block! but fs->super_blk.s_free_blocks_count>0");
    return 1;
}


int ext2_make(ext2_t *fs, uint16_t type, uint32_t father_id, char *name, uint32_t *new_id_out) {
    uint32_t ninode_id;
    if (ext2_alloc_inodeid(fs, type, &ninode_id)) PANIC("cannot alloc inodeid");
    if (new_id_out)
        *new_id_out = ninode_id;
    uint8_t *fblk = (uint8_t *) kmalloc_paging(fs->block_size, NULL);
    uint8_t *blk = (uint8_t *) kmalloc_paging(fs->block_size, NULL);
    uint32_t fblkno, findex;
    ext2_get_inode_block(fs, father_id, &findex, &fblkno, NULL, fblk);
    ext2_inode_t *father = (ext2_inode_t *) (fblk + findex * sizeof(ext2_inode_t));
    ASSERT(IS_DIR(father->i_mode));
    //Check whether file exists.
    ext2_dir_iterator_t iter;
    ext2_dir_t *dir;
    uint8_t name_len = (uint8_t) strlen(name);
    ext2_dir_iterator_init(fs, father, &iter);
    while (ext2_dir_iterator_next(&iter, &dir)) {
        //FIXME wrong algo.
        if (dir->name_length == name_len) {
            int x = 0;
            for (; x < name_len && dir->name[x] == name[x]; x++);
            if (x == name_len - 1) {
                putf_const("[-] %s already exists.\n", name);
                return 1;
            }
        }
    }
    ext2_dir_iterator_exit(&iter);
    //Update inode
    uint32_t index, blk_no;
    if (ext2_get_inode_block(fs, ninode_id, &index, &blk_no, NULL, blk)) PANIC("cannot get inode block.");
    ASSERT(index < fs->block_size / sizeof(ext2_inode_t));
    ext2_inode_t *inode = (ext2_inode_t *) ((uint32_t) blk + sizeof(ext2_inode_t) * index);
    //ASSERT(inode->i_mode == 0);
    putf_const("[-]blockNO[%x][OFF:%x]", blk_no, blk_no * fs->block_size + sizeof(ext2_inode_t) * index);
    memset(inode, 0, sizeof(ext2_inode_t));
    type &= 0xF000;
    inode->i_mode = (uint16_t) (type | 0x1FF);
    inode->i_ctime = 0xDDDD;
    inode->i_uid = 0xCCAA;
    inode->i_size = 0;
    inode->i_links_count = 1;
    inode->i_sectors_count = fs->sections_per_group;
    uint32_t bno;
    ASSERT(ext2_alloc_block(fs, &bno) == 0);
    ASSERT(bno < fs->super_blk.s_blocks_count);
    inode->i_block[0] = bno;
    if (type == INODE_TYPE_DIRECTORY) {
        inode->i_size = 1024;
    } else {
        inode->i_size = 3;
    }
    ext2_write_block(fs, blk_no, 1, blk);
    inode = NULL;
    if (type == INODE_TYPE_DIRECTORY) {
        //Create the default directories;
        ext2_read_block(fs, bno, 1, blk);
        ext2_dir_t *dd = (ext2_dir_t *) blk;
        dd->inode_id = ninode_id;
        dd->name[0] = '.';
        dd->name[1] = '\0';
        dd->name_length = 1;
        dd->type_indicator = INODE_DIR_TYPE_INDICATOR_DIRECTORY;
        dd->size = 12;
        dd = (ext2_dir_t *) ((uint32_t) dd + dd->size);
        dd->inode_id = father_id;
        dd->name[0] = '.';
        dd->name[1] = '.';
        dd->name[2] = '\0';
        dd->name_length = 2;
        dd->type_indicator = INODE_DIR_TYPE_INDICATOR_DIRECTORY;
        dd->size = (uint16_t) (fs->block_size - 12);
        ext2_write_block(fs, bno, 1, blk);
    } else {
        //Test
        ext2_read_block(fs, bno, 1, blk);
        memset(blk, 0xFF, fs->block_size);
        blk[0] = 'A';
        blk[1] = 'S';
        blk[2] = 'S';
        ext2_write_block(fs, bno, 1, blk);
    }
    //Update dir table.
    //ASSERT(father->i_block[father->i_blocks_count - 1]);
    ext2_read_block(fs, father->i_block[0], 1, blk);
    dir = (ext2_dir_t *) blk;
    uint8_t found = false;
    int x = 0;
    for (; x < EXT2_NDIR_BLOCKS && !found;) {
        while (true) {
            if ((uint32_t) dir >= (uint32_t) ((uint32_t) blk + fs->block_size)) {
                x++;
                if (father->i_block[x] == 0) {
                    TODO;
                    //Create a new block.
                    ext2_alloc_block(fs, &father->i_block[x]);
                    father->i_sectors_count += fs->sections_per_group;//Write back..
                    ext2_read_block(fs, father->i_block[x], 1, blk);
                    dir = (ext2_dir_t *) blk;
                    break;
                }
                ext2_read_block(fs, father->i_block[x], 1, blk);
                dir = (ext2_dir_t *) blk;
            }
            if (dir->inode_id == 0) {
                found = true;
                break;
            } else putf_const("[|%x]", dir->inode_id);
            uint32_t tsize = 8 + dir->name_length;
            if (tsize % 4 != 0)
                tsize += 4 - tsize % 4;
            if (dir->size != tsize) {
                //fix length
                uint32_t old_len = dir->size;
                dir->size = (uint16_t) (8 + dir->name_length);
                if (dir->size % 4 != 0)
                    dir->size += 4 - dir->size % 4;
                putf_const("[**] fix length:[%x] -->[%x]\n", old_len, dir->size);
                dir = (ext2_dir_t *) ((uint32_t) dir + dir->size);
                k_delay(5);
                found = true;
                break;
            }
            dir = (ext2_dir_t *) ((uint32_t) dir + dir->size);
        }
    }
    ASSERT(dir);
    dir->inode_id = ninode_id;
    dir->type_indicator = (uint8_t) (IS_DIR(type) ? INODE_DIR_TYPE_INDICATOR_DIRECTORY : INODE_DIR_TYPE_INDICATOR_FILE);
    dir->name_length = name_len;
    for (int y = 0; y < name_len; y++)
        dir->name[y] = name[y];
    dir->name[name_len] = 0;
    dir->size = (uint16_t) ((uint16_t) blk + fs->block_size - ((uint16_t) dir));
    ext2_write_block(fs, father->i_block[x], 1, blk);
    return 0;
}

int ext2_read_file_block_singly(ext2_t *fs, ext2_inode_t *inode, uint32_t block_no, uint8_t *buff) {
    uint32_t max_blocks = fs->block_size / sizeof(uint32_t);
    uint32_t *blk = (uint32_t *) kmalloc(fs->block_size);
    ext2_read_block(fs, inode->i_singly_block, 1, blk);
    uint32_t no = block_no - EXT2_SIND_BLOCK;
    ASSERT(no < max_blocks);
    putf_const("[%s] %x", __func__, blk[no]);
    if (blk[no] == 0)
        return 1;
    int err = ext2_read_block(fs, blk[no], 1, buff);
    //kfree(blk);
    return err;
}

int ext2_read_file_block_doubly(ext2_t *fs, ext2_inode_t *inode, uint32_t block_no, uint8_t *buff) {
    TODO;
}

/**
 * @return 0:succ */
int ext2_read_file_block(ext2_t *fs, ext2_inode_t *inode, uint32_t block_no, uint8_t *buff) {
    uint32_t max_blocks = fs->block_size / sizeof(uint32_t);
    putf_const("[+] max_blocks:%x read[%x]", max_blocks, block_no);
    if (block_no < EXT2_SIND_BLOCK) {
        putf_const("[%s] direct read[%x]:%x\n", __func__, block_no, inode->i_block[block_no]);
        return ext2_read_block(fs, inode->i_block[block_no], 1, buff);
    } else if (block_no < EXT2_SIND_BLOCK + max_blocks) {
        //singly Linked
        putf_const("[%s] singly direct read[%x]:%x\n", __func__, block_no, inode->i_block[block_no]);
        return ext2_read_file_block_singly(fs, inode, block_no, buff);
    } else
        TODO;
}

/**
 * @return read bytes*/
int ext2_read_file(ext2_t *fs, ext2_inode_t *inode, uint32_t offset, uint32_t size, uint8_t *buff) {
    uint32_t block_offset = offset / fs->block_size;
    uint32_t block_count = size / fs->block_size;
    for (int x = 0; x < block_count; x++) {
        int err = ext2_read_file_block(fs, inode, block_offset + x, buff + x * fs->block_size);
        if (err) {
            return -1;
        }
    }
    return size;//TODO
}

inline uint32_t ext2_get_block_section(ext2_t *fs, uint32_t blk_no, uint8_t section_no) {
    if (section_no * SECTION_SIZE > fs->super_blk.s_block_size)
        return 0;
    return blk_no * fs->super_blk.s_block_size + section_no * SECTION_SIZE;
}

int ext2_get_file_blockid(ext2_t *fs, ext2_inode_t *inode, uint32_t block_no, bool create, uint32_t *id_out) {
    if (block_no < EXT2_SIND_BLOCK)
        *id_out = inode->i_block[block_no];
    else if (block_no < EXT2_SIND_BLOCK + fs->addr_per_blocks) {
        if (inode->i_singly_block == 0) {
            if (!create)return 1;
            //Alloc block
            CHK(ext2_alloc_block(fs, &inode->i_singly_block), "fail to alloc new block");
            uint32_t *blkids = (uint32_t *) kmalloc_paging(fs->super_blk.s_block_size, NULL);
            uint32_t *blkids_p = blkids;
            if (blkids == 0)goto _err;
            memset((uint8_t *) blkids, 0, fs->super_blk.s_block_size);
            for (int x = 0; x <= block_no - EXT2_SIND_BLOCK; x++) {
                CHK(ext2_alloc_block(fs, blkids_p), "fail to alloc new child block:%x", x);
                blkids_p++;
            }
            CHK(ext2_write_block(fs, inode->i_singly_block, 1, (uint8_t *) blkids), "I/O");
            kfree(blkids);
            *id_out = blkids_p[block_no - EXT2_SIND_BLOCK];
            return 0;
        }

    } else
        TODO;
    _err:
    return -1;
}

int
ext2_write_file_block(ext2_t *fs, ext2_inode_t *inode, uint32_t block_no, uint8_t *buff) {
    uint32_t max_blocks = fs->block_size / sizeof(uint32_t);
    putf_const("[+] max_blocks:%x read[%x]", max_blocks, block_no);
    if (block_no < EXT2_SIND_BLOCK) {
        putf_const("[%s] direct write[%x]:%x\n", __func__, block_no, inode->i_block[block_no]);
        if (inode->i_block[block_no] == 0) {
            //Alloc block;
            if (ext2_alloc_block(fs, &inode->i_block[block_no])) {
                puterr_const("Cannot alloc new block.");
                return -1;
            }
        }

        return ext2_write_block(fs, inode->i_block[block_no], 1, buff);
    } else if (block_no < EXT2_SIND_BLOCK + max_blocks) {
        //singly Linked
        putf_const("[%s] singly direct write[%x]:%x\n", __func__, block_no, inode->i_block[block_no]);
        TODO;
    } else
        TODO;
}

/**
 * @return write bytes or -1 error*/
int ext2_write_file(ext2_t *fs, ext2_inode_t *inode, uint32_t offset, uint32_t size, uint8_t *buff) {
    uint32_t block_offset = offset / fs->block_size;
    uint32_t block_count = size / fs->block_size;
    uint8_t *tbuff = 0;
    uint32_t hblk_off = offset % fs->block_size;
    uint32_t fblk_size = size % fs->block_size;
    int x = 0;
    if (hblk_off || block_offset == 0) {
        tbuff = (uint8_t *) kmalloc_paging(sizeof(fs->super_blk.s_block_size), NULL);
        uint32_t tblkno = 0;
        ext2_get_file_blockid(fs, inode, block_offset, true, &tblkno);
        putf_const("[+]write hblk blkid:%x hblk_off:%x size:%x", tblkno, hblk_off,
                   MIN(fs->block_size - hblk_off, size - hblk_off));
        ext2_read_block(fs, tblkno, 1, tbuff);
        memcpy(&tbuff[hblk_off], buff, MIN(fs->block_size - hblk_off, size - hblk_off));
        ext2_write_block(fs, tblkno, 1, tbuff);
        x++;
    }
    for (; x < block_count; x++) {
        int err = ext2_write_file_block(fs, inode, block_offset + x, buff + x * fs->block_size);
        if (err) {
            return -1;
        }
        if (x == block_count - 1 && size % fblk_size != 0) {
            uint32_t tblkno = 0;
            ext2_get_file_blockid(fs, inode, block_offset + block_count, true, &tblkno);
            ext2_read_block(fs, tblkno, 1, tbuff);
            memcpy(tbuff, buff, MIN(fs->block_size, size - block_count * fs->super_blk.s_block_size));
            ext2_write_block(fs, tblkno, 1, tbuff);
        }
    }
    return 0;
}

void ext2_test() {
    //Test code...
    dev.read = ata_section_read;
    dev.write = ata_section_write;
    strcpy(dev.name, "DCAT DISK1");
    ext2_init(&ext2_fs_test, &dev);
    ext2_inode_t node, *sillynode;
    uint8_t *buff = kmalloc_paging(1024, NULL);
    uint32_t offset, blk_no;
    ext2_get_inode_block(&ext2_fs_test, 2, &offset, NULL, NULL, buff);
    memcpy(&node, buff + offset * sizeof(ext2_inode_t), sizeof(ext2_inode_t));
    //ext2_find_inote(&ext2_fs, 2, &node);
    ASSERT(IS_DIR(node.i_mode));
    uint32_t sillyid;
    ext2_make(&ext2_fs_test, INODE_TYPE_FILE, 2, "silly", &sillyid);
    ext2_make(&ext2_fs_test, INODE_TYPE_DIRECTORY, 2, "sill3y", NULL);

    ext2_get_inode_block(&ext2_fs_test, sillyid, NULL, &blk_no, &sillynode, buff);
    putf_const("[+]silly id:[%x][%x]\n", sillyid, sillynode);
    char sillybuff[25];
    strcpy(sillybuff, STR("DCAT'"));
    ASSERT(ext2_write_file(&ext2_fs_test, sillynode, 0, 5, sillybuff) == 0);
    sillynode->i_size = 5;
    ext2_write_block(&ext2_fs_test, blk_no, 1, buff);
    ext2_dir_iterator_t iter;
    ext2_dir_iterator_init(&ext2_fs_test, &node, &iter);
    ext2_dir_t *d;
    while (ext2_dir_iterator_next(&iter, &d)) {
        char name[256];
        memcpy(name, d->name, d->name_length);
        name[d->name_length] = '\0';
        if (name[0] != 's')continue;
        ext2_inode_t inode;
        ext2_find_inote(&ext2_fs_test, d->inode_id, &inode);
        putf_const("[+] [%x]fn:%s file_size:%x ", d->inode_id, name, inode.i_size);
        putf_const("uid:%x es:%x next:%x\n", inode.i_uid, d->size, ((uint32_t) iter.cur_dir) + iter.cur_dir->size);
        /*if (name[0] == 'b') {
            putf_const("[+] try to read file\n");
            uint8_t *buff = (uint8_t *) kmalloc_paging(0x1000, NULL);
            putf_const("[+]buff addr:%x\n", buff);
            ext2_read_file(&ext2_fs_test, &inode, 0x3000, 1024, buff);
            buff[0xFFF] = '\0';
            puts(buff);
            //putf_const("read data:%s\n", buff);
        }*/
    }
    ext2_dir_iterator_exit(&iter);
    putf_const("all done![%x]", sizeof(ext2_inode_t));
    for (;;);
    putf_const("first block addr:[%x][%x]", node.i_block[0], node.i_block[0] * ext2_fs_test.block_size / 2);
    ext2_dir_t *dir = kmalloc(ext2_fs_test.block_size);
    ext2_read_block(&ext2_fs_test, node.i_block[0], 1, dir);
    while (dir->inode_id != 0) {
        char name[256];
        memcpy(name, dir->name, dir->name_length);
        name[dir->name_length] = '\0';
        putf_const("[+] [%x]fn:%s esize:%x\n", dir, name, dir->size);
        dir = ((uint32_t) dir) + dir->size;
    }

    for (;;);
}

int ext2_init(ext2_t *fs_out, blk_dev_t *dev) {
    putf_const("[ext2_init]callin\n");
    memset(fs_out, 0, sizeof(ext2_t));
    fs_out->magic = EXT2_MAGIC;
    fs_out->dev = dev;
    fs_out->dev->read(0x400, sizeof(ext2_super_block_t), (uint8_t *) &fs_out->super_blk);
    ASSERT(fs_out->super_blk.s_magic == EXT2_MAGIC);
    ext2_super_block_t *super_blk = &fs_out->super_blk;
    fs_out->block_size = (uint32_t) (1 << (super_blk->s_block_size + 10));//log2(x)-10
    fs_out->fragment_size = (uint32_t) (1 << (super_blk->s_fragment_size + 10));
    fs_out->sections_per_group = fs_out->block_size / SECTION_SIZE;
    fs_out->addr_per_blocks = super_blk->s_block_size / sizeof(uint32_t);
    ASSERT(fs_out->sections_per_group > 0);
    fs_out->block_group_count = (super_blk->s_blocks_count - super_blk->s_first_data_block - 1) /
                                super_blk->s_blocks_per_group + 1;
    fs_out->block_group = (ext2_group_desc_t *) kmalloc_paging(fs_out->block_group_count *
                                                               sizeof(ext2_group_desc_t), NULL);
    fs_out->dev->read(0x800, fs_out->block_group_count * sizeof(ext2_group_desc_t),
                      fs_out->block_group);
    return 0;
}

int32_t ext2_fs_node_write(fs_node_t *node, uint32_t offset, uint32_t len, uint8_t *buff) {
    return -1;
}

int32_t ext2_fs_node_read(fs_node_t *node, uint32_t offset, uint32_t len, uint8_t *buff) {
    return -1;
}

int ext2_get_fs_node(ext2_t *fs, uint32_t inode_id, const char *fn, fs_node_t *node) {
    strcpy(node->name, fn);
    ext2_inode_t inode;
    ext2_find_inote(fs, inode_id, &inode);
    node->mask = inode.i_mode;
    node->uid = inode.i_uid;
    node->gid = inode.i_gid;
    if (IS_DIR(inode.i_mode)) {
        node->flags = FS_DIRECTORY;
    } else if (IS_FILE(inode.i_mode)) {
        node->flags = FS_FILE;
    } else node->flags = 0;//?
    node->length = inode.i_size;
    node->fsp = fs;
    node->inode = inode_id;
    return 0;
}

int32_t ext2_fs_node_readdir(fs_node_t *node, __fs_special_t *fsp, uint32_t count, dirent_t *result) {
    ext2_t *fs = fsp;
    ASSERT(fs->magic == EXT2_MAGIC);
    ext2_inode_t inode;
    CHK(ext2_find_inote(fs, node->inode, &inode), "inode not exist");
    ext2_dir_iterator_t iter;
    ext2_dir_t *dir;
    ext2_inode_t cinode;
    fs_node_t tmp_node;
    int x = 0;
    ext2_dir_iterator_init(fs, &inode, &iter);
    while (ext2_dir_iterator_next(&iter, &dir)) {
        ASSERT(dir->name_length < 256);
        memcpy(result[x].name, dir->name, dir->name_length);
        result[x].name[dir->name_length] = '\0';
        result[x].node = dir->inode_id;
        result[x].name_len = dir->name_length;
        if (dir->type_indicator == INODE_DIR_TYPE_INDICATOR_DIRECTORY)
            result[x].type = FS_DIRECTORY;
        else if (dir->type_indicator = INODE_DIR_TYPE_INDICATOR_FILE)
            result[x].type = FS_FILE;
        else PANIC("Unknown file type:%x", dir->type_indicator);
        //CHK(ext2_find_inote(fs, dir->inode_id, &cinode), "");
        //ext2_get_fs_node(fs, &cinode, result[x].name, &result[x].node);
        if (x >= count)break;
        x++;
    }
    ext2_dir_iterator_exit(&iter);
    return x;
    _err:
    return -1;
}

int ext2_fs_node_finddir(fs_node_t *node, __fs_special_t *fsp, char *name, fs_node_t *result) {
    ext2_t *fs = fsp;
    char sname[256];
    bool found = false;
    ASSERT(fs->magic == EXT2_MAGIC);
    ext2_inode_t inode;
    CHK(ext2_find_inote(fs, node->inode, &inode), "inode not exist");
    ext2_dir_iterator_t iter;
    ext2_dir_t *dir;
    ext2_inode_t cinode;
    int x = 0;
    ext2_dir_iterator_init(fs, &inode, &iter);
    while (ext2_dir_iterator_next(&iter, &dir)) {
        ASSERT(dir->name_length < 256);
        memcpy(sname, dir->name, dir->name_length);
        sname[dir->name_length] = '\0';
        //putf_const("[ext2_fs_node_finddir]fn:%s[%x]\n", sname, (uint32_t) strstr(sname, name));
        if (strstr(sname, name) == 0) {
            found = true;
            ext2_get_fs_node(fs, dir->inode_id, sname, result);
            break;
        }
        x++;
    }
    ext2_dir_iterator_exit(&iter);
    if (found)
        return 0;
    else
        return 1;
    _err:
    return -1;
}

int ext2_fs_node_get_node(__fs_special_t *fsp, uint32_t inode_id, fs_node_t *node) {
    ext2_t *fs = fsp;
    char sname[256];
    bool found = false;
    ASSERT(fs->magic == EXT2_MAGIC);
    return ext2_find_inote(fs, inode_id, node);
}

int ext2_fs_node_make(fs_node_t *node, __fs_special_t *fsp, uint8_t type, char *name, fs_node_t *new_node_out) {
    ext2_t *fs = fsp;
    uint32_t inode_id = node->inode;
    int e2type;
    switch (type) {
        case FS_DIRECTORY:
            e2type = INODE_TYPE_DIRECTORY;
            break;
        case FS_FILE:
            e2type = INODE_DIR_TYPE_INDICATOR_FILE;
            break;
        default: putf_const("[ext2_fs_node_make]unknown type:%x\n", type);
            return -1;
    }
    uint32_t new_nnid;
    CHK(ext2_make(fs, e2type, inode_id, name, &new_nnid), "make failed.");
    ext2_get_fs_node(fs, new_nnid, name, new_node_out);
    return 0;
    _err:
    return 1;
}

__fs_special_t *ext2_fs_node_mount(blk_dev_t *dev, fs_node_t *rootnode) {
    ext2_t *fs = kmalloc(sizeof(ext2_t));
    CHK(ext2_init(fs, dev), "[ext2]fail to mount");
    rootnode->inode = 2;
    rootnode->fsp = fs;
    return fs;
    _err:
    return 0;
}

void ext2_create_fstype() {
    strcpy(ext2_fs.name, STR("EXT2_TEST"));
    ext2_fs.mount = ext2_fs_node_mount;
    ext2_fs.read = ext2_fs_node_read;
    ext2_fs.write = ext2_fs_node_write;
    ext2_fs.readdir = ext2_fs_node_readdir;
    ext2_fs.finddir = ext2_fs_node_finddir;
    ext2_fs.getnode = ext2_fs_node_get_node;
    ext2_fs.make = ext2_fs_node_make;
    //Test
    dev.read = ata_section_read;
    dev.write = ata_section_write;
}