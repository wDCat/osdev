//
// Created by dcat on 7/7/17.
//

#include <str.h>
#include <proc.h>
#include <syscall_handler.h>
#include <proc.h>
#include "../memory/include/kmalloc.h"
#include <exec.h>
#include <schedule.h>
#include <timer.h>
#include <procfs.h>
#include <contious_heap.h>
#include <page.h>
#include <dynlibs.h>

extern void _gdt_flush();

extern void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran);

mutex_lock_t pcbs_lck;
pid_t pid = 0;
pid_t current_pid = 0;
pcb_t processes[MAX_PROC_COUNT];
uint32_t proc_count = 1;


pid_t getpid() {
    return current_pid;
}

/**For Debug...*/
void setpid(pid_t pid) {
    current_pid = pid;
}

pcb_t *getpcb(pid_t pid) {
    ASSERT(pid >= 0);
    return &processes[pid];
}

void create_first_proc() {
    pcb_t *init_pcb = getpcb(1);
    init_pcb->present = true;
    init_pcb->status = STATUS_RUN;
    init_pcb->pid = 1;
    init_pcb->page_dir = kernel_dir;
    init_pcb->dynlibs_end_addr = 0xA0000000;
    strcpy(init_pcb->dir, "/");

    extern tss_entry_t tss_entry;
    memcpy(&init_pcb->tss, &tss_entry, sizeof(tss_entry_t));
    //init_pcb->tss = &tss_entry;
    current_pid = 1;
}

void idle() {
    dprintf("Hello!I'm idle proc.");
    for (;;)
        delay(0xFFF);
}

void create_idle_proc() {
    pcb_t *pcb = getpcb(0);
    if (pcb->reserved_page)
        kfree(pcb->reserved_page);
    pcb->present = true;
    pcb->status = STATUS_READY;
    pcb->pid = 0;
    pcb->page_dir = kernel_dir;
    pcb->reserved_page = kmalloc_paging(0x1000, NULL);
    tss_entry_t *tss = &pcb->tss;
    tss->cs = 1 << 3 | 0;
    tss->es =
    tss->ss =
    tss->ds =
    tss->fs =
    tss->gs = 2 << 3 | 0;
    tss->cr3 = pcb->page_dir->physical_addr;
    tss->ebp = tss->esp = pcb->reserved_page + 0xFF0;
    tss->eip = (uint32_t) idle;
}

void proc_install() {
    mutex_init(&pcbs_lck);
    memset(processes, 0, sizeof(pcb_t) * MAX_PROC_COUNT);
    proc_avali_queue = (proc_queue_t *) kmalloc(sizeof(proc_queue_t));
    memset(proc_avali_queue, 0, sizeof(proc_queue_t));
    proc_ready_queue = (proc_queue_t *) kmalloc(sizeof(proc_queue_t));
    memset(proc_ready_queue, 0, sizeof(proc_queue_t));
    proc_wait_queue = (proc_queue_t *) kmalloc(sizeof(proc_queue_t));
    memset(proc_wait_queue, 0, sizeof(proc_queue_t));
    proc_died_queue = (proc_queue_t *) kmalloc(sizeof(proc_queue_t));
    memset(proc_died_queue, 0, sizeof(proc_queue_t));
    create_idle_proc();
    create_first_proc();
}


pid_t find_free_pid() {
    for (pid_t x = 0; x < MAX_PROC_COUNT; x++) {
        if (!processes[x].present)return x;
    }
    return 0;//pid 0 is for idle.
}


void create_user_stack(pcb_t *pcb, uint32_t end_addr, uint32_t size, uint32_t *new_ebp, uint32_t *new_esp,
                       page_directory_t *dir) {
    for (uint32_t p = end_addr - size; p < end_addr; p += 0x1000) {
        alloc_frame(get_page(p, true, dir), false, true);
        page_typeinfo_t *info = get_page_type(p, pcb->page_dir);
        info->pid = pcb->pid;
        info->free_on_proc_exit = true;
        info->copy_on_fork = true;
    }
    if (new_ebp)
        *new_ebp = end_addr - 0x10;
    if (new_esp)
        *new_esp = end_addr - 0x10;
}

inline void save_cur_proc_reg(regs_t *r) {
    return;
    /*
    if (getpid() >= 2) {
        pcb_t *pcb = getpcb(getpid());
        pcb->lastreg = r;
    }*/
}

