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
    if (inode == 0) PANIC("Out of memory!");
    memset(inode, 0, 0x1000);
    inode->magic = CATMFS_MAGIC;
    return inode;
}

int catmfs_fs_node_make(fs_node_t *node, __fs_special_t *fsp_, uint8_t type, char *name) {
    catmfs_inode_t *inode = (catmfs_inode_t *) node->inode;
    catmfs_special_t *fsp = fsp_;
    if (inode->magic != CATMFS_MAGIC) {
        putf_const("[catmfs_make]not a catmfs node[%x].\n", inode->magic);
        return 1;
    }
    catmfs_inode_t *ninode = catmfs_alloc_inode();
    ninode->type = type;
    ninode->size = type == FS_DIRECTORY ? 1024 : 0;
    uint8_t *raw = (uint8_t *) ((uint32_t) inode + sizeof(catmfs_inode_t));
    while (true) {
        catmfs_dir_t *dir = (catmfs_dir_t *) raw;
        if (dir->inode == 0) {
            putf_const("[catmfs_make]offset:%x name:%s\n", dir, name);
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
            putf_const("[catmfs_make]Out of space.last:[%x]\n", raw);
            return 1;
        }
    }
}

int32_t catmfs_fs_node_readdir(fs_node_t *node, __fs_special_t *fsp_, uint32_t count, struct dirent *result) {
    catmfs_inode_t *inode = (catmfs_inode_t *) node->inode;
    catmfs_special_t *fsp = fsp_;
    if (inode->magic != CATMFS_MAGIC) {
        putf_const("[catmfs_fs_node_readdir]not a catmfs node[%x].\n", inode->magic);
        return -1;
    }
    //putf_const("[catmfs_fs_node_readdir]target:%x\n", inode);
    uint8_t *raw = (uint8_t *) ((uint32_t) inode + sizeof(catmfs_inode_t));
    int x = 0;
    for (; x < count; x++) {
        catmfs_dir_t *dir = (catmfs_dir_t *) raw;
        if (dir->inode == 0 && x--)break;
        if (dir->name_len > 255) {
            putf_const("[catmfs_fs_node_readdir]Bad namelen:%x\n", dir->name_len);
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
        putf_const("[catmfs_fs_node_finddir]not a catmfs node[%x].\n", inode->magic);
        return 1;
    }
    //putf_const("[catmfs_fs_node_finddir]target:%x\n", inode);
    uint8_t *raw = (uint8_t *) ((uint32_t) inode + sizeof(catmfs_inode_t));
    for (int x = 0;; x++) {
        char sname[256];
        catmfs_dir_t *dir = (catmfs_dir_t *) raw;
        if (dir->inode == 0)break;
        if (dir->name_len > 255) {
            putf_const("[catmfs_fs_node_finddir]Bad namelen:%x\n", dir->name_len);
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
        if ((uint32_t) raw - (uint32_t) inode > 0x1000)
            break;
    }
    //not found
    return 1;
}

int catmfs_get_fs_node(uint32_t inode_id, fs_node_t *node) {
    catmfs_inode_t *inode = (catmfs_inode_t *) inode_id;
    if (inode->magic != CATMFS_MAGIC) {
        putf_const("[catmfs_get_fs_node]not a catmfs node[%x].\n", inode->magic);
        return -1;
    }
    node->inode = inode_id;
    node->uid = inode->uid;
    node->gid = inode->gid;
    node->length = inode->size;

}

__fs_special_t *catmfs_fs_node_mount(void *dev, fs_node_t *node) {
    catmfs_special_t *fsp = (catmfs_special_t *) kmalloc(sizeof(catmfs_special_t));
    catmfs_inode_t *root = catmfs_alloc_inode();
    fsp->root_inode_id = (uint32_t) root;
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
}