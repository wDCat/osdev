//
// Created by dcat on 8/31/17.
//

#include <proc.h>
#include <str.h>
#include <elfloader.h>
#include <schedule.h>
#include <isrs.h>
#include <umalloc.h>
#include <elf.h>
#include <kmalloc.h>
#include <dynlibs.h>
#include "exec.h"

int kexec(pid_t pid, const char *path, int argc, char *const argv[], char *const envp[]) {
    dprintf("%x try to exec %s", pid, path);
    pcb_t *pcb = getpcb(pid);
    page_directory_t *orig_pd = current_dir;
    int envc = 0;
    if (envp && envp[0] != NULL) {
        for (; envp[envc] != NULL; envc++);
        envc++;
    }
    char *targv[argc], *tenvp[envc];
    if (!pcb->present) {
        deprintf("process %x not exist.", pid);
        goto _err;
    }
    uint32_t args_tspace = kmalloc_paging(PAGE_SIZE, NULL);
    char *tsw = (char *) args_tspace;
    for (int x = 0; x < argc; x++) {
        if ((uint32_t) tsw - args_tspace > PAGE_SIZE) {
            deprintf("argv and envs bigger than 1 page.");
            goto _err;
        }
        strcpy(tsw, argv[x]);
        targv[x] = tsw;
        tsw = tsw + strlen(argv[x]) + 1;
    }
    for (int x = 0; x < envc; x++) {
        if ((uint32_t) tsw - args_tspace > PAGE_SIZE) {
            deprintf("argv and envs bigger than 1 page.");
            goto _err;
        }
        if (envp[x] == NULL) {
            tenvp[x] = NULL;
            break;
        }
        tenvp[x] = tsw;
        strcpy(tsw, envp[x]);
        tsw = tsw + strlen(envp[x]) + 1;
    }
    if (current_dir != pcb->page_dir) {
        dprintf("switch to proc's pd:%x orig:%x", pcb->page_dir, orig_pd);
        switch_page_directory(pcb->page_dir);
    }
    int fd = kopen(pid, path, 0);
    if (fd < 0) {
        deprintf("cannot open elf:%s", path);
        goto _err;
    }
    if (pcb->heap_ready)
        CHK(destory_user_heap(pcb), "fail to release old heap..");
    if (pcb->edg) {
        CHK(elsp_free_edg(pcb->edg), "fail to release old edg");
        kfree(pcb->edg);
        pcb->edg = NULL;
    }
    elf_digested_t *edg = (elf_digested_t *) kmalloc(sizeof(elf_digested_t));
    memset(edg, 0, sizeof(elf_digested_t));
    pcb->edg = edg;
    edg->pid = pid;
    edg->fd = (int8_t) fd;
    edg->global_offset = 0x8000000;
    CHK(elsp_load_header(edg), "");
    CHK(elsp_load_sections(edg), "");
    CHK(elsp_load_need_dynlibs(edg), "");
    CHK(elsp_free_edg(edg), "");
    dprintf("elf end addr:%x", edg->elf_end_addr);
    uint32_t heap_start = edg->elf_end_addr + (PAGE_SIZE - (edg->elf_end_addr % PAGE_SIZE)) + PAGE_SIZE * 2;
    CHK(create_user_heap(pcb, heap_start, 0x4000), "");
    uint32_t sym_start;
    uint32_t eip;
    if (dynlibs_find_symbol(pcb->pid, "_start", &sym_start)) {
        eip = elsp_get_entry(edg);
        dprintf("redirect eip to elf_entry:%x", eip);
    } else {
        dprintf("redirect eip to _start:%x", sym_start);
        eip = sym_start;
    }
    pcb->tss.eip = eip;
    uint32_t *esp = (uint32_t *) pcb->tss.esp;
    dprintf("elf load done.new PC:%x", eip);
    uint8_t *espptr = (uint8_t *) esp;
    strcpy(pcb->cmdline, path);
    char **argvp = (char **) argv;
    if (argv) {
        //push **argv
        for (int x = argc - 1; x >= 0; x--) {
            dprintf("arg:%x", targv[x]);
            char *str = targv[x];
            int len = strlen(str) + 1;
            espptr -= len;
            espptr -= (uint32_t) espptr % 4;
            argvp[x] = (char *) espptr;
            strcat(pcb->cmdline, " ");
            strcat(pcb->cmdline, targv[argc - x - 1]);
            dprintf("copy str %s to %x", str, espptr);
            strcpy(espptr, str);
        }
    }
    esp = ((uint32_t *) espptr) - 1;
    uint32_t __envp = NULL, __argvp = NULL;
    uint32_t __env_rs = 0;
    if (envp && envp[0] != NULL) {
        envc = 0;
        for (; envp[envc] != NULL; envc++);
        envc++;
        __env_rs = envc * 2 * sizeof(uint32_t);
        uint32_t *uenvp = umalloc(pid, __env_rs);
        __envp = (uint32_t) uenvp;
        //push *envp
        for (int x = 0; tenvp[x] != NULL && x < 128; x++) {
            dprintf("env:%s %x %x", tenvp[x], uenvp, 0x2333);
            int len = strlen(tenvp[x]);
            uchar_t *cp = umalloc(pid, len + 1);
            strcpy(cp, tenvp[x]);
            *uenvp = (uint32_t) cp;
            uenvp += 1;
        }
        *uenvp = NULL;
    }

    if (argv) {
        //push *argv
        esp = esp - argc - 1;
        for (int x = 0; x < argc; x++) {
            esp += 1;
            *esp = (uint32_t) argvp[x];
            dprintf("push arg %x to %x", *esp, esp);
        }
        esp -= argc;
        __argvp = (uint32_t) (esp + 1);
    }
    //push envp_reserved
    *esp = __env_rs;
    esp -= 1;
    //push envp
    *esp = __envp;
    esp -= 1;
    //push argv
    *esp = __argvp;
    esp -= 1;
    //push argc
    *esp = (uint32_t) argc;
    esp -= 1;
    pcb->tss.esp = (uint32_t) esp;

    if (current_dir != orig_pd) {
        dprintf("switch back pd:%x", orig_pd);
        switch_page_directory(orig_pd);
    }
    strcpy(pcb->name, path);
    dprintf("kexec done.");
    if (args_tspace)
        kfree(args_tspace);
    proc_rejmp(pcb);
    return 0;
    _err:
    if (args_tspace)
        kfree(args_tspace);
    if (current_dir != orig_pd && orig_pd != 0) {
        dprintf("switch back pd:%x", orig_pd);
        switch_page_directory(orig_pd);
    }
    dprintf("kexec failed.");
    return 1;
}


int sys_exec(const char *path, int argc, char *const argv[], char *const envp[]) {
    return kexec(getpid(), path, argc, argv, envp);
}