inline void save_proc_state(pcb_t *pcb, regs_t *r) {
    //no necessary to save it...
    return;
    /*
    tss_entry_t *tss = &pcb->tss;
    tss->eax = r->eax;
    tss->ebx = r->ebx;
    tss->ecx = r->ecx;
    tss->edx = r->edx;
    if (pcb->pid == 0)
        tss->eip = (uint32_t) idle;
    else
        tss->eip = r->eip;
    tss->ebp = r->ebp;
    tss->esp = r->useresp;
    tss->esi = r->esi;
    tss->edi = r->edi;
    tss->ss = r->ss;
    tss->es = r->es;
    tss->cs = r->cs;
    tss->ds = r->ds;
    tss->fs = r->fs;
    tss->gs = r->gs;
    tss->eflags = r->eflags;*/
}

proc_queue_t *find_pqueue_by_status(proc_status_t status) {
    switch (status) {
        case STATUS_DIED:
            return proc_died_queue;
        case STATUS_READY:
            return proc_ready_queue;
        case STATUS_WAIT:
            return proc_wait_queue;
        default:
            return 0;
    }
}


void set_proc_status(pcb_t *pcb, proc_status_t new_status) {
    if (pcb < 0x100) PANIC("so silly the cat!");
    int ints;
    scli(&ints);
    if (pcb->status == new_status || pcb->pid <= 1) {
        srestorei(&ints);
        return;
    }
    dprintf("Proc %x status %x ==> %x", pcb->pid, pcb->status, new_status);
    proc_queue_t *old = find_pqueue_by_status(pcb->status);
    proc_queue_t *ns = find_pqueue_by_status(new_status);
    if (old) {
        CHK(proc_queue_remove(old, pcb), "");
    }
    if (ns) {
        CHK(proc_queue_insert(ns, pcb), "");
    }
    pcb->status = new_status;
    srestorei(&ints);
    return;
    _err:
    PANIC("error happened when set proc status!");
}

inline void set_active_user_tss(tss_entry_t *tss) {
    uint32_t base = (uint32_t) tss;
    uint32_t limit = base + sizeof(tss_entry_t);
    gdt_set_gate(TSS_USER_PROC_ID, base, limit, 0x89, 0x00);
}

inline void set_active_user_ldt(ldt_limit_entry_t *ldt_table, uint8_t ldt_table_count) {
    uint32_t base = (uint32_t) ldt_table;
    uint32_t limit = base + sizeof(ldt_limit_entry_t) * ldt_table_count;
    gdt_set_gate(LDT_USER_PROC_ID, base, limit, 0x82, 0x00);
}

void switch_to_proc_hard(pcb_t *pcb) {
    uint32_t old_pid = current_pid;
    set_active_user_tss(&pcb->tss);
    //set_active_user_ldt(pcb->ldt_table, pcb->ldt_table_count);
    _gdt_flush();
    dprintf("switch(hard) to task %x[PC:%x][ESP:%x]", pcb->pid, pcb->tss.eip, pcb->tss.esp);
    struct {
        uint32_t a;
        uint32_t b;
    } lj;
    lj.a = 0;
    pcb->rejmp = false;
    //after ljmp the cpu save the tss?
    __asm__ __volatile__("movw %%dx,%1;"
            "ljmp %0;"::"m"(*&lj.a), "m"(*&lj.b), "d"(TSS_USER_PROC_ID << 3));
    //proc which call this function will be paused at here.. eip=ret;
    ret:
    dprintf("welcome back.proc %x", old_pid);
}

void switch_to_proc_soft(pcb_t *pcb) {
    TODO;
    dprintf("switch(soft) to task %x at %x lastreg:%x pd:%x", pcb->pid, pcb, pcb->lastreg, pcb->page_dir);
    uint32_t a;
    pcb->rejmp = false;
    __asm__ __volatile__("int $0x61;":"=a" (a) : "b" (pcb));
}

void switch_to_proc(pcb_t *pcb) {
    ASSERT(pcb->present && (pcb->pid > 1 || pcb->pid == 0));
    int ints;
    scli(&ints);
    if (pcb->pid == current_pid) {
        if (!pcb->rejmp) {
            ssti(&ints);
            return;
        }
        dprintf("proc %x rejmp~", pcb->pid);
    }
    pcb_t *old_pcb = getpcb(getpid());
    if (pcb->pid > MAX_PROC_COUNT) PANIC("Bad PID:%x pcb:%x", pcb->pid, pcb);
    set_proc_status(pcb, STATUS_RUN);
    current_pid = pcb->pid;
    current_dir = pcb->page_dir;
    switch_to_proc_hard(pcb);
}

void create_ldt(pcb_t *pcb) {
    pcb->ldt_table_count = 1;
    pcb->ldt_table[0].base = 0x0;
    pcb->ldt_table[0].limit = 0xFFFFFFF;
}

