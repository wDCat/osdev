//
// Created by dcat on 8/17/17.
//
#include <str.h>
#include <ext2.h>
#include <superblk.h>
#include "ext2.h"
#include "ide.h"
#include "../../memory/include/kmalloc.h"
#include "vfs.h"

ext2_t ext2_fs_test;
blk_dev_t dev;
fs_t ext2_fs;


int ext2_read_block(ext2_t *fs, uint32_t base, uint32_t count, uint8_t *buff) {
    uint32_t offset = base * fs->block_size;
    uint32_t size = count * fs->block_size;
    dprintf("[%x:%x] ==> [%x]", base, offset, buff);
    return fs->dev->read(offset, size, buff);
}

int ext2_write_block(ext2_t *fs, uint32_t base, uint32_t count, uint8_t *buff) {
    uint32_t offset = base * fs->block_size;
    uint32_t size = count * fs->block_size;
    dprintf("[%x:%x] <== [%x]", base, offset, buff);
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
    dprintf("inode[%x][%x][%x][%x]", inode_id, off + off_les, off_les, inode);
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
        dprintf("switch block:%x/%x [%x][%x]", iter->next_block_id, EXT2_NDIR_BLOCKS,
                iter->inode->i_block[iter->next_block_id], iter->block);
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
    dprintf("free blk:%x", iter->block);
    kfree(iter->block);
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
        deprintf("[-] out of inode!");
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
                    dprintf("[+][%x] found empty inode:%x", bitmap, *new_inode_id);
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

int ext2_alloc_block(ext2_t *fs, uint32_t *new_block_id, bool empty) {
    uint8_t *bitmap = (uint8_t *) kmalloc_paging(fs->block_size, NULL);
    if (fs->super_blk.s_free_blocks_count <= 0) {
        deprintf("[-] out of block!");
        return -1;
    }
    for (uint32_t x = 0; x < fs->block_group_count; x++) {
        if (fs->block_group[x].bg_free_blocks_count > 0) {
            dprintf("[search:%x]", x);
            if (ext2_read_block(fs, fs->block_group[x].bg_block_bitmap, 1, bitmap)) {
                PANIC("[-] fail to read block.")
            }
            for (uint32_t y = 0; y < fs->super_blk.s_blocks_per_group; y++) {
                if (BIT_GET(bitmap[y / 8], y % 8) == 0) {
                    dprintf("[+][%x] found empty block:=%x", bitmap, y);
                    BIT_SET(bitmap[y / 8], y % 8);
                    *new_block_id = (uint32_t) fs->super_blk.s_blocks_per_group * x + y;
                    fs->block_group[x].bg_free_blocks_count--;
                    fs->super_blk.s_free_blocks_count--;
                    //Write back
                    ext2_write_block(fs, fs->block_group[x].bg_block_bitmap, 1, bitmap);
                    ext2_update_sblk_and_bg(fs);
                    if (empty) {
                        dprintf("clear blk:%x", *new_block_id);
                        memset(bitmap, 0, fs->block_size);
                        ext2_write_block(fs, *new_block_id, 1, bitmap);
                    }
                    return 0;
                    break;
                }
            }

        }
    }
    PANIC("[-] Out of block! but fs->super_blk.s_free_blocks_count>0");
    kfree(bitmap);
    return 1;
}

int ext2_make(ext2_t *fs, uint16_t type, uint32_t father_id, char *name, uint32_t *new_id_out) {
    uint32_t ninode_id;
    CHK (ext2_alloc_inodeid(fs, type, &ninode_id), "out of inodeid?");
    if (new_id_out)
        *new_id_out = ninode_id;
    uint8_t *fblk = (uint8_t *) kmalloc_paging(fs->block_size, NULL);
    uint8_t *blk = (uint8_t *) kmalloc_paging(fs->block_size, NULL);
    uint32_t fblkno, findex;
    CHK(ext2_get_inode_block(fs, father_id, &findex, &fblkno, NULL, fblk), "cannot get father inode");
    ext2_inode_t *father = (ext2_inode_t *) (fblk + findex * sizeof(ext2_inode_t));
    ASSERT(IS_DIR(father->i_mode));
    //Check whether file exists.
    ext2_dir_iterator_t iter;
    ext2_dir_t *dir;
    uint8_t name_len = (uint8_t) strlen(name);
    ext2_dir_iterator_init(fs, father, &iter);
    while (ext2_dir_iterator_next(&iter, &dir)) {
        char *np = dir->name;
        if (dir->name_length == name_len) {
            if (!memcmp(np, name, name_len)) {
                dprintf("[-] %s already exists.", name);
                return 1;
            }
        }
    }
    ext2_dir_iterator_exit(&iter);
    //Update inode
    uint32_t index, blk_no;
    CHK(ext2_get_inode_block(fs, ninode_id, &index, &blk_no, NULL, blk), "cannot get child inode");
    ASSERT(index < fs->block_size / sizeof(ext2_inode_t));
    ext2_inode_t *inode = (ext2_inode_t *) ((uint32_t) blk + sizeof(ext2_inode_t) * index);
    //ASSERT(inode->i_mode == 0);
    dprintf("[-]blockNO[%x][OFF:%x]", blk_no, blk_no * fs->block_size + sizeof(ext2_inode_t) * index);
    memset(inode, 0, sizeof(ext2_inode_t));
    type &= 0xF000;
    inode->i_mode = (uint16_t) (type | 0x1FF);
    inode->i_ctime = 0xDDDD;
    inode->i_uid = 0xCCAA;
    inode->i_size = 0;
    inode->i_links_count = 1;
    inode->i_sectors_count = fs->sections_per_group;
    uint32_t bno;
    ASSERT(ext2_alloc_block(fs, &bno, true) == 0);
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
    }
    //Update dir table.
    ext2_read_block(fs, father->i_block[0], 1, blk);
    dir = (ext2_dir_t *) blk;
    uint8_t found = false;
    int x = 0;
    uint32_t offset = 0;
    for (; x < EXT2_NDIR_BLOCKS && !found;) {
        while (true) {
            if ((uint32_t) dir >= (uint32_t) ((uint32_t) blk + fs->block_size)) {
                x++;
                if (father->i_block[x] == 0) {
                    TODO;
                    //Create a new block.
                    ext2_alloc_block(fs, &father->i_block[x], false);
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
            } else
                dprintf("[|%x]", dir->inode_id);
            uint32_t tsize = 8 + dir->name_length;
            if (tsize % 4 != 0)
                tsize += 4 - tsize % 4;
            dprintf("[dir]nlen:%x size:%x tsize:%x", dir->name_length, dir->size, tsize);
            if (dir->size != tsize && offset + dir->size >= fs->block_size) {
                //jump to ignore deleted dir?
                /*Offset   0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F
                01140800   F1 0F 00 00 0C 00 01 02  2E 00 00 00 02 00 00 00   ñ       .
                01140810   24 00 02 02 2E 2E 00 00  F7 0F 00 00 18 00 0F 02   $   ..  ÷
                01140820   75 6E 74 69 74 6C 65 64  20 66 6F 6C 64 65 72 00   untitled folder
                01140830   F2 0F 00 00 0C 00 02 02  73 31 00 00 F3 0F 00 00   ò       s1  ó
                01140840   0C 00 02 01 63 31 00 00  F4 0F 00 00 0C 00 02 01       c1  ô
                01140850   63 32 00 00 F5 0F 00 00  0C 00 02 01 63 33 00 00   c2  õ       c3
                01140860   F6 0F 00 00 0C 00 02 01  63 34 00 00 F7 0F 00 00   ö       c4  ÷
                01140870   94 03 02 02 73 32 00 00  00 00 00 00 00 00 00 00   ”   s2
                 */
                //fix length
                uint32_t old_len = dir->size;
                dir->size = tsize;
                dprintf("[**] fix length:[%x] -->[%x]", old_len, dir->size);
                dir = (ext2_dir_t *) ((uint32_t) dir + dir->size);
                found = true;
                break;
            }
            dir = (ext2_dir_t *) ((uint32_t) dir + dir->size);
            offset += dir->size;
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
    kfree(blk);
    kfree(fblk);
    dprintf("make done");
    return 0;
    _err:
    kfree(blk);
    kfree(fblk);
    return 1;

}

int ext2_read_file_block_singly(ext2_t *fs, ext2_inode_t *inode, uint32_t block_no, bool create, uint8_t *buff) {
    uint32_t max_blocks = fs->block_size / sizeof(uint32_t);
    uint32_t *blk = (uint32_t *) kmalloc_paging(fs->block_size, NULL);
    ext2_read_block(fs, inode->i_singly_block, 1, blk);
    uint32_t no = block_no - EXT2_SIND_BLOCK;
    ASSERT(no < max_blocks);
    dprintf("[%s] %x", __func__, blk[no]);
    if (blk[no] == 0)
        if (create)TODO;
        else return -1;
    int err = ext2_read_block(fs, blk[no], 1, buff);
    kfree(blk);
    return err;
}

int ext2_read_file_block_doubly(ext2_t *fs, ext2_inode_t *inode, uint32_t block_no, bool create, uint8_t *buff) {
    TODO;
}

/**
 * @return 0:succ */
int ext2_read_file_block(ext2_t *fs, ext2_inode_t *inode, uint32_t block_no, bool create, uint8_t *buff) {
    uint32_t max_blocks = fs->block_size / sizeof(uint32_t);
    dprintf("max_blocks:%x read[%x]", max_blocks, block_no);
    if (block_no < EXT2_SIND_BLOCK) {
        dprintf("direct read[%x]:%x", block_no, inode->i_block[block_no]);
        if (inode->i_block[block_no] == 0) {
            if (!create)return 1;
            if (ext2_alloc_block(fs, &inode->i_block[block_no], true)) {
                deprintf("cannot alloc blk");
                return 1;
            }
        }
        return ext2_read_block(fs, inode->i_block[block_no], 1, buff);
    } else if (block_no < EXT2_SIND_BLOCK + max_blocks) {
        //singly Linked
        dprintf("singly direct read[%x]:%x", block_no, inode->i_block[block_no]);
        return ext2_read_file_block_singly(fs, inode, block_no, create, buff);
    } else
        TODO;
}

/**
 * @return read bytes*/
int ext2_read_file(ext2_t *fs, ext2_inode_t *inode, uint32_t offset, uint32_t size, uint8_t *buff) {
    if (offset > inode->i_size)return 0;
    if (offset + size > inode->i_size) {
        dprintf("fix size:%x --> %x", size, inode->i_size - offset);
        size = inode->i_size - offset;
    }
    uint32_t block_size = fs->block_size;
    uint32_t block_offset = offset / block_size;
    uint32_t block_count = size / block_size;
    uint8_t *tbuff = 0;
    uint32_t hblk_off = offset % block_size;
    uint32_t off = 0, les = size;
    dprintf("boff:%x bcou:%x", block_offset, block_count);
    uint32_t x = block_offset;
    if (hblk_off) {
        if (!tbuff)
            tbuff = (uint8_t *) kmalloc_paging(block_size, NULL);
        CHK(ext2_read_file_block(fs, inode, x, true, tbuff), "");
        uint32_t s = MIN(les, block_size - hblk_off);
        dprintf("head dblk no:%x offset:%x size:%x", x, hblk_off, s);
        memcpy(&buff[off], &tbuff[hblk_off], s);
        les -= s;
        off += s;
        x++;
    }
    if (block_count > 0)
        for (; x < block_offset + block_count; x++) {
            CHK(ext2_read_file_block(fs, inode, x, false, &buff[off]), "");
            les -= block_size;
            off += block_size;
        }
    if (les) {
        if (!tbuff)
            tbuff = (uint8_t *) kmalloc_paging(block_size, NULL);
        CHK(ext2_read_file_block(fs, inode, x, false, tbuff), "");
        dprintf("footer dblk no:%x size:%x", x, les);
        memcpy(&buff[off], tbuff, les);
        off += les;

    }
    _err:
    if (tbuff) {
        kfree(tbuff);
    }
    return off;
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
            CHK(ext2_alloc_block(fs, &inode->i_singly_block, true), "fail to alloc new block");
            uint32_t *blkids = (uint32_t *) kmalloc_paging(fs->super_blk.s_block_size, NULL);
            uint32_t *blkids_p = blkids;
            if (blkids == 0)goto _err;
            memset((uint8_t *) blkids, 0, fs->super_blk.s_block_size);
            for (int x = 0; x <= block_no - EXT2_SIND_BLOCK; x++) {
                CHK(ext2_alloc_block(fs, blkids_p, true), "fail to alloc new child block:%x", x);
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

int ext2_write_file_block(ext2_t *fs, ext2_inode_t *inode, uint32_t block_no, uint8_t *buff) {
    uint32_t max_blocks = fs->block_size / sizeof(uint32_t);
    dprintf("max_blocks:%x read[%x]", max_blocks, block_no);
    if (block_no < EXT2_SIND_BLOCK) {
        dprintf("direct write[%x]:%x", block_no, inode->i_block[block_no]);
        if (inode->i_block[block_no] == 0) {
            //Alloc block;
            if (block_no == 0) PANIC("blkno[0]==0");
            if (ext2_alloc_block(fs, &inode->i_block[block_no], true)) {
                deprintf("Cannot alloc new block.");
                return -1;
            }
        }

        return ext2_write_block(fs, inode->i_block[block_no], 1, buff);
    } else if (block_no < EXT2_SIND_BLOCK + max_blocks) {
        //singly Linked
        dprintf("singly direct write[%x]:%x", block_no, inode->i_block[block_no]);
        TODO;
    } else
        TODO;
}

int ext2_update_inode(ext2_t *fs, uint32_t inode_id, ext2_inode_t *new_inode) {

    uint8_t *blk = (uint8_t *) kmalloc_paging(fs->block_size, NULL);
    uint32_t fblkno, findex;
    CHK(ext2_get_inode_block(fs, inode_id, &findex, &fblkno, NULL, blk), "cannot get inode");
    ext2_inode_t *inode = (ext2_inode_t *) (blk + findex * sizeof(ext2_inode_t));
    memcpy(inode, new_inode, sizeof(ext2_inode_t));
    dprintf("saving inode %x", inode_id);
    CHK(ext2_write_block(fs, fblkno, 1, blk), "");
    return 0;
    _err:
    return 1;
}

/**
 * @return write bytes or -1 error*/
int32_t ext2_write_file(ext2_t *fs, ext2_inode_t *inode, uint32_t offset, uint32_t size, uint8_t *buff) {
    uint32_t block_size = fs->block_size;
    uint32_t block_offset = offset / block_size;
    uint32_t block_count = size / block_size;
    uint8_t *tbuff = 0;
    uint32_t hblk_off = offset % block_size;
    uint32_t off = 0, les = size;
    dprintf("boff:%x bcou:%x", block_offset, block_count);
    uint32_t x = block_offset;
    if (hblk_off) {
        if (!tbuff)
            tbuff = (uint8_t *) kmalloc_paging(block_size, NULL);
        CHK(ext2_read_file_block(fs, inode, x, true, tbuff), "");
        uint32_t s = MIN(les, block_size - hblk_off);
        dprintf("head dblk no:%x offset:%x size:%x", x, hblk_off, s);
        memcpy(&tbuff[hblk_off], &buff[off], s);
        CHK(ext2_write_file_block(fs, inode, x, tbuff), "");
        les -= s;
        off += s;
        x++;
    }
    if (block_count > 0)
        for (; x < block_offset + block_count; x++) {
            CHK(ext2_write_file_block(fs, inode, x, &buff[off]), "");
            les -= block_offset;
            off += block_size;
        }
    if (les) {
        if (!tbuff)
            tbuff = (uint8_t *) kmalloc_paging(block_size, NULL);
        CHK(ext2_read_file_block(fs, inode, x, true, tbuff), "");
        dprintf("footer dblk no:%x size:%x", x, les);
        memcpy(tbuff, &buff[off], les);
        CHK(ext2_write_file_block(fs, inode, x, tbuff), "");
        off += les;

    }
    _err:
    if (tbuff) {
        kfree(tbuff);
    }
    return off;

}

void ext2_test() {
    //Test code...
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
    dprintf("callin");
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

int32_t ext2_fs_node_write(fs_node_t *node, __fs_special_t *fsp, uint32_t offset, uint32_t len, uint8_t *buff) {
    ext2_t *fs = fsp;
    ASSERT(fs->magic == EXT2_MAGIC);
    ext2_inode_t inode;
    CHK(ext2_find_inote(fs, node->inode, &inode), "inode not exist");
    int32_t ret = ext2_write_file(fs, &inode, offset, len, buff);
    if (offset + len > inode.i_size) {
        dprintf("update inode %x size %x ==> %x", node->inode, inode.i_size, offset + len);
        inode.i_size = offset + len;
        CHK(ext2_update_inode(fs, node->inode, &inode), "");
    }
    return ret;
    _err:
    return -1;
}

int32_t ext2_fs_node_read(fs_node_t *node, __fs_special_t *fsp, uint32_t offset, uint32_t len, uint8_t *buff) {
    ext2_t *fs = fsp;
    ASSERT(fs->magic == EXT2_MAGIC);
    ext2_inode_t inode;
    CHK(ext2_find_inote(fs, node->inode, &inode), "inode not exist");
    int32_t ret = ext2_read_file(fs, &inode, offset, len, buff);
    return ret;
    _err:
    return -1;
}

int ext2_get_fs_node(ext2_t *fs, uint32_t inode_id, const char *fn, fs_node_t *node) {
    if (inode_id < 2) {
        deprintf("Bad inode_id:%x", inode_id);
        return 1;
    }
    strcpy(node->name, fn);
    ext2_inode_t inode;
    ext2_find_inote(fs, inode_id, &inode);
    node->mask = inode.i_mode;
    node->uid = inode.i_uid;
    node->gid = inode.i_gid;
    if (IS_DIR(inode.i_mode) || inode_id == 2) {
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
    ASSERT(fs->magic == EXT2_MAGIC);
    TODO;
}

int ext2_fs_node_make(fs_node_t *node, __fs_special_t *fsp, uint8_t type, char *name) {
    ext2_t *fs = fsp;
    uint32_t inode_id = node->inode;
    int e2type;
    switch (type) {
        case FS_DIRECTORY:
            e2type = INODE_TYPE_DIRECTORY;
            break;
        case FS_FILE:
            e2type = INODE_TYPE_FILE;
            break;
        default:
            deprintf("[ext2_fs_node_make]unknown type:%x", type);
            return -1;
    }
    uint32_t new_nnid;
    CHK(ext2_make(fs, e2type, inode_id, name, &new_nnid), "make failed.");
    //ext2_get_fs_node(fs, new_nnid, name, new_node_out);
    return 0;
    _err:
    return 1;
}

__fs_special_t *ext2_fs_node_mount(blk_dev_t *dev, fs_node_t *rootnode) {
    ext2_t *fs = kmalloc(sizeof(ext2_t));
    CHK(ext2_init(fs, dev), "[ext2]fail to mount");
    rootnode->inode = 2;
    rootnode->fsp = fs;
    rootnode->flags = FS_DIRECTORY;
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
}