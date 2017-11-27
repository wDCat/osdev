//
// Created by dcat on 8/22/17.
//

#include <str.h>
#include <blk_dev.h>
#include <ext2.h>
#include "../../memory/include/kmalloc.h"
#include <catmfs.h>
#include <vfs.h>
#include <catrfmt.h>
#include <ustat.h>
#include "../../dev/tty/include/tty.h"
#include <procfs.h>
#include <devfs.h>
#include <devfile.h>
#include "proc.h"

mount_point_t mount_points[MAX_MOUNT_POINTS];
uint32_t mount_points_count;
file_handle_t global_fh_table[MAX_FILE_HANDLES];
fs_node_t fs_root;
vfs_t vfs;
mutex_lock_t vfs_lock;
extern fs_t ext2_fs;
extern fs_t catmfs;
extern blk_dev_t dev;
bool __vfs_ready = false;

inline bool vfs_ready() {
    return __vfs_ready;
}

void stdio_install() {
    pcb_t *pcb = getpcb(1);
    kclose_all(1);
    ASSERT(kopen(1, "/dev/stdin", 0) == 0);//STDIN
    ASSERT(kopen(1, "/dev/stdout", 0) == 1);//STDOUT
    ASSERT(kopen(1, "/dev/stderr", 0) == 2);//STDERR
}

void vfs_install() {
    memset(mount_points, 0, sizeof(mount_point_t) * MAX_MOUNT_POINTS);
    mount_points_count = 0;
    memset(global_fh_table, 0, sizeof(file_handle_t) * MAX_FILE_HANDLES);
    mutex_init(&vfs_lock);
}

void vfs_init(vfs_t *vfs) {
    memset(vfs, 0, sizeof(vfs_t));
}

int vfs_cloneobj(vfs_t *src, vfs_t *target) {
    memcpy(target, src, sizeof(vfs_t));
    return 0;
}

inline mount_point_t *vfs_get_mount_point(vfs_t *vfs, fs_node_t *node) {
    return node->vfs_mp;
}

int vfs_find_mount_point(vfs_t *vfs, const char *path, mount_point_t **mp_out, char **relatively_path) {
    dprintf("mount point count:%x", mount_points_count);
    int maxpathlen = 0, mx = -1;
    mount_point_t *mp = 0;
    for (int x = 0; x < mount_points_count; x++) {
        mp = &mount_points[x];
        int len = strlen(mp->path);
        if (strstr(path, mp->path) == 0 && len > maxpathlen) {
            maxpathlen = len;
            mx = x;
        }
    }
    if (mx == -1) {
        dprintf("%s fs not found!", path);
        return 1;
    } else {
        mp = &mount_points[mx];
        if (mp_out)
            *mp_out = mp;
        if (relatively_path)
            if (maxpathlen > strlen(path))
                *relatively_path = 0;
            else
                *relatively_path = (char *) (path + maxpathlen);
        dprintf("%s fs:%s", path, mp->fs->name);
        return 0;
    }
}

inline int vfs_cpynode(fs_node_t *target, fs_node_t *src) {
    memcpy(target, src, sizeof(fs_node_t));
}

int vfs_resolve_symlink(vfs_t *vfs, mount_point_t *mp, fs_node_t *node, fs_node_t *node_out) {
    if (node->type != FS_SYMLINK) {
        deprintf("not a symlink node!");
        return -1;
    }
    if (mp->fs->readlink == 0) {
        deprintf("fs operator not implemented");
        return -1;
    }
    char *buff = kmalloc(MAX_PATH_LEN);
    CHK(mp->fs->readlink(node, mp->fsp, buff, MAX_PATH_LEN), "");
    dprintf("symlink %x===>%s", node->inode, buff);
    CHK(vfs_get_node(vfs, buff, node_out), "");
    return 0;
    _err:
    deprintf("fail to resolve symlink!");
    return -1;
}