void clean_pcb_table() {
    dprintf("PCB Table clean routine...");
    uint32_t procc = proc_count;
    for (pid_t x = 2; x < procc; x++) {
        pcb_t *pcb = getpcb(x);
        if (get_proc_status(pcb) == STATUS_DIED && pcb->hold_proc == 0) {
            if (pcb->fpcb) {
                ASSERT(pcb->fpcb->cpcb);
                pcb_t *pp = pcb->fpcb->cpcb;
                if (pp->pid == pcb->pid) {
                    pcb->fpcb->cpcb = pcb->next_pcb;
                } else {
                    bool found = false;
                    while (pp->next_pcb != 0) {
                        if (pp->next_pcb->pid == pcb->pid) {
                            pp->next_pcb = pcb->next_pcb;
                            found = true;
                            break;
                        }
                    }
                    if (!found) PANIC("proc %s ix %x 's child,but it not exist in it's father's cpcb");

                }
            }
            memset(pcb, 0, sizeof(pcb_t));
            if (procfs_remove_proc(x)) {
                PANIC("Error happened when remove %d from /proc", x);
            }
            proc_count--;
        }
    }

    dprintf("remove %x trash proc.", procc - proc_count);
}

int create_user_heap(pcb_t *pcb, uint32_t start, uint32_t size) {
    ASSERT(pcb->heap_ready == false);
    pcb->heap_ready = true;
    for (uint32_t x = start; x <= start + size; x += PAGE_SIZE) {
        alloc_frame(get_page(x, true, pcb->page_dir), false, true);
        page_typeinfo_t *info = get_page_type(x, pcb->page_dir);
        info->pid = pcb->pid;
        info->free_on_proc_exit = true;
        info->copy_on_fork = true;
    }
    create_heap(&pcb->heap, start, start + size, start + size);
    return 0;
}

int destory_user_heap(pcb_t *pcb) {
    if (pcb->heap_ready) {
        pcb->heap_ready = false;
        heap_t *heap = &pcb->heap;
        for (uint32_t x = heap->start_addr; x <= heap->end_addr; x += PAGE_SIZE) {
            dprintf("free frame:%x %x", x, get_page(x, false, pcb->page_dir)->frame);
            free_frame(get_page(x, false, pcb->page_dir));
        }
        destory_heap(&pcb->heap);
    }
    return 0;
}

pid_t fork(regs_t *r) {
    cli();
    if (proc_count % 8 == 0) {
        clean_pcb_table();
    }
    pid_t fpid = getpid();
    pid_t cpid;
    cpid = find_free_pid();
    extern tss_entry_t tss_entry;
    if (cpid == 0) PANIC("Out of PCB///");
    pcb_t *fpcb = getpcb(fpid);
    pcb_t *cpcb = getpcb(cpid);
    dprintf("%x try to fork.child %x PC:%x", fpid, cpid, fpcb->tss.eip);
    mutex_init(&cpcb->lock);
    proc_count++;
    cpcb->present = true;
    cpcb->pid = cpid;
    cpcb->page_dir = clone_page_directory(fpcb->page_dir);
    cpcb->reserved_page = (uint32_t) (kmalloc_paging(0x1000, NULL));
    memset(cpcb->reserved_page, 0, 0x1000);
    cpcb->spages = (struct spage_info *) cpcb->reserved_page;
    if (fpid > 1 && dynlibs_clone_tree(fpid, cpid)) {
        PANIC("cannot clone dynlibs tree");
    }
    dprintf("new dynlibs tree:%x", cpcb->dynlibs);
    cpcb->dynlibs_end_addr = fpcb->dynlibs_end_addr;
    //Copy open file table...
    memcpy(cpcb->fh, fpcb->fh, sizeof(fpcb->fh));
    memset(cpcb->spages, 0, 0x1000);
    tss_entry_t *tss = &cpcb->tss;
    memcpy(tss, &fpcb->tss, sizeof(tss_entry_t));
    tss->cr3 = cpcb->page_dir->physical_addr;
    tss->eax = 0;
    tss->ebx = r->ebx;
    tss->ecx = r->ecx;
    tss->edx = r->edx;
    tss->esi = r->esi;
    tss->edi = r->edi;
    if (fpid == 1) {
        create_user_stack(cpcb, UM_STACK_START, UM_STACK_SIZE, &tss->ebp, &tss->esp, cpcb->page_dir);
        //create_user_heap(cpcb);
        dprintf("new user stack:[%x][%x]", tss->ebp, tss->esp);
        strcpy(cpcb->dir, "/");
        char neko[256], neko2[256], neko5[256];
        strcpy(neko, "Hello");
        strcpy(neko2, "DCat");
        strcpy(neko5, "DCat~~~");
        char *args[3] = {neko, neko2, neko5};
        char neko3[256], neko4[256];
        strcpy(neko3, "PATH=/:/bin");
        strcpy(neko4, "COLOR=black");
        char *envp[3] = {neko3, neko4, NULL};
        kexec(cpid, "/init", 3, args, envp);
        //extern int little_test2();
        //tss->eip = (uint32_t) little_test2;
        tss->cs = 3 << 3 | 3;
        tss->es =
        tss->ss =
        tss->ds =
        tss->fs = 4 << 3 | 3;
        tss->gs = GDT_TLS_ID << 3 | 3;
    } else {
        for (uint32_t x = UM_STACK_START; x >= UM_STACK_START - UM_STACK_SIZE; x -= 0x1000) {
            page_typeinfo_t *tinfo = get_page_type(x, cpcb->page_dir);
            tinfo->pid = cpcb->pid;
        }
        tss->eip = r->eip;
        tss->ebp = r->ebp;
        tss->esp = r->useresp;
        tss->ss = r->ss;
        tss->es = r->es;
        tss->cs = r->cs;
        tss->ds = r->ds;
        tss->fs = r->fs;
        tss->gs = r->gs;
    }
    if (fpcb->heap_ready) {
        cpcb->heap_ready = true;
        clone_heap(&fpcb->heap, &cpcb->heap);
    }
    dprintf("proc cs[%x] es[%x]", r->cs, r->es);
    strcpy(cpcb->dir, fpcb->dir);
    tss->eflags = r->eflags | 0x200;
    tss->ss0 = 0x10;
    /*
    for (uint32_t s = 0; s <= UM_KSTACK_SIZE; s++) {
        alloc_frame(get_page(UM_KSTACK_START + s, true, cpcb->page_dir), false, true);
    }*/
    tss->esp0 = (uint32_t) cpcb->reserved_page + 0xFF0;
    tss->ldt = 0;
    cpcb->fpcb = fpcb;
    if (fpcb->cpcb) {
        pcb_t *pp = fpcb->cpcb;
        while (true) {
            if (pp->next_pcb == NULL) {
                pp->next_pcb = cpcb;
                break;
            } else pp = pp->next_pcb;
        }
    } else fpcb->cpcb = cpcb;
    procfs_insert_proc(cpid);
    set_proc_status(fpcb, STATUS_READY);
    switch_to_proc(cpcb);
    return cpid;
}

