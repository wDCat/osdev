//
// Created by dcat on 8/31/17.
//

#include <proc.h>
#include <str.h>
#include <elfloader.h>
#include <schedule.h>
#include "exec.h"

int kexec(pid_t pid, const char *path, int argc, ...) {
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
    dprintf("elf load done.new PC:%x", eip);
    uint32_t *esp = (uint32_t *) pcb->tss.esp - argc;
    va_list args;
    va_start(args, argc);
    for (int x = 0; x < argc; x++) {
        esp += 1;
        *esp = va_arg(args, uint32_t);
        dprintf("push arg %x to %x", *esp, esp);
    }
    esp -= argc;
    *esp = (uint32_t) argc;
    esp -= 1;
    pcb->tss.esp = (uint32_t) esp;
    if (current_dir != orig_pd) {
        dprintf("switch back pd:%x", orig_pd);
        switch_page_directory(orig_pd);
    }
    dprintf("kexec done.");
    if (getpid() != 1) {
        dprintf("debug loop");
        //LOOP();
    }
    do_schedule_rejmp(NULL);
    return 0;
    _err:
    return 1;
}

//FIXME args pass
int sys_exec(const char *path, int argc, ...) {
    return kexec(getpid(), path, argc);
}