int vfs_get_node4(vfs_t *vfs, const char *path, fs_node_t *node, bool follow_link) {
    dprintf("target:%s follow_link:%x", path, follow_link);
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
        if (node)
            vfs_cpynode(node, &mp->root);
        return 0;
    }
    vfs_cpynode(&cur, &mp->root);
    if (rpath != 0) {
        int slen = strlen(rpath);
        while (x < slen) {
            char name[256];
            int len;
            if ((p = strchr(&rpath[x], '/'))) {
                len = (int) (p - &rpath[x]);
                if (len == 0) {
                    x++;
                    continue;
                }
                memcpy(name, &rpath[x], len);
                name[len] = '\0';
            } else if (x < slen) {
                len = slen - x + 1;
                memcpy(name, &rpath[x], len);
                name[len] = '\0';
            } else break;
            dprintf("cd fn:%s", name);
            fs_node_t tmp;
            if (mp->fs->finddir == 0) {
                deprintf("fs operator not implemented");
                return 1;
            }
            if (mp->fs->finddir(&cur, mp->fsp, name, &tmp)) {
                dprintf("obj not found:%s", name);
                if (node)
                    vfs_cpynode(node, &cur);
                return 1;
            }
            tmp.vfs_mp = mp;
            if (tmp.type == FS_SYMLINK && follow_link) {
                char *buff = kmalloc(MAX_PATH_LEN);
                if (mp->fs->readlink(&tmp, mp->fsp, buff, MAX_PATH_LEN)) {
                    deprintf("fail to resolve symlink:%s", name);
                    return 1;
                }
                dprintf("symlink following to %s x=%x", buff, x);
                //generate new path
                //FIXME //////
                if (buff[0] != '/') {
                    dmemcpy(&buff[x + 1], buff, strlen(buff));
                    memcpy(&buff[1], rpath, x);
                    buff[0] = '/';
                }
                if (buff[strlen(buff) - 1] != '/')
                    strcat(buff, "/");
                if (x + len + 1 < slen)
                    strcat(buff, &rpath[x + len + 1]);
                dprintf("new path:%s", buff);
                int ret = vfs_get_node(vfs, buff, node);
                kfree(buff);
                return ret;
            }
            vfs_cpynode(&cur, &tmp);
            x += len + 1;
        }
    }
    if (node)
        vfs_cpynode(node, &cur);
    return 0;
}

inline int vfs_get_node(vfs_t *vfs, const char *path, fs_node_t *node) {
    return vfs_get_node4(vfs, path, node, true);
}

int vfs_cd(vfs_t *vfs, const char *path) {
    if (vfs_get_node(vfs, path, &vfs->current)) {
        deprintf("no such file or dir:%s", path);
        return 1;
    }
    if (vfs->current.type == FS_DIRECTORY) {
        dprintf("cd to dir:%s", path);
        vfs_cpynode(&vfs->current_dir, &vfs->current);
    } else if (vfs->current.type == FS_FILE)
        dprintf("cd to file:%s", path);
    else
        deprintf("cd to unknown thing(%x):%s", vfs->current.type, path);
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
    mp->root.vfs_mp = mp;
    strcpy(mp->path, path);
    mount_points_count++;

    if (!str_compare(path, "/")) {
        //create .. and .
        vfs_t tmpvfs;
        vfs_cloneobj(vfs, &tmpvfs);
        vfs_cd(&tmpvfs, path);
        vfs_symlink(&tmpvfs, "..", "..");
        vfs_symlink(&tmpvfs, ".", ".");
    }
    dprintf("%s mount to %s", fs->name, path);
    return 0;
}

