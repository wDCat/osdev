//
// Created by dcat on 10/13/17.
//

#ifndef W2_DYNLIBS_H
#define W2_DYNLIBS_H

#include <system.h>
#include <proc_queue.h>

#define MAX_LOADED_LIBS 64
typedef struct {
    uint32_t start_addr;
    uint32_t frameno;
    bool dirty;
} dynlib_frame_t;
typedef struct {
    char *path;
    elf_digested_t edg;
    dynlib_frame_t *frames;
    proc_queue_t *used_proc;
} dynlib_t;
typedef struct dynlib_inctree {
    dynlib_t *lib;
    struct dynlib_inctree *need;//left sub tree
    struct dynlib_inctree *next;//right sub tree
} dynlib_inctree_t;

void dynlibs_install();

int dynlibs_load(pid_t pid, const char *path);

int dynlibs_add_to_tree(pid_t pid, dynlib_inctree_t *parent, dynlib_t *lib);

#endif //W2_DYNLIBS_H
