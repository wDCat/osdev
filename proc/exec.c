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
#include <fcntl.h>
#include <open.h>
#include "exec.h"

#define push_stack(esp, val)({\
    *(esp)=val;\
    (esp)-=1;\
})
#define push_aux(esp, key, value) ({\
    push_stack((esp),(value));\
    push_stack((esp),(key));\
})

int kexec(pid_t pid, const char *path, int argc, char *const argv[], char *const envp[]) {
    pid_t orig_pid = getpid();
    pcb_t *pcb = getpcb(pid);
    setpid(pid);
    page_directory_t *orig_pd = current_dir;
    int envc = 0;
    if (envp && envp[0] != NULL) {
        for (; envp[envc] != NULL; envc++);
        envc++;
    }
    dprintf("%x try to exec %s argc:%x envc:%x", pid, path, argc, envc);
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
    if (fd < 3) {
        //not cover stdio
        for (int x = 3; x < 10; x++) {
            if (!isopenedfd(pid, x)) {
                if (kdup3(pid, fd, x, NULL)) {
                    deprintf("dup failed %d -> %d", fd, x);
                    goto _err;
                }
                fd = x;
                break;
            }
        }
        if (fd < 3) {
            deprintf("too many opened files");
            goto _err;
        }
    }
    if (pcb->heap_ready)
        CHK(destory_user_heap(pcb), "fail to release old heap..");

    if (pcb->edg) {
        CHK(elsp_free_edg(pcb->edg), "fail to release old edg");
        kfree(pcb->edg);
        pcb->edg = NULL;
    }
    if (pcb->dynlibs) {
        dprintf("try to unload");
        CHK(dynlibs_unload_all(pcb->pid), "fail to unload old dynlibs");;
    }
    bool musl_libc = false;
    elf_digested_t *edg = (elf_digested_t *) kmalloc(sizeof(elf_digested_t));
    memset(edg, 0, sizeof(elf_digested_t));
    pcb->edg = edg;
    CHK(elsp_init_edg(edg, pid, fd), "");
    edg->global_offset = 0x8000000;
    CHK(elsp_load_header(edg), "");
    CHK(elsp_load_sections(edg), "");
    dprintf("elf end addr:%x", edg->elf_end_addr);
    uint32_t heap_start = edg->elf_end_addr + (PAGE_SIZE - (edg->elf_end_addr % PAGE_SIZE)) + PAGE_SIZE * 2;
    CHK(create_user_heap(pcb, heap_start, 0x4000), "");
    pcb->program_break = heap_start + 0x4000;
    CHK(elsp_load_need_dynlibs(edg), "");
    for (int x = 0; x < MAX_FILE_HANDLES; x++) {
        if (pcb->fh[x].present && (pcb->fh[x].flags & FD_CLOEXEC)) {
            dprintf("close on exec: fd:%d", x);
            kclose(pid, x);
        }
    }
    uint32_t sym_start;
    uint32_t sym_main;
    uint32_t eip;
    if (dynlibs_find_symbol(pcb->pid, "_start", &sym_start)) {//old libdcat.so
        if (elsp_find_symbol(pcb->edg, "_start", &sym_start)) {
            eip = elsp_get_entry(edg);
            dprintf("redirect eip to elf_entry:%x", eip);
        } else {
            musl_libc = true;
            sym_start += pcb->edg->global_offset;
            if (elsp_find_symbol(pcb->edg, "main", &sym_main)) {
                dwprintf("main not found!");
                sym_main = 0x0;
            }
            sym_main += pcb->edg->global_offset;
            dprintf("redirect eip to _start:%x main:%x", sym_start, sym_main);
            eip = sym_start;
        }
    } else {
        dprintf("redirect eip to libdcat._start:%x", sym_start);
        eip = sym_start;
    }
    sym_main += pcb->edg->global_offset;
    pcb->tss.eip = eip;
    uint32_t *esp = (uint32_t *) (UM_STACK_START - 0x10 * sizeof(uint32_t));
    dprintf("elf load done.new PC:%x", eip);
    uint8_t *espptr = (uint8_t *) esp;
    strcpy(pcb->cmdline, path);
    if (argv) {
        //push **argv
        for (int x = argc - 1; x >= 0; x--) {
            dprintf("arg:%x", targv[x]);
            char *str = targv[x];
            int len = strlen(str) + 1;
            espptr -= len;
            espptr -= (uint32_t) espptr % 4;
            targv[x] = (char *) espptr;
            strcat(pcb->cmdline, " ");
            strcat(pcb->cmdline, targv[argc - x - 1]);
            dprintf("copy str %s to %x", str, espptr);
            strcpy(espptr, str);
        }
    }
    espptr -= 4;
    esp = (uint32_t *) espptr;
    uint32_t __envp = NULL, __argvp = NULL;
    uint32_t __env_rs = 0;
    if (tenvp && tenvp[0] != NULL) {
        if (musl_libc) {
            uint32_t envpp[envc];
            memset(envpp, 0, sizeof(uint32_t) * envc);
            for (int x = 0; tenvp[x] != NULL && x < envc; x++) {
                char *str = tenvp[x];
                int len = strlen(str) + 1;
                espptr -= len;
                espptr -= (uint32_t) espptr % 4;
                strcpy((char *) espptr, str);
                envpp[x] = (uint32_t) espptr;
                dprintf("env:%s %x %x", tenvp[x], espptr, esp);
            }
            espptr -= 4;
            esp = (uint32_t *) espptr;
            push_aux(esp, NULL, NULL);
            push_aux(esp, AT_HWCAP, 0);
            push_aux(esp, AT_SYSINFO, 0);
            push_aux(esp, AT_PAGESZ, PAGE_SIZE);
            push_stack(esp, NULL);
            for (int x = 0; envpp[x] != NULL && x < envc; x++) {
                dprintf("push envp %x to %x", envpp[x], esp);
                push_stack(esp, envpp[x]);
            }
        } else {
            envc = 0;
            for (; tenvp[envc] != NULL; envc++);
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
    }
    if (argv) {
        push_stack(esp, NULL);
        //push *argv
        for (int x = 0; x < argc; x++) {
            dprintf("push arg %x to %x", targv[argc - x - 1], esp);
            push_stack(esp, targv[argc - x - 1]);
        }
        __argvp = (uint32_t) (esp + 1);
        //esp -= 1;
    }

    if (musl_libc) {
        *esp = (uint32_t) argc;//p
        dprintf("push argc:%x to %x", argc, esp);
    } else {
        /**
         * -------------------
         * TLS Pointer
         * -------------------
         * nouse
         * -------------------
         * *argv ~
         * -------------------
         * envp
         * -------------------
         * argv
         * -------------------
         * argc
         * -------------------
         *  <--esp
         * */
        //push envp_reserved
        push_stack(esp, __env_rs);
        push_stack(esp, NULL);
        push_stack(esp, __envp);//push envp
        push_stack(esp, __argvp);//push argv
        push_stack(esp, argc);//push argc
    }
    dprintf("push done.esp:%x", esp);
    pcb->tss.esp = (uint32_t) esp;
    uint32_t *tls = (uint32_t *) (UM_STACK_START - sizeof(uint32_t));
    *tls = (uint32_t) umalloc(pid, sizeof(uint32_t) * 0xA);
    pcb->tls = tls;
    if (current_dir != orig_pd) {
        dprintf("switch back pd:%x", orig_pd);
        switch_page_directory(orig_pd);
    }
    strcpy(pcb->name, path);
    dprintf("kexec done.");
    setpid(orig_pid);
    if (args_tspace)
        kfree(args_tspace);
    proc_rejmp(pcb);
    return 0;
    _err:
    setpid(orig_pid);
    if (args_tspace)
        kfree(args_tspace);
    if (current_dir != orig_pd && orig_pd != 0) {
        dprintf("switch back pd:%x", orig_pd);
        switch_page_directory(orig_pd);
    }
    dprintf("kexec failed.");
    return 1;
}

//Unix like
int sys_execve(const char *path, char *const argv[], char *const envp[]) {
    int argc = 0;
    for (; argv[argc] != NULL; argc++);
    return kexec(getpid(), path, argc, argv, envp);
}

int sys_exec(const char *path, int argc, char *const argv[], char *const envp[]) {
    return kexec(getpid(), path, argc, argv, envp);
}