int32_t vfs_symlink(vfs_t *vfs, const char *name, const char *target) {
    mount_point_t *mp;
    fs_node_t srcnode;
    mp = vfs_get_mount_point(vfs, &vfs->current);
    if (mp == NULL) {
        deprintf("mount point not found");
        return -1;
    }
    if (mp->fs->symlink == 0 || mp->fs->make == 0) {
        deprintf("fs operator not implemented");
        return -1;
    }
    if (mp->fs->make(&vfs->current_dir, mp->fsp, FS_SYMLINK, name) ||
        mp->fs->finddir(&vfs->current_dir, mp->fsp, name, &srcnode)) {
        deprintf("fail to make symbol file.");
        return -1;
    }
    return mp->fs->symlink(&srcnode, mp->fsp, target);

}

int32_t vfs_ls(vfs_t *vfs, dirent_t *dirs, uint32_t max_count) {
    mount_point_t *mp;
    mp = vfs_get_mount_point(vfs, &vfs->current);
    if (mp == NULL) {
        dprintf("mount point not found:%s", vfs->path);
        return 1;
    }
    if (mp->fs->readdir == 0) {
        deprintf("fs operator not implemented");
        return -1;
    }
    dprintf("ls %s calling %x", vfs->path, mp->fs->readdir);
    return mp->fs->readdir(&vfs->current_dir, mp->fsp, max_count, dirs);

}

int vfs_make(vfs_t *vfs, uint8_t type, const char *name) {
    mount_point_t *mp;
    mp = vfs_get_mount_point(vfs, &vfs->current);
    if (mp == NULL) {
        dprintf("mount point not found:%s", vfs->path);
        return 1;
    }
    if (mp->fs->make == 0) {
        deprintf("fs operator not implemented");
        return -1;
    }
    return mp->fs->make(&vfs->current_dir, mp->fsp, type, name);
}

inline int vfs_mkdir(vfs_t *vfs, const char *name) {
    return vfs_make(vfs, FS_DIRECTORY, name);
}

inline int vfs_touch(vfs_t *vfs, const char *name) {
    return vfs_make(vfs, FS_FILE, name);
}

int32_t vfs_read(vfs_t *vfs, uint32_t size, uchar_t *buff) {
    mount_point_t *mp;
    mp = vfs_get_mount_point(vfs, &vfs->current);
    if (mp == NULL) {
        deprintf("mount point not found:%s", vfs->path);
        return -1;
    }
    if (vfs->current.type != FS_FILE) {
        deprintf("try to read a directory or something else:%s", vfs->path);
        return -1;
    }
    if (mp->fs->read == 0) {
        deprintf("fs operator not implemented");
        return -1;
    }
    return mp->fs->read(&vfs->current, mp->fsp, size, buff);
}

int32_t vfs_write(vfs_t *vfs, uint32_t size, uchar_t *buff) {
    mount_point_t *mp;
    mp = vfs_get_mount_point(vfs, &vfs->current);
    if (mp == NULL) {
        deprintf("mount point not found:%s", vfs->path);
        return -1;
    }
    if (vfs->current.type != FS_FILE) {
        deprintf("try to write a directory or something else:%s", vfs->path);
        return -1;
    }
    if (mp->fs->write == 0) {
        deprintf("fs operator not implemented");
        return -1;
    }
    return mp->fs->write(&vfs->current, mp->fsp, size, buff);
}

int vfs_rm(vfs_t *vfs) {
    //TODO rm a symlink or a true file.
    mount_point_t *mp;
    char *rpath;
    if (vfs_find_mount_point(vfs, vfs->path, &mp, &rpath)) {
        deprintf("mount point not found:%s", vfs->path);
        return -1;
    }
    return mp->fs->rm(&vfs->current, mp->fsp);
}

