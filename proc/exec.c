//
// Created by dcat on 8/31/17.
//

#include <proc.h>
#include <str.h>
#include <elfloader.h>
#include <schedule.h>
#include <isrs.h>
#include <umalloc.h>
#include "exec.h"

int kexec(pid_t pid, const char *path, int argc, char *const argv[], char *const envp[]) {
    dprintf("%x try to exec %s", pid, path);
    pcb_t *pcb = getpcb(pid);
    page_directory_t *orig_pd = current_dir;
    if (!pcb->present) {
        deprintf("process %x not exist.", pid);
        goto _err;
    }
    int fd = kopen(pid, path, 0);
    if (fd < 0) {
        deprintf("cannot open elf:%s", path);
        goto _err;
    }
    if (current_dir != pcb->page_dir) {
        dprintf("switch to proc's pd:%x orig:%x", pcb->page_dir, orig_pd);
        switch_page_directory(pcb->page_dir);
    }
    if (!pcb->heap_ready) {
        pcb->heap_ready = true;
        create_user_heap(pcb);
    }
    uint32_t eip;
    if (elf_load(pid, fd, &eip)) {
        deprintf("error happened during elf load.");
        goto _err;
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
            dprintf("arg:%x", argv[x]);
            char *str = argv[x];
            int len = strlen(str) + 1;
            espptr -= len;
            espptr -= (uint32_t) espptr % 4;
            argvp[x] = (char *) espptr;
            strcat(pcb->cmdline, " ");
            strcat(pcb->cmdline, argv[argc - x - 1]);
            dprintf("copy str %s to %x", str, espptr);
            strcpy(espptr, str);
        }
    }
    esp = ((uint32_t *) espptr) - 1;
    uint32_t __envp = NULL, __argvp = NULL;
    uint32_t __env_rs = 0;
    if (envp && envp[0] != NULL) {
        int envc = 0;
        for (; envp[envc] != NULL; envc++);
        envc++;
        __env_rs = envc * 2 * sizeof(uint32_t);
        uint32_t *uenvp = umalloc(pid, __env_rs);
        __envp = (uint32_t) uenvp;
        //push *envp
        for (int x = 0; envp[x] != NULL && x < 128; x++) {
            dprintf("env:%s %x %x", envp[x], uenvp, 0x2333);
            int len = strlen(envp[x]);
            uchar_t *cp = umalloc(pid, len + 1);
            strcpy(cp, envp[x]);
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
    proc_rejmp(pcb);
    return 0;
    _err:
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