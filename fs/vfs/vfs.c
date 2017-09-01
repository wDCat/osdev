//
// Created by dcat on 8/22/17.
//

#include <str.h>
#include <blk_dev.h>
#include <ext2.h>
#include <kmalloc.h>
#include <catmfs.h>
#include <vfs.h>
#include "proc.h"

fs_node_t fs_root;
vfs_t test;
extern fs_t ext2_fs;
extern fs_t catmfs;
extern blk_dev_t dev;

void stdio_install() {
    pcb_t *pcb = getpcb(1);
    pcb->fh[0].present = 1;
    pcb->fh[1].present = 1;
    pcb->fh[2].present = 1;
}

void vfs_install() {
    memset(mount_points, 0, sizeof(mount_point_t) * MAX_MOUNT_POINTS);
    mount_points_count = 0;
    memset(global_fh_table, 0, sizeof(file_handle_t) * MAX_FILE_HANDLES);
    stdio_install();
}

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


int vfs_get_node(vfs_t *vfs, const char *path, fs_node_t *node) {
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
        vfs_cpynode(node, &mp->root);
        return 0;
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
            dprintf("obj not found:%s", name);
            return 1;
        }
        vfs_cpynode(&cur, &tmp);
        x += len + 1;
    }
    vfs_cpynode(node, &cur);
    return 0;
}