int vfs_pretty_path(const char *path, char *out) {
    int off = 0;
    if (out != NULL && path != out) {
        strcpy(out, path);
    } else {
        out = (char *) path;
    }
    while (true) {
        if (off > strlen(out))break;
        char *o = strchr(&out[off], '/');
        int ol;
        if (o == NULL) {
            ol = strlen(out) - off;
        } else {
            ol = (int) (o - out - off);
        }
        if (ol == 2 && out[off] == '.' && out[off + 1] == '.') {
            int y = off - 2;
            for (; y >= 0; y--) {
                if (out[y] == '/')break;
            }
            if (o != NULL) {
                int len = strlen(o);
                memcpy(&out[y], o, len);
                out[y + len] = 0;
            } else out[y] = 0;
            off = y + 1;
        } else if (ol == 1 && out[off] == '.') {
            int len = strlen(&out[off + 2]);
            memcpy(&out[off], &out[off + 2], len);
            out[off + len] = 0;
        } else off += ol + 1;
        if (o == NULL)break;
    }

    dprintf("pretty path: %s", out);
}


int vfs_fix_path(uint32_t pid, const char *name, char *out, int max_len) {
    const char *cwd = kgetcwd(pid);
    int p = 0, len = strlen(name), cwd_len = strlen(cwd);
    if (len >= MAX_PATH_LEN) {
        deprintf("name too long//");
        sprintf(out, "/");
        return 2;
    }
    switch (name[0]) {
        case '.':
            if (cwd_len + len - 2 >= MAX_PATH_LEN)
                goto _err;
            if (len > 1 && name[1] == '/') {
                p++;
                sprintf(out, "%s%s", cwd, &name[p + 1]);
            } else {
                if (len >= 1 && name[1] == '/')p++;
                sprintf(out, "%s%s", cwd, &name[p]);
            }
            break;
        case '/':
            sprintf(out, "%s", name);
            break;
        default:
            if (cwd_len + len - 1 >= MAX_PATH_LEN)
                goto _err;
            if (len >= 1 && name[1] == '/')p++;
            sprintf(out, "%s%s", cwd, &name[p]);
    }
    dprintf("fix path:%s ==> %s", name, out);
    return 0;
    _err:
    sprintf(out, "%s", name);
    return 1;//ignored
}

int8_t kopen(uint32_t pid, const char *name, int flags) {
    pcb_t *pcb = getpcb(pid);
    char *path = (char *) kmalloc(MAX_PATH_LEN);
    vfs_fix_path(pid, name, path, MAX_PATH_LEN);
    vfs_pretty_path(path, NULL);
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
        goto _ret;
    }
    file_handle_t *fh = &pcb->fh[fd];
    if (vfs_get_node(&pcb->vfs, path, &fh->node)) {
        if (!(flags & O_CREAT)) {
            deprintf("no such file or dir:%s type:%d", name, flags);
            fd = -1;
            goto _ret;
        } else {
            int x = strlen(path) - 2;
            while (path[x] != '/' && x >= 0)x--;
            if (x == 0 && path[0] != '/') {
                deprintf("Bad path:%s", path);
                fd = -1;
                goto _ret;
            }
            char origchr = path[x + 1];
            path[x + 1] = 0;
            vfs_cd(&pcb->vfs, path);
            path[x + 1] = origchr;
            dprintf("path:%s fn:%s", path, &path[x + 1]);
            if (vfs_make(&pcb->vfs, FS_FILE, &path[x + 1]) || vfs_get_node(&pcb->vfs, path, &fh->node)) {
                deprintf("cannot make new file:%s", path);
                fd = -1;
                goto _ret;
            }
        }
    }
    mp = vfs_get_mount_point(&pcb->vfs, &fh->node);
    if (mp == NULL) {
        deprintf("mount point not found:%s", path);
        fd = -1;
        goto _ret;
    }

    fh->present = 1;
    fh->flags = flags;
    fh->mp = mp;
    fh->node.offset = 0;
    if (mp->fs->open && mp->fs->open(fh)) {
        deprintf("fs's open callback failed.");
        goto _ret;
    }
    dprintf("proc %x open %s fd:%x", pid, path, fd);
    _ret:
    kfree(path);
    return fd;
}