inline uint32_t *get_errno_location(pcb_t *pcb) {
    return pcb->tls;
}

void
copy_current_stack(uint32_t start_addr, uint32_t size, uint32_t *new_ebp, uint32_t *new_esp, uint32_t start_esp,
                   page_directory_t *dir) {
    ASSERT(current_dir != NULL);
    dprintf("creating user stack:%x %x", start_addr, size);
    uint32_t old_stack_pointer;
    uint32_t old_base_pointer;
    __asm__ __volatile__("mov %%esp, %0" : "=r" (old_stack_pointer));
    __asm__ __volatile__("mov %%ebp, %0" : "=r" (old_base_pointer));
    for (int x = size; x >= 0; x -= 0x1000) {
        page_t *page = get_page(start_addr - x, true, dir);//栈是高地址到低地址增长的
        ASSERT(page);
        dprintf("[%x]page addr:%x dir:%x", start_addr - x, page, dir);
        alloc_frame(page, false, true);
    }
    flush_TLB();
    int32_t offset = (uint32_t) start_addr - start_esp;
    dprintf("A[%x][%x][%x]B", old_base_pointer, old_stack_pointer, offset);
    uint32_t new_stack_pointer = old_stack_pointer + offset;
    uint32_t new_base_pointer = old_base_pointer + offset;
    dprintf("copy %x to %x size:%x", old_stack_pointer, new_stack_pointer, start_esp - old_stack_pointer);
    uint32_t *p = new_stack_pointer;
    memcpy(new_stack_pointer, old_stack_pointer, start_esp - old_stack_pointer);
    dprintf("copy done.");

    //FIXME May crash some stack entries....
    for (int i = (uint32_t) start_addr; i > (uint32_t) start_addr - size; i -= 4) {
        uint32_t tmp = *(uint32_t *) i;
        // If the value of tmp is inside the range of the old stack, assume it is a base pointer
        // and remap it. This will unfortunately remap ANY value in this range, whether they are
        // base pointers or not.
        if ((old_stack_pointer < tmp) && (tmp < start_esp)) {
            tmp = tmp + offset;
            uint32_t *tmp2 = (uint32_t *) i;
            *tmp2 = tmp;
        }
    }
    if (new_ebp)
        *new_ebp = new_base_pointer;
    if (new_esp)
        *new_esp = new_stack_pointer;
    dprintf("all done.now ret.");
}