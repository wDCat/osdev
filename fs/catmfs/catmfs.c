//
// Created by dcat on 8/25/17.
//


#include <kmalloc.h>
#include <catmfs.h>
#include <str.h>
#include <vfs.h>


fs_t catmfs;

catmfs_inode_t *catmfs_alloc_inode() {
    catmfs_inode_t *inode = (catmfs_inode_t *) kmalloc_paging(0x1000, NULL);
    if (inode == 0) deprintf("Out of memory!");
    memset(inode, 0, 0x1000);
    inode->magic = CATMFS_MAGIC;
    return inode;
}

int catmfs_fs_node_make(fs_node_t *node, __fs_special_t *fsp_, uint8_t type, char *name) {
    catmfs_inode_t *inode = (catmfs_inode_t *) node->inode;
    catmfs_special_t *fsp = fsp_;
    if (inode->magic != CATMFS_MAGIC) {
        deprintf("not a catmfs node[%x].", inode->magic);
        return 1;
    }
    catmfs_inode_t *ninode = catmfs_alloc_inode();
    ninode->type = type;
    ninode->size = type == FS_DIRECTORY ? 1024 : 0;
    uint8_t *raw = (uint8_t *) ((uint32_t) inode + sizeof(catmfs_inode_t));
    while (true) {
        catmfs_dir_t *dir = (catmfs_dir_t *) raw;
        if (dir->inode == 0) {
            dprintf("offset:%x name:%s", dir, name);
            uint8_t len = strlen(name);
            dir->name_len = len;
            dir->type = type;
            dir->inode = ninode;
            dir->len = 8 + dir->name_len;
            dir->len += 4 - (dir->len % 4);//4 align
            char *np = (char *) ((uint32_t) dir + 9);
            memcpy(np, name, len);
            return 0;
        }
        raw = (uint8_t *) ((uint32_t) raw + dir->len);
        if ((uint32_t) raw - (uint32_t) inode > 0x1000) {
            deprintf("Out of space.last:[%x]", raw);
            return 1;
        }
    }
}

int32_t catmfs_fs_node_readdir(fs_node_t *node, __fs_special_t *fsp_, uint32_t count, struct dirent *result) {
    catmfs_inode_t *inode = (catmfs_inode_t *) node->inode;
    catmfs_special_t *fsp = fsp_;
    if (inode->magic != CATMFS_MAGIC) {
        deprintf("not a catmfs node[%x].", inode->magic);
        return -1;
    }
    //putf_const("[catmfs_fs_node_readdir]target:%x\n", inode);
    uint8_t *raw = (uint8_t *) ((uint32_t) inode + sizeof(catmfs_inode_t));
    int x = 0;
    for (; x < count; x++) {
        catmfs_dir_t *dir = (catmfs_dir_t *) raw;
        if (dir->inode == 0 && x--)break;
        if (dir->name_len > 255) {
            deprintf("Bad namelen:%x", dir->name_len);
            return -1;
        }
        char *np = (char *) ((uint32_t) dir + 9);
        memcpy(result[x].name, np, dir->name_len);
        result[x].name[dir->name_len] = '\0';
        //putf_const("[catmfs_fs_node_readdir] offset:%x len:%x ", dir, dir->len);
        //putf_const("fn:%s\n", result[x].name);
        result[x].type = dir->type;
        result[x].name_len = dir->name_len;
        result[x].node = dir->inode;
        raw = (uint8_t *) ((uint32_t) raw + dir->len);
        if ((uint32_t) raw - (uint32_t) inode > 0x1000 && x--)
            break;
    }
    return x + 1;
}

int catmfs_fs_node_finddir(fs_node_t *node, __fs_special_t *fsp_, const char *name, fs_node_t *result_out) {
    catmfs_inode_t *inode = (catmfs_inode_t *) node->inode;
    catmfs_special_t *fsp = fsp_;
    if (inode->magic != CATMFS_MAGIC) {
        deprintf("not a catmfs node[%x].", inode->magic);
        return 1;
    }
    //putf_const("[catmfs_fs_node_finddir]target:%x\n", inode);
    uint8_t *raw = (uint8_t *) ((uint32_t) inode + sizeof(catmfs_inode_t));
    for (int x = 0;; x++) {
        char sname[256];
        catmfs_dir_t *dir = (catmfs_dir_t *) raw;
        if (dir->inode == 0)break;
        if (dir->name_len > 255) {
            dprintf("Bad namelen:%x", dir->name_len);
            return -1;
        }
        char *np = (char *) ((uint32_t) dir + 9);
        memcpy(sname, np, dir->name_len);
        sname[dir->name_len] = 0;

        if (strcmp(sname, name)) {
            catmfs_get_fs_node(dir->inode, result_out);
            return 0;
        }
        raw = (uint8_t *) ((uint32_t) raw + dir->len);
        if ((uint32_t) raw - (uint32_t) inode > fsp->page_size)
            break;
    }
    //not found
    return 1;
}

int catmfs_get_fs_node(uint32_t inode_id, fs_node_t *node) {
    catmfs_inode_t *inode = (catmfs_inode_t *) inode_id;
    if (inode->magic != CATMFS_MAGIC) {
        deprintf("not a catmfs node[%x].", inode->magic);
        return -1;
    }
    node->inode = inode_id;
    node->uid = inode->uid;
    node->gid = inode->gid;
    node->length = inode->size;
    node->flags = inode->type;

}