void kclose_all(uint32_t pid) {
    pcb_t *pcb = getpcb(pid);
    for (int8_t x = 0; x < MAX_FILE_HANDLES; x++) {
        if (pcb->fh[x].present) {
            pcb->fh[x].present = 0;
            mount_point_t *mp = pcb->fh[x].mp;
            if (mp->fs->close && mp->fs->close(&pcb->fh[x])) {
                deprintf("fs's close callback failed.");
            }
        }
    }
}

int8_t sys_open(const char *name, int flags) {
    return kopen(getpid(), name, flags);
}

int8_t kclose(uint32_t pid, int8_t fd) {
    pcb_t *pcb = getpcb(pid);
    file_handle_t *fh = &pcb->fh[fd];
    if (!fh->present) {
        dwprintf("cannot close a closed file:%x", fd);
        return -1;
    }
    mount_point_t *mp = fh->mp;
    if (mp->fs->close && mp->fs->close(fh)) {
        deprintf("fs's close callback failed.");
        return -1;
    }
    fh->present = 0;
    fh->mp = NULL;
    return 0;
}

int8_t sys_close(int8_t fd) {
    return kclose(getpid(), fd);
}

int32_t kread(uint32_t pid, int8_t fd, uchar_t *buff, int32_t size) {
    pcb_t *pcb = getpcb(pid);
    file_handle_t *fh = &pcb->fh[fd];
    if (!fh->present) {
        dwprintf("fd %x not present.", fd);
        return -1;
    }
    if (fh->mp == 0) {
        dwprintf("[%x]mount point not found.", fd);
        return -1;
    }
    if (fh->mp->fs->read == 0) {
        dwprintf("fs operator is not implemented.");
        return -1;
    }
    int32_t ret = fh->mp->fs->read(&fh->node, fh->mp->fsp, size,
                                   buff);
    return ret;
}

int32_t sys_read(int8_t fd, uchar_t *buff, int32_t size) {
    pcb_t *pcb = getpcb(getpid());
    file_handle_t *fh = &pcb->fh[fd];
    if (!fh->present) {
        dwprintf("fd %x not present.", fd);
        return -1;
    }
    int32_t ret = kread(getpid(), fd, buff, size);
    //fh->node.offset += ret;
    return ret;
}

off_t ktell(uint32_t pid, int8_t fd) {
    pcb_t *pcb = getpcb(pid);
    file_handle_t *fh = &pcb->fh[fd];
    if (!fh->present) {
        dwprintf("fd %x not present.", fd);
        return -1;
    }
    if (fh->mp == 0) {
        dwprintf("[%x]mount point not found.", fd);
        return -1;
    }
    if (fh->mp->fs->tell == 0) {
        return fh->node.offset;
    }
    return fh->mp->fs->tell(&fh->node, fh->mp->fsp);
}

off_t sys_tell(int8_t fd) {
    return ktell(getpid(), fd);
}

off_t klseek(uint32_t pid, int8_t fd, off_t offset, int where) {
    pcb_t *pcb = getpcb(pid);
    file_handle_t *fh = &pcb->fh[fd];
    if (!fh->present) {
        dwprintf("fd %x not present.", fd);
        return -1;
    }
    if (fh->mp == 0) {
        dwprintf("[%x]mount point not found.", fd);
        return -1;
    }
    uint32_t noffset = ktell(pid, fd);
    switch (where) {
        case SEEK_CUR:
            noffset += offset;
            break;
        case SEEK_END:
            noffset = fh->node.size + offset;
            break;
        case SEEK_SET:
            noffset = (uint32_t) offset;
            break;
        default:
            dwprintf("Bad where!");
            return -1;
    }
    if (fh->mp->fs->lseek == 0) {
        dwprintf("fs operator is not implemented.");
        return -1;
    }
    fh->mp->fs->lseek(&fh->node, fh->mp->fsp, noffset);
    return noffset;
}

off_t sys_lseek(int8_t fd, off_t offset, int where) {
    return klseek(getpid(), fd, offset, where);
}

