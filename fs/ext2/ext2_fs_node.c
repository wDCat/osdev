//
// Created by dcat on 9/29/17.
//

#include <fs_node.h>
#include <str.h>
#include <kmalloc.h>
#include <vfs.h>
#include "ext2_fs_node.h"

extern int ext2_find_inote(ext2_t *ext2_fs, uint32_t inode_id,
                           ext2_inode_t *inode_out);

extern int32_t ext2_write_file(ext2_t *fs, ext2_inode_t *inode,
                               uint32_t offset, uint32_t size, uint8_t *buff);

extern int ext2_update_inode(ext2_t *fs, uint32_t inode_id,
                             ext2_inode_t *new_inode);

extern int ext2_read_file(ext2_t *fs, ext2_inode_t *inode,
                          uint32_t offset, uint32_t size, uint8_t *buff);

extern void ext2_dir_iterator_init(ext2_t *fs, ext2_inode_t *inode,
                                   ext2_dir_iterator_t *iter);

extern int ext2_dir_iterator_next(ext2_dir_iterator_t *iter,
                                  ext2_dir_t **dir);

extern int ext2_dir_iterator_exit(ext2_dir_iterator_t *iter);

extern int ext2_make(ext2_t *fs, uint16_t type,
                     uint32_t father_id, char *name,
                     uint32_t *new_id_out);

extern int ext2_init(ext2_t *fs_out, blk_dev_t *dev);

extern blk_dev_t dev;
extern fs_t ext2_fs;

int32_t ext2_fs_node_write(fs_node_t *node, __fs_special_t *fsp, uint32_t len, uint8_t *buff) {
    uint32_t offset = node->offset;
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
    node->offset += ret;
    return ret;
    _err:
    return -1;
}

int32_t ext2_fs_node_read(fs_node_t *node, __fs_special_t *fsp, uint32_t len, uint8_t *buff) {
    uint32_t offset = node->offset;
    ext2_t *fs = fsp;
    ASSERT(fs->magic == EXT2_MAGIC);
    ext2_inode_t inode;
    CHK(ext2_find_inote(fs, node->inode, &inode), "inode not exist");
    int32_t ret = ext2_read_file(fs, &inode, offset, len, buff);
    node->offset += ret;
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

    node->size = inode.i_size;
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

int ext2_fs_node_lseek(fs_node_t *node, __fs_special_t *fsp_, uint32_t offset) {
    //do nothing
    return 0;
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
    strcpy(ext2_fs.name, STR("ext2"));
    ext2_fs.mount = ext2_fs_node_mount;
    ext2_fs.read = ext2_fs_node_read;
    ext2_fs.write = ext2_fs_node_write;
    ext2_fs.readdir = ext2_fs_node_readdir;
    ext2_fs.finddir = ext2_fs_node_finddir;
    ext2_fs.getnode = ext2_fs_node_get_node;
    ext2_fs.make = ext2_fs_node_make;
    ext2_fs.lseek = ext2_fs_node_lseek;
}