int32_t catmfs_fs_node_read(fs_node_t *node, __fs_special_t *fsp_, uint32_t offset, uint32_t size, uint8_t *buff) {
    catmfs_inode_t *inode = (catmfs_inode_t *) node->inode;
    catmfs_special_t *fsp = fsp_;
    uint8_t *raw = (uint8_t *) ((uint32_t) inode + sizeof(catmfs_inode_t));
    uint32_t off = 0, les = size, blkno = 0;
    uint32_t fblksize = fsp->page_size - sizeof(catmfs_inode_t);
    uint32_t pblksize = fsp->page_size - sizeof(catmfs_blk_t);
    if (offset < fblksize) {
        memcpy(&buff[off], raw, MIN(fblksize - offset, les));
        off += MIN(fblksize - offset, les);
        les -= off;
        offset = 0;
    } else offset -= fblksize;
    if (inode->singly_block == 0)
        goto _ret;
    uint32_t *blk_ptrs = (uint32_t *) inode->singly_block;
    if (offset > 0) {
        blkno = (uint8_t) (offset / pblksize);
        offset = offset % pblksize;
    }
    while (les > 0 && blkno < fsp->max_singly_blks) {
        catmfs_blk_t *dblk = (catmfs_blk_t *) blk_ptrs[blkno];
        if (dblk == 0)goto _ret;
        if (dblk->magic != CATMFS_BLK) {
            deprintf("bad catmfs block:%x", dblk);
            goto _ret;
        }
        uint8_t *braw;
        if (offset > 0) {
            braw = (uint8_t *) ((uint32_t) dblk + sizeof(catmfs_blk_t) + offset);
        } else {
            braw = (uint8_t *) ((uint32_t) dblk + sizeof(catmfs_blk_t));
        }
        uint32_t s = MIN(pblksize, les);
        memcpy(&buff[off], braw, s);
        off += s;
        les -= s;
        blkno++;
    }
    _ret:
    return size - les;
}

int32_t catmfs_fs_node_write(fs_node_t *node, __fs_special_t *fsp_, uint32_t offset, uint32_t size, uint8_t *buff) {
    catmfs_inode_t *inode = (catmfs_inode_t *) node->inode;
    catmfs_special_t *fsp = fsp_;
    uint8_t *raw = (uint8_t *) ((uint32_t) inode + sizeof(catmfs_inode_t));
    uint32_t off = 0, les = size, blkno = 0;
    uint32_t fblksize = fsp->page_size - sizeof(catmfs_inode_t);
    uint32_t pblksize = fsp->page_size - sizeof(catmfs_blk_t);
    putf_const("fblksize:%x pblksize:%x", fblksize, pblksize);
    dprintf("inode:%x begin:%x", inode, raw);
    if (offset < fblksize) {
        uint32_t s = MIN(fblksize - offset, les);
        dprintf("write %x-%x to fblk[%x]", off, off + s, &raw[offset]);
        memcpy(&raw[offset], &buff[off], s);
        off += s;
        les -= s;
        offset = 0;
    } else offset -= fblksize;
    if (inode->singly_block == 0 && les > 0) {
        inode->singly_block = kmalloc_paging(fsp->page_size, NULL);
        dprintf("[%x]allocated new sblk:%x", inode, inode->singly_block);
    }
    uint32_t *blk_ptrs = (uint32_t *) inode->singly_block;
    if (offset > 0) {
        blkno = (uint8_t) (offset / pblksize);
        offset = offset % pblksize;
    }
    while (les > 0 && blkno < fsp->max_singly_blks) {
        catmfs_blk_t *dblk = (catmfs_blk_t *) blk_ptrs[blkno];
        if (dblk == 0) {
            blk_ptrs[blkno] = (uint32_t) kmalloc_paging(fsp->page_size, NULL);
            dblk = (catmfs_blk_t *) blk_ptrs[blkno];
            dprintf("[%x]allocated new datablk for %x:%x", inode, blkno, dblk);
            dblk->magic = CATMFS_BLK;
        } else if (dblk->magic != CATMFS_BLK) {
            deprintf("bad catmfs block:%x", dblk);
            goto _ret;
        }
        uint8_t *braw;
        if (offset > 0) {
            braw = (uint8_t *) ((uint32_t) dblk + sizeof(catmfs_blk_t) + offset);
        } else {
            braw = (uint8_t *) ((uint32_t) dblk + sizeof(catmfs_blk_t));
        }
        uint32_t s = MIN(pblksize, les);
        dprintf("write %x-%x to pblk[no:%x][%x]", off, off + s, blkno, braw);
        memcpy(braw, &buff[off], s);
        off += s;
        les -= s;
        blkno++;
    }
    _ret:
    return size - les;

}

__fs_special_t *catmfs_fs_node_mount(void *dev, fs_node_t *node) {
    catmfs_special_t *fsp = (catmfs_special_t *) kmalloc(sizeof(catmfs_special_t));
    catmfs_inode_t *root = catmfs_alloc_inode();
    fsp->root_inode_id = (uint32_t) root;
    fsp->page_size = 0x1000;
    fsp->max_singly_blks = fsp->page_size / sizeof(uint32_t);
    root->type = FS_DIRECTORY;
    root->size = 1024;
    root->permission = 0xFFFF;
    root->uid = root->gid = 0;//root:root
    catmfs_get_fs_node(fsp->root_inode_id, node);
    //putf_const("[catmfs_fs_node_mount]root inode:%x\n", fsp->root_inode_id);
    return fsp;
}

void catmfs_create_fstype() {
    memset(&catmfs, 0, sizeof(fs_t));
    strcpy(catmfs.name, "CATMFS_TEST");
    catmfs.make = catmfs_fs_node_make;
    catmfs.readdir = catmfs_fs_node_readdir;
    catmfs.mount = catmfs_fs_node_mount;
    catmfs.finddir = catmfs_fs_node_finddir;
    catmfs.read = catmfs_fs_node_read;
    catmfs.write = catmfs_fs_node_write;
}