int32_t kwrite(uint32_t pid, int8_t fd, uchar_t *buff, int32_t size) {
    pcb_t *pcb = getpcb(pid);
    file_handle_t *fh = &pcb->fh[fd];
    if (!fh->present) {
        dwprintf("fd %x not present.", fd);
        return -1;
    }
    if (fh->mp == 0) {
        dwprintf("[%x]mount point not found.", fd);
        return -1;
    }
    int32_t ret = fh->mp->fs->write(&fh->node, fh->mp->fsp, size,
                                    buff);

    return ret;
}

int32_t sys_write(int8_t fd, uchar_t *buff, int32_t size) {
    pcb_t *pcb = getpcb(getpid());
    file_handle_t *fh = &pcb->fh[fd];
    if (!fh->present) {
        dwprintf("fd %x not present.", fd);
        return -1;
    }
    int32_t ret = kwrite(getpid(), fd, buff, size);
    if (ret > 0)
        fh->node.offset += ret;
    return ret;
}

int sys_access(const char *name, int mode) {
    pcb_t *pcb = getpcb(getpid());
    char *path = (char *) kmalloc(MAX_PATH_LEN);
    vfs_fix_path(getpid(), name, path, MAX_PATH_LEN);
    int ret = -vfs_get_node(&pcb->vfs, path, NULL);
    kfree(path);
    return ret;
    //TODO type./..
}

int sys_stat(const char *name, stat_t *stat) {
    char *path = (char *) kmalloc(MAX_PATH_LEN);
    vfs_fix_path(getpid(), name, path, MAX_PATH_LEN);
    CHK(vfs_cd(&vfs, path), "No such file or directory.");
    fs_node_t *n = &vfs.current;
    stat->st_dev = 30;
    stat->st_ino = n->inode;
    stat->st_mode = n->mode;
    stat->st_nlink = 1;
    stat->st_uid = n->uid;
    stat->st_gid = n->gid;
    stat->st_rdev = 0;
    stat->st_size = n->size;
    stat->st_blksize = 0;
    stat->st_blocks = 0;
    stat->st_atime = 0;
    stat->st_mtime = 0;
    stat->st_ctime = 0;
    kfree(path);
    dprintf("stat done.");
    return 0;
    _err:
    kfree(path);
    return -1;
}

int sys_stat64(const char *name, stat64_t *stat) {
    char *path = (char *) kmalloc(MAX_PATH_LEN);
    vfs_fix_path(getpid(), name, path, MAX_PATH_LEN);
    CHK(vfs_cd(&vfs, path), "No such file or directory.");
    dprintf("debug:::st64:%x", stat);
    memset(stat, 0xFF, sizeof(stat64_t));
    fs_node_t *n = &vfs.current;
    stat->st_dev = 20;
    stat->st_ino = n->inode;
    stat->st_mode = n->mode;
    stat->st_nlink = 1;
    stat->st_uid = n->uid;
    stat->st_gid = n->gid;
    stat->st_rdev = 0;
    stat->st_size = n->size;
    stat->st_blksize = 0;
    stat->st_blocks = 0;
    stat->st_atime = 0;
    stat->st_mtime = 0;
    stat->st_ctime = 0;
    kfree(path);
    return 0;
    _err:
    kfree(path);
    return -1;
}

int sys_chdir(const char *name) {
    char *path = (char *) kmalloc(MAX_PATH_LEN);
    vfs_fix_path(getpid(), name, path, MAX_PATH_LEN);
    vfs_pretty_path(path, path);
    pcb_t *pcb = getpcb(getpid());
    int len = strlen(path);
    if (path[len - 1] != '/')
        strcat(path, "/");

    CHK(vfs_cd(&pcb->vfs, path), "");
    strcpy(pcb->dir, path);
    kfree(path);
    return 0;
    _err:
    kfree(path);
    return -1;
}

