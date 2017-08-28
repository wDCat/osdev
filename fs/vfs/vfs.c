//
// Created by dcat on 8/22/17.
//

#include <str.h>
#include <blk_dev.h>
#include <ext2.h>
#include <kmalloc.h>
#include <catmfs.h>
#include <vfs.h>
#include "include/vfs.h"

fs_node_t fs_root;
vfs_t test;
extern fs_t ext2_fs;
extern fs_t catmfs;
extern blk_dev_t dev;

void vfs_init(vfs_t *vfs) {
    memset(vfs, 0, sizeof(vfs_t));
}

int vfs_find_mount_point(vfs_t *vfs, const char *path, mount_point_t **mp_out, char **relatively_path) {
    int maxpathlen = 0;
    mount_point_t *mp = 0;
    for (int x = 0; x < mount_points_count; x++) {
        mp = &mount_points[x];
        int len = strlen(mp->path);
        if (strstr(path, mp->path) == 0 && len > maxpathlen) {
            maxpathlen = strlen(mp->path);
        }
    }
    if (mp_out)
        *mp_out = mp;
    if (relatively_path)
        *relatively_path = path + maxpathlen;
    if (mp == 0) {
        dprintf("%s fs not found!", path);
        return 1;
    } else {
        dprintf("%s fs:%s rpath:%s", path, mp->fs->name, *relatively_path);
        return 0;
    }

}

inline int vfs_cpynode(fs_node_t *target, fs_node_t *src) {
    memcpy(target, src, sizeof(fs_node_t));
}

int vfs_cd(vfs_t *vfs, const char *path) {
    dprintf("target:%s", path);
    fs_node_t cur;
    int x = 0;
    char *p = 0;
    char *rpath;
    int tlen = strlen(path);

    mount_point_t *mp;
    if (vfs_find_mount_point(vfs, path, &mp, &rpath)) {
        dprintf("mount point not found:%s", path);
        return 1;
    }
    if (tlen == 1 && path[0] == '/') {
        dprintf("cd to root");
        vfs_cpynode(&vfs->current, &mp->root);
        goto _succ;
    }
    vfs_cpynode(&cur, &mp->root);
    int slen = strlen(rpath);
    while (x < slen) {
        char name[256];
        int len;
        if ((p = strchr(&rpath[x], '/'))) {
            len = (int) (p - &rpath[x]);
            memcpy(name, &rpath[x], len);
            name[len] = '\0';
        } else if (x + 1 < slen) {
            len = slen - x + 1;
            memcpy(name, &rpath[x], len);
            name[len] = '\0';
        } else break;
        dprintf("cd fn:%s", name);
        fs_node_t tmp;
        if (mp->fs->finddir(&cur, mp->fsp, name, &tmp)) {
            dprintf("dir not found:%s", name);
            return 1;
        }
        vfs_cpynode(&cur, &tmp);
        x += len + 1;
    }
    vfs_cpynode(&vfs->current, &cur);
    _succ:
    if (vfs->current.flags == FS_DIRECTORY) {
        dprintf("cd to dir:%s", path);
        vfs_cpynode(&vfs->current_dir, &vfs->current);
    } else if (vfs->current.flags == FS_FILE)
        dprintf("cd to file:%s", path);
    else
        deprintf("cd to unknown thing(%x):%s", vfs->current.flags, path);
    strcpy(vfs->path, path);
    dprintf("%s inode:%x", path, vfs->current.inode);
    return 0;
}

int vfs_mount(vfs_t *vfs, const char *path, fs_t *fs, void *dev) {
    mount_point_t *mp = &mount_points[mount_points_count];
    mp->fsp = fs->mount(dev, &mp->root);
    dprintf("root inode:%x", mp->root.inode);
    if (mp->fsp == 0) {
        dprintf("fail to mount %s to %s", fs->name, path);
    }
    mp->fs = fs;
    strcpy(mp->path, path);
    mount_points_count++;
    return 0;
}

int32_t vfs_ls(vfs_t *vfs, dirent_t *dirs, uint32_t max_count) {
    mount_point_t *mp;
    char *rpath;
    if (vfs_find_mount_point(vfs, vfs->path, &mp, &rpath)) {
        dprintf("mount point not found:%s", vfs->path);
        return 1;
    }
    return mp->fs->readdir(&vfs->current_dir, mp->fsp, max_count, dirs);

}

int vfs_make(vfs_t *vfs, uint8_t type, const char *name) {
    mount_point_t *mp;
    char *rpath;
    if (vfs_find_mount_point(vfs, vfs->path, &mp, &rpath)) {
        dprintf("mount point not found:%s", vfs->path);
        return 1;
    }
    return mp->fs->make(&vfs->current_dir, mp->fsp, type, name);
}

inline int vfs_mkdir(vfs_t *vfs, const char *name) {
    return vfs_make(vfs, FS_DIRECTORY, name);
}