int vfs_cd(vfs_t *vfs, const char *path) {
    if (vfs_get_node(vfs, path, &vfs->current)) {
        deprintf("no such file or dir:%s", path);
        return 1;
    }
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

void vfs_cd_pdir(vfs_t *vfs) {
    vfs_cpynode(&vfs->current, &vfs->current_dir);
}

int vfs_mount(vfs_t *vfs, const char *path, fs_t *fs, void *dev) {
    mount_point_t *mp = &mount_points[mount_points_count];
    mp->fsp = fs->mount(dev, &mp->root);
    dprintf("root inode:%x", mp->root.inode);
    if (mp->fsp == 0) {
        deprintf("fail to mount %s to %s", fs->name, path);
        return 1;
    }
    mp->fs = fs;
    strcpy(mp->path, path);
    mount_points_count++;
    dprintf("%s mount to %s", fs->name, path);
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

int32_t vfs_read(vfs_t *vfs, uint32_t offset, uint32_t size, uchar_t *buff) {
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

int32_t vfs_write(vfs_t *vfs, uint32_t offset, uint32_t size, uchar_t *buff) {
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

int vfs_rm(vfs_t *vfs) {
    mount_point_t *mp;
    char *rpath;
    if (vfs_find_mount_point(vfs, vfs->path, &mp, &rpath)) {
        deprintf("mount point not found:%s", vfs->path);
        return -1;
    }
    return mp->fs->rm(&vfs->current, mp->fsp);
}

int8_t sys_open(const char *name, uint8_t mode) {
    pcb_t *pcb = getpcb(getpid());
    const char *path = name;
    mount_point_t *mp;
    int8_t fd = -1;
    for (int8_t x = 0; x < MAX_FILE_HANDLES; x++) {
        if (!pcb->fh[x].present) {
            fd = x;
            break;
        }
    }
    if (fd == -1) {
        deprintf("cannot open more than %x files.", MAX_FILE_HANDLES);
        return -1;
    }
    file_handle_t *fh = &pcb->fh[fd];
    if (vfs_get_node(&pcb->vfs, path, &fh->node)) {
        deprintf("no such file or dir:%s", name);
        return -1;
    }
    if (vfs_find_mount_point(&pcb->vfs, path, &mp, NULL)) {
        dprintf("mount point not found:%s", path);
        return 1;
    }
    fh->present = 1;
    fh->mode = mode;
    fh->mp = mp;
    dprintf("proc %x open %s fd:%x", getpid(), path, fd);
    return fd;
}

int sys_close(int8_t fd) {
    pcb_t *pcb = getpcb(getpid());
    if (!pcb->fh[fd].present) {
        deprintf("cannot close a closed file:%x", fd);
        return -1;
    }
    pcb->fh[fd].present = 0;
    return 0;
}

int32_t kread(uint32_t pid, int8_t fd, uint32_t offset, int32_t size, uchar_t *buff) {
    pcb_t *pcb = getpcb(pid);
    file_handle_t *fh = &pcb->fh[fd];
    if (!fh->present) {
        deprintf("fd %x not present.", fd);
        return -1;
    }
    if (fh->mp == 0) {
        deprintf("[%x]mount point not found.", fd);
        return -1;
    }
    int32_t ret = fh->mp->fs->read(&fh->node, fh->mp->fsp, offset, size, buff);

    return ret;
}

int32_t sys_read(int8_t fd, int32_t size, uchar_t *buff) {
    pcb_t *pcb = getpcb(getpid());
    file_handle_t *fh = &pcb->fh[fd];
    int32_t ret = kread(getpid(), fd, fh->offset, size, buff);
    fh->offset += ret;
    return ret;
}

void vfs_test() {
    ext2_create_fstype();
    vfs_init(&test);
    catmfs_create_fstype();
    vfs_mount(&test, "/", &ext2_fs, &dev);

    putf("[vfs]ROOTFS(CATMFS) mount to /\n");
    putf("[vfs]proc pid:%x\n", getpid());
    int fd = sys_open("/a.elf", 0);
    putf("[vfs]open fd:%x\n", fd);
    uchar_t *d = (uchar_t *) kmalloc_paging(0x4000, NULL);
    putf_const("data:%x\n", d);
    putf_const("read %x bytes\n", sys_read(fd, 0x4000, d));
    d[10] = 0;
    putf(d);
    for (;;);
}

void vfs_test_old() {
    dirent_t *dirs = (dirent_t *) kmalloc_paging(0x2000, NULL);
    ext2_create_fstype();
    vfs_init(&test);
    catmfs_create_fstype();
    vfs_mount(&test, "/", &catmfs, &dev);
    putf("[vfs]ROOTFS(CATMFS) mount to /\n");
    //pause();
    vfs_cd(&test, "/");
    ASSERT(vfs_make(&test, FS_FILE, STR("tio")) == 0);
    vfs_cd(&test, "/tio");
    char neko[250];
    strcpy(neko, "HelloDCat!");
    int size = 0x40;
    char *bigneko = (char *) kmalloc_paging(size, NULL);
    putf_const("bigneko:%x\n", bigneko);
    memset(bigneko, 'H', size);
    int32_t a = vfs_write(&test, 0, 10, bigneko);
    kfree(bigneko);
    size = 0x2032;
    bigneko = (char *) kmalloc_paging(size, NULL);
    putf_const("write[1]ret:%x\n", a);
    memset(bigneko, 'P', size);
    a = vfs_write(&test, 10, 10, bigneko);
    putf_const("write[2]ret:%x\n", a);
    char *nya = (char *) kmalloc_paging(size, NULL);
    memset(nya, 0, 2032);
    int ret = vfs_read(&test, 10, 10, nya);
    putnf("[%x][%d]read result:%s", 100, nya, ret, nya);

    ASSERT(vfs_make(&test, FS_FILE, STR("nemkoo")) == 0);
    ASSERT(vfs_make(&test, FS_FILE, STR("nemykoo")) == 0);
    ASSERT(vfs_make(&test, FS_FILE, STR("akami")) == 0);
    ASSERT(vfs_make(&test, FS_DIRECTORY, STR("neko")) == 0);
    vfs_cd(&test, "/neko");
    ASSERT(vfs_make(&test, FS_FILE, STR("akami2")) == 0);
    vfs_cd(&test, "/neko/akami2");
    ASSERT(vfs_rm(&test) == 0);
    vfs_cd(&test, "/neko");
    ASSERT(vfs_rm(&test) == 0);
    vfs_cd(&test, "/");
    int count = vfs_ls(&test, dirs, 20);
    putf("[vfs]there are %x files in / [%x]\n", count, test.current.inode);
    for (int x = 0; x < count; x++) {
        putf("[file]%s type:%x inode:%x\n", dirs[x].name, dirs[x].type, dirs[x].node);
    }
    ASSERT(vfs_make(&test, FS_DIRECTORY, STR("neko")) == 0);
    vfs_cd(&test, "/neko/");
    ASSERT(vfs_make(&test, FS_FILE, STR("s1")) == 0);
    ASSERT(vfs_make(&test, FS_DIRECTORY, STR("ext2")) == 0);
    count = vfs_ls(&test, dirs, 20);
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