int kdup3(pid_t pid, int oldfd, int newfd, int flags) {
    pcb_t *pcb = getpcb(pid);
    if (!pcb->fh[oldfd].present) {
        dwprintf("fd %x not opened.", oldfd);
        return -1;
    }
    if (pcb->fh[newfd].present) {
        dwprintf("fd %x opened.", newfd);
        return -1;
    }
    memcpy(&pcb->fh[newfd], &pcb->fh[oldfd], sizeof(file_handle_t));
    pcb->fh[oldfd].present = 0;
    return 0;
}

int sys_dup3(int oldfd, int newfd, int flags) {
    return kdup3(getpid(), oldfd, newfd, flags);
}

inline bool isopenedfd(pid_t pid, int fd) {
    return getpcb(pid)->fh[fd].present;
}

char *kgetcwd(pid_t pid) {
    return getpcb(pid)->dir;
}

char *sys_getcwd(char *buff, int len) {
    pcb_t *pcb = getpcb(getpid());
    if (strlen(pcb->dir) > len)
        return NULL;
    strcpy(buff, pcb->dir);
    return buff;
}

int sys_ls(const char *path, dirent_t *dirents, uint32_t count) {
    CHK(vfs_cd(&vfs, path), "");
    return vfs_ls(&vfs, dirents, count);
    _err:
    return -1;
}

void mount_rootfs(uint32_t initrd) {
    vfs_init(&vfs);
    ext2_create_fstype();
    catmfs_create_fstype();
    tty_create_fstype();
    procfs_create_type();
    devfs_create_fstype();
    mutex_lock(&vfs_lock);
    CHK(vfs_mount(&vfs, "/", &catmfs, NULL), "");
    CHK(vfs_cd(&vfs, "/"), "");
    mount_point_t *mp;
    vfs_find_mount_point(&vfs, "/", &mp, NULL);
    CHK(catmfs_fs_node_load_catrfmt(&vfs.current_dir, mp->fsp, initrd), "");
    CHK(vfs_cd(&vfs, "/"), "");
    CHK(vfs_mkdir(&vfs, "data"), "");
    CHK(vfs_mount(&vfs, "/data", &ext2_fs, &disk1), "");
    CHK(vfs_mkdir(&vfs, "proc"), "");
    CHK(vfs_mount(&vfs, "/proc", &procfs, NULL), "");
    CHK(vfs_cd(&vfs, "/"), "");
    CHK(vfs_mkdir(&vfs, "dev"), "");
    CHK(vfs_mount(&vfs, "/dev", &devfs, NULL), "");
    CHK(tty_register_self(), "");
    CHK(devfile_register_self(), "");
    CHK(procfs_after_vfs_inited(), "");
    CHK(devfs_after_vfs_inited(), "");
    //symlink test
    CHK(vfs_cd(&vfs, "/"), "");
    CHK(vfs_symlink(&vfs, "link", "/data"), "");
    CHK(vfs_cd(&vfs, "/"), "");
    CHK(vfs_symlink(&vfs, "libc.so.6", "libc.so"), "");
    //end symlink test
    stdio_install();
    mutex_unlock(&vfs_lock);
    dprintf("rootfs mounted.");
    __vfs_ready = true;
    return;
    _err:
    PANIC("Failed to mount rootfs.");
}

