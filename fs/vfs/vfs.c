//
// Created by dcat on 8/22/17.
//

#include <str.h>
#include <blk_dev.h>
#include <ext2.h>
#include <kmalloc.h>
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
        putf_const("[vfs_find_mount_point]%s fs not found!\n", path);
        return 1;
    } else {
        putf_const("[vfs_find_mount_point]%s fs:%s rpath:", path, mp->fs->name);
        puts(*relatively_path);
        putln_const("");
        return 0;
    }

}

inline int vfs_cpynode(fs_node_t *target, fs_node_t *src) {
    memcpy(target, src, sizeof(fs_node_t));
}

int vfs_cd(vfs_t *vfs, const char *path) {
    putf_const("[vfs_cd] target:%s\n", path);
    fs_node_t cur;
    int x = 0;
    char *p = 0;
    char *rpath;
    int tlen = strlen(path);

    mount_point_t *mp;
    if (vfs_find_mount_point(vfs, path, &mp, &rpath)) {
        putf_const("[vfs_cd]mount point not found:%s\n", path);
        return 1;
    }
    if (tlen == 1 && path[0] == '/') {
        putf_const("[vfs_cd] cd to root\n");
        vfs_cpynode(&vfs->current, &mp->root);
        goto _succ;
    }
    vfs_cpynode(&cur, &mp->root);
    int slen = strlen(rpath);
    while (x < slen && (p = strchr(&rpath[x], '/'))) {
        int len = (int) (p - &rpath[x]);
        char name[256];
        memcpy(name, &rpath[x], len);
        name[len] = '\0';
        putf_const("[vfs_cd]cd fn:%s\n", name);
        fs_node_t tmp;
        if (mp->fs->finddir(&cur, mp->fsp, name, &tmp)) {
            putf_const("[vfs_cd]dir not found:%s\n", name);
            return 1;
        }
        vfs_cpynode(&cur, &tmp);
        x += len + 1;
    }
    vfs_cpynode(&vfs->current, &cur);
    _succ:
    strcpy(vfs->path, path);
    putf_const("[vfs_cd]%s inode:%x\n", path, vfs->current.inode);
    return 0;
}

int vfs_mount(vfs_t *vfs, const char *path, fs_t *fs, void *dev) {
    mount_point_t *mp = &mount_points[mount_points_count];
    mp->fsp = fs->mount(dev, &mp->root);
    putf_const("[vfs_mount]root inode:%x\n", mp->root.inode);
    if (mp->fsp == 0) {
        putf_const("[vfs_mount]fail to mount %s to %s", fs->name, path);
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
        putf_const("[vfs_ls]mount point not found:%s\n", vfs->path);
        return 1;
    }
    return mp->fs->readdir(&vfs->current, mp->fsp, max_count, dirs);

}

int vfs_make(vfs_t *vfs, uint8_t type, char *name) {
    mount_point_t *mp;
    char *rpath;
    if (vfs_find_mount_point(vfs, vfs->path, &mp, &rpath)) {
        putf_const("[vfs_make]mount point not found:%s\n", vfs->path);
        return 1;
    }
    return mp->fs->make(&vfs->current, mp->fsp, type, name);
}

int vfs_open(vfs_t *vfs, const char *name) {

}

void vfs_test() {
    dirent_t *dirs = (dirent_t *) kmalloc_paging(0x2000, NULL);
    ext2_create_fstype();
    vfs_init(&test);
    catmfs_create_fstype();
    vfs_mount(&test, "/", &catmfs, NULL);
    putf_const("[vfs]ROOTFS(CATMFS) mount to /\n");
    pause();
    vfs_cd(&test, "/");
    ASSERT(vfs_make(&test, FS_FILE, STR("nemkoo")) == 0);
    ASSERT(vfs_make(&test, FS_FILE, STR("nemykoo")) == 0);
    ASSERT(vfs_make(&test, FS_FILE, STR("akami")) == 0);
    ASSERT(vfs_make(&test, FS_DIRECTORY, STR("neko")) == 0);
    vfs_cd(&test, "/neko/");
    ASSERT(vfs_make(&test, FS_FILE, STR("s1")) == 0);
    ASSERT(vfs_make(&test, FS_DIRECTORY, STR("ext2")) == 0);
    int count = vfs_ls(&test, dirs, 20);
    putf_const("[vfs]there are %x files in /neko [%x]\n", count, test.current.inode);
    for (int x = 0; x < count; x++) {
        putf_const("[file]%s type:%x inode:%x\n", dirs[x].name, dirs[x].type, dirs[x].node);
    }
    pause();
    vfs_mount(&test, "/neko/ext2/", &ext2_fs, &dev);
    putf_const("[vfs]ATA01(EXT2) mount to /neko/ext2\n");
    vfs_cd(&test, "/neko/ext2/");
    //vfs_make(&test, FS_FILE, STR("NNNNN"));
    count = vfs_ls(&test, dirs, 20);
    putf_const("[vfs]there are %x files in /neko/ext2/ [%x]\n", count, test.current.inode);
    for (int x = 0; x < count; x++) {
        putf_const("[file]%s type:%x inode:%x\n", dirs[x].name, dirs[x].type, dirs[x].node);
    }
    pause();
    vfs_cd(&test, "/neko/ext2/neko/");
    //vfs_make(&test, FS_FILE, STR("NNNNN"));
    count = vfs_ls(&test, dirs, 20);
    putf_const("[vfs]there are %x files in /neko/ext2/neko/ [%x]\n", count, test.current.inode);
    for (int x = 0; x < count; x++) {
        putf_const("[file]%s type:%x inode:%x\n", dirs[x].name, dirs[x].type, dirs[x].node);
    }
}