inline int vfs_touch(vfs_t *vfs, const char *name) {
    return vfs_make(vfs, FS_FILE, name);
}

int32_t vfs_read(vfs_t *vfs, uint32_t offset, uint32_t size, uint8_t *buff) {
    mount_point_t *mp;
    char *rpath;
    if (vfs_find_mount_point(vfs, vfs->path, &mp, &rpath)) {
        deprintf("mount point not found:%s", vfs->path);
        return -1;
    }
    if (vfs->current.flags != FS_FILE) {
        deprintf("try to read a directory or something else:%s", vfs->path);
        return -1;
    }
    return mp->fs->read(&vfs->current, mp->fsp, offset, size, buff);
}

int32_t vfs_write(vfs_t *vfs, uint32_t offset, uint32_t size, uint8_t *buff) {
    mount_point_t *mp;
    char *rpath;
    if (vfs_find_mount_point(vfs, vfs->path, &mp, &rpath)) {
        deprintf("mount point not found:%s", vfs->path);
        return -1;
    }
    if (vfs->current.flags != FS_FILE) {
        deprintf("try to write a directory or something else:%s", vfs->path);
        return -1;
    }
    return mp->fs->write(&vfs->current, mp->fsp, offset, size, buff);
}

int vfs_open(vfs_t *vfs, const char *name) {

}

void vfs_test() {
    dirent_t *dirs = (dirent_t *) kmalloc_paging(0x2000, NULL);
    ext2_create_fstype();
    vfs_init(&test);
    catmfs_create_fstype();
    vfs_mount(&test, "/", &ext2_fs, &dev);
    putf("[vfs]ROOTFS(CATMFS) mount to /\n");
    //pause();
    vfs_cd(&test, "/");
    ASSERT(vfs_make(&test, FS_FILE, STR("tio")) == 0);
    vfs_cd(&test, "/tio");
    char neko[250];
    strcpy(neko, "HelloDCat!");
    int size = 0x10;
    char *bigneko = (char *) kmalloc_paging(size, NULL);
    putf_const("bigneko:%x\n", bigneko);
    memset(bigneko, 'H', size);
    ASSERT(vfs_write(&test, 0, size, bigneko) == size);
    memset(bigneko, 'S', size);
    ASSERT(vfs_write(&test, 0, size, bigneko) == size);
    char nya[250];
    memset(nya, 0, 250);
    ASSERT(vfs_read(&test, 0xFE4, 20, nya) == 20);
    putf("read result:%s", nya);
    return;
    ASSERT(vfs_make(&test, FS_FILE, STR("nemkoo")) == 0);
    ASSERT(vfs_make(&test, FS_FILE, STR("nemykoo")) == 0);
    ASSERT(vfs_make(&test, FS_FILE, STR("akami")) == 0);
    ASSERT(vfs_make(&test, FS_DIRECTORY, STR("neko")) == 0);
    vfs_cd(&test, "/neko/");
    ASSERT(vfs_make(&test, FS_FILE, STR("s1")) == 0);
    ASSERT(vfs_make(&test, FS_DIRECTORY, STR("ext2")) == 0);
    int count = vfs_ls(&test, dirs, 20);
    putf("[vfs]there are %x files in /neko [%x]\n", count, test.current.inode);
    for (int x = 0; x < count; x++) {
        putf("[file]%s type:%x inode:%x\n", dirs[x].name, dirs[x].type, dirs[x].node);
    }
    //pause();
    vfs_mount(&test, "/neko/ext2/", &ext2_fs, &dev);
    putf("[vfs]ATA01(EXT2) mount to /neko/ext2\n");
    vfs_cd(&test, "/neko/ext2/");
    //vfs_make(&test, FS_FILE, STR("NNNNN"));
    count = vfs_ls(&test, dirs, 20);
    putf("[vfs]there are %x files in /neko/ext2/ [%x]\n", count, test.current.inode);
    for (int x = 0; x < count; x++) {
        putf("[file]%s type:%x inode:%x\n", dirs[x].name, dirs[x].type, dirs[x].node);
    }
    //pause();
    vfs_cd(&test, "/neko/ext2/neko/");
    //vfs_make(&test, FS_FILE, STR("NNNNN"));
    count = vfs_ls(&test, dirs, 20);
    putf("[vfs]there are %x files in /neko/ext2/neko/ [%x]\n", count, test.current.inode);
    for (int x = 0; x < count; x++) {
        putf("[file]%s type:%x inode:%x\n", dirs[x].name, dirs[x].type, dirs[x].node);
    }
    //pause();
    if (vfs_touch(&test, "ssr-v2")) {
        putf("touch failed.\n");
    }
    vfs_mkdir(&test, "ssvr");
    vfs_cd(&test, "/neko/ext2/neko/ssvr/");
    vfs_touch(&test, "a little cat sleeping here.");

}