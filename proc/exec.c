//
// Created by dcat on 8/31/17.
//

#include <proc.h>
#include <str.h>
#include <elfloader.h>
#include <schedule.h>
#include <isrs.h>
#include "exec.h"

int kexec(pid_t pid, const char *path, int argc, char **argv) {
    dprintf("%x try to exec %s", pid, path);
    pcb_t *pcb = getpcb(pid);
    if (!pcb->present) {
        deprintf("process %x not exist.", pid);
        goto _err;
    }
    int fd = kopen(pid, path, 0);
    if (fd < 0) {
        deprintf("cannot open elf:%s", path);
        goto _err;
    }
    page_directory_t *orig_pd = current_dir;
    if (current_dir != pcb->page_dir) {
        dprintf("switch to proc's pd:%x orig:%x", pcb->page_dir, orig_pd);
        switch_page_directory(pcb->page_dir);
    }
    uint32_t eip;
    if (elf_load(pid, fd, &eip)) {
        deprintf("error happened during elf load.");
        goto _err;
    }
    pcb->tss.eip = eip;
    uint8_t *espptr = (uint8_t *) pcb->tss.esp;
    uint32_t *esp;
    dprintf("elf load done.new PC:%x", eip);
    strcpy(pcb->cmdline, path);
    if (argv) {
        //push **argv
        for (int x = argc - 1; x >= 0; x--) {
            dprintf("arg:%x", argv[x]);
            char *str = argv[x];
            int len = strlen(str);
            espptr -= len;
            espptr -= (uint32_t) espptr % 4;
            argv[x] = (char *) espptr;
            strcat(pcb->cmdline, " ");
            strcat(pcb->cmdline, argv[argc - x - 1]);
            dprintf("copy str %s to %x", str, espptr);
            strcpy(espptr, str);

        }

        //push *argv
        esp = (uint32_t *) espptr - argc - 1;
        for (int x = 0; x < argc; x++) {
            esp += 1;
            *esp = (uint32_t) argv[x];
            dprintf("push arg %x to %x", *esp, esp);
        }
        esp -= argc;
    }
    //push argv
    *esp = (uint32_t) (esp + 1);
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


int sys_exec(const char *path, int argc, char **argv) {
    return kexec(getpid(), path, argc, argv);
}