void vfs_test() {
    ext2_create_fstype();
    vfs_init(&vfs);
    catmfs_create_fstype();
    vfs_mount(&vfs, "/", &ext2_fs, &dev);
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
    vfs_init(&vfs);
    catmfs_create_fstype();
    vfs_mount(&vfs, "/", &catmfs, &dev);
    putf("[vfs]ROOTFS(CATMFS) mount to /\n");
    //pause();
    vfs_cd(&vfs, "/");
    ASSERT(vfs_make(&vfs, FS_FILE, STR("tio")) == 0);
    vfs_cd(&vfs, "/tio");
    char neko[250];
    strcpy(neko, "HelloDCat!");
    int size = 0x40;
    char *bigneko = (char *) kmalloc_paging(size, NULL);
    putf_const("bigneko:%x\n", bigneko);
    memset(bigneko, 'H', size);
    int32_t a = vfs_write(&vfs, 10, bigneko);
    kfree(bigneko);
    size = 0x2032;
    bigneko = (char *) kmalloc_paging(size, NULL);
    putf_const("write[1]ret:%x\n", a);
    memset(bigneko, 'P', size);
    a = vfs_write(&vfs, 10, bigneko);
    putf_const("write[2]ret:%x\n", a);
    char *nya = (char *) kmalloc_paging(size, NULL);
    memset(nya, 0, 2032);
    int ret = vfs_read(&vfs, 10, nya);
    putnf("[%x][%d]read result:%s", 100, nya, ret, nya);

    ASSERT(vfs_make(&vfs, FS_FILE, STR("nemkoo")) == 0);
    ASSERT(vfs_make(&vfs, FS_FILE, STR("nemykoo")) == 0);
    ASSERT(vfs_make(&vfs, FS_FILE, STR("akami")) == 0);
    ASSERT(vfs_make(&vfs, FS_DIRECTORY, STR("neko")) == 0);
    vfs_cd(&vfs, "/neko");
    ASSERT(vfs_make(&vfs, FS_FILE, STR("akami2")) == 0);
    vfs_cd(&vfs, "/neko/akami2");
    ASSERT(vfs_rm(&vfs) == 0);
    vfs_cd(&vfs, "/neko");
    ASSERT(vfs_rm(&vfs) == 0);
    vfs_cd(&vfs, "/");
    int count = vfs_ls(&vfs, dirs, 20);
    putf("[vfs]there are %x files in / [%x]\n", count, vfs.current.inode);
    for (int x = 0; x < count; x++) {
        putf("[file]%s type:%x inode:%x\n", dirs[x].name, dirs[x].type, dirs[x].node);
    }
    ASSERT(vfs_make(&vfs, FS_DIRECTORY, STR("neko")) == 0);
    vfs_cd(&vfs, "/neko/");
    ASSERT(vfs_make(&vfs, FS_FILE, STR("s1")) == 0);
    ASSERT(vfs_make(&vfs, FS_DIRECTORY, STR("ext2")) == 0);
    count = vfs_ls(&vfs, dirs, 20);
    putf("[vfs]there are %x files in /neko [%x]\n", count, vfs.current.inode);
    for (int x = 0; x < count; x++) {
        putf("[file]%s type:%x inode:%x\n", dirs[x].name, dirs[x].type, dirs[x].node);
    }
    //pause();
    vfs_mount(&vfs, "/neko/ext2/", &ext2_fs, &dev);
    putf("[vfs]ATA01(EXT2) mount to /neko/ext2\n");
    vfs_cd(&vfs, "/neko/ext2/");
    //vfs_make(&vfs, FS_FILE, STR("NNNNN"));
    count = vfs_ls(&vfs, dirs, 20);
    putf("[vfs]there are %x files in /neko/ext2/ [%x]\n", count, vfs.current.inode);
    for (int x = 0; x < count; x++) {
        putf("[file]%s type:%x inode:%x\n", dirs[x].name, dirs[x].type, dirs[x].node);
    }
    //pause();
    vfs_cd(&vfs, "/neko/ext2/neko/");
    //vfs_make(&vfs, FS_FILE, STR("NNNNN"));
    count = vfs_ls(&vfs, dirs, 20);
    putf("[vfs]there are %x files in /neko/ext2/neko/ [%x]\n", count, vfs.current.inode);
    for (int x = 0; x < count; x++) {
        putf("[file]%s type:%x inode:%x\n", dirs[x].name, dirs[x].type, dirs[x].node);
    }
    //pause();
    if (vfs_touch(&vfs, "ssr-v2")) {
        putf("touch failed.\n");
    }
    vfs_mkdir(&vfs, "ssvr");
    vfs_cd(&vfs, "/neko/ext2/neko/ssvr/");
    vfs_touch(&vfs, "a little cat sleeping here.");

}