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

extern void _gdt_flush();

extern void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran);

pid_t pid = 0;
pid_t current_pid = 0;
pcb_t processes[MAX_PROC_COUNT];
uint32_t proc_count = 1;

proc_queue_t *proc_avali_queue,
        *proc_ready_queue,
        *proc_wait_queue,
        *proc_died_queue;

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
    strcpy(init_pcb->dir, "/");
    extern tss_entry_t tss_entry;
    memcpy(&init_pcb->tss, &tss_entry, sizeof(tss_entry_t));
    //init_pcb->tss = &tss_entry;
    current_pid = 1;
}

void idle() {
    dprintf("now idle.");
    for (;;) {
        extern void irq_remap();
        irq_remap();
        delay(5);
    }
}

void create_idle_proc() {
    pcb_t *pcb = getpcb(0);
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


pid_t find_free_pcb() {
    for (pid_t x = 0; x < MAX_PROC_COUNT; x++) {
        if (!processes[x].present)return x;
    }
    return 0;//pid 0 is for idle.
}


void create_user_stack(uint32_t end_addr, uint32_t size, uint32_t *new_ebp, uint32_t *new_esp, page_directory_t *dir) {
    for (uint32_t p = end_addr - size; p < end_addr; p += 0x1000) {
        alloc_frame(get_page(p, true, dir), false, true);
    }
    if (new_ebp)
        *new_ebp = end_addr - 0x10;
    if (new_esp)
        *new_esp = end_addr - 0x10;
}

inline void save_proc_state(pcb_t *pcb, regs_t *r) {
    //cpu does it...
    return;
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
    tss->eflags = r->eflags;
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

/*
 * use macro instead~
inline proc_status_t get_proc_status(pcb_t *pcb) {
    return pcb->status;
}
*/

void set_proc_status(pcb_t *pcb, proc_status_t new_status) {
    ASSERT(pcb > 0x100);//silly
    cli();
    if (pcb->status == new_status || pcb->pid <= 1)return;
    dprintf("Proc %x status %x ==> %x", pcb->pid, pcb->status, new_status);
    proc_queue_t *old = find_pqueue_by_status(pcb->status);
    proc_queue_t *ns = find_pqueue_by_status(new_status);
    uint32_t sc = 256;
    if (old) {
        proc_queue_entry_t *e = old->first;
        if (old->first->pcb == pcb) {
            old->count--;
            kfree(old->first);
            old->first = old->first->next;
        } else
            while (e && sc--)
                if (e->next->pcb == pcb) {
                    proc_queue_entry_t *oe = e->next;
                    e->next = e->next->next;
                    old->count--;
                    kfree(oe);
                    break;
                } else e = e->next;
    }
    if (ns) {
        if (ns->first == NULL) {
            proc_queue_entry_t *ne = (proc_queue_entry_t *) kmalloc(sizeof(proc_queue_entry_t));
            ne->pcb = pcb;
            ne->next = NULL;
            ns->first = ne;
            ns->count++;
            ASSERT(ns->count == 1);
        } else {
            proc_queue_entry_t *e = ns->first;
            while (e && sc--) {
                if (e->pcb == pcb)break;
                if (e->next == NULL) {
                    proc_queue_entry_t *ne = (proc_queue_entry_t *) kmalloc(sizeof(proc_queue_entry_t));
                    ne->pcb = pcb;
                    ne->next = NULL;
                    e->next = ne;
                    ns->count++;
                    break;
                }
                e = e->next;
            }
        }
    }
    if (sc == 0) PANIC("Unknown Exception");
    pcb->status = new_status;
    sti();
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

void switch_to_proc_soft(regs_t *r, pcb_t *pcb) {
    TODO;
}


void switch_to_proc(pcb_t *pcb) {
    ASSERT(pcb->present && (pcb->pid > 1 || pcb->pid == 0));
    cli();
    uint32_t old_pid = current_pid;
    if (pcb->pid == current_pid) {
        if (!pcb->rejmp) {
            return;
        }
        dprintf("proc %x rejmp~", pcb->pid);
    }

    pcb->rejmp = false;
    pcb_t *old_pcb = getpcb(getpid());
    if (pcb->pid > MAX_PROC_COUNT) PANIC("Bad PID:%x pcb:%x", pcb->pid, pcb);
    set_proc_status(pcb, STATUS_RUN);

    set_active_user_tss(&pcb->tss);
    //set_active_user_ldt(pcb->ldt_table, pcb->ldt_table_count);
    _gdt_flush();
    dprintf("switch to task %x[PC:%x][ESP:%x]", pcb->pid, pcb->tss.eip, pcb->tss.esp);
    current_pid = pcb->pid;
    current_dir = pcb->page_dir;
    //k_delay(1);//cause bug...
    struct {
        uint32_t a;
        uint32_t b;
    } lj;
    lj.a = 0;
    __asm__ __volatile__("movw %%dx,%1;"
            "ljmp %0;"::"m"(*&lj.a), "m"(*&lj.b), "d"(TSS_USER_PROC_ID << 3));
    dprintf("welcome back.proc %x", old_pid);
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

void create_user_heap(pcb_t *pcb) {
    for (uint32_t x = 0xD0000000; x < 0xD0001000; x++) {
        alloc_frame(get_page(x, true, pcb->page_dir), false, true);
    }
    create_heap(&pcb->heap, 0xD0000000, 0xD0001000, 0xD0001000);
}

pid_t fork(regs_t *r) {
    cli();
    if (proc_count % 5 == 0) {
        clean_pcb_table();
    }
    pid_t fpid = getpid();
    pid_t cpid;
    cpid = find_free_pcb();
    extern tss_entry_t tss_entry;
    if (cpid == 0) PANIC("Out of PCB///");
    dprintf("%x try to fork.child %x", fpid, cpid);
    pcb_t *fpcb = getpcb(fpid);
    pcb_t *cpcb = getpcb(cpid);
    proc_count++;
    cpcb->present = true;
    cpcb->pid = cpid;
    cpcb->page_dir = clone_page_directory(fpcb->page_dir);
    cpcb->reserved_page = (uint32_t) (kmalloc_paging(0x1000, NULL));
    memset(cpcb->reserved_page, 0, 0x1000);
    cpcb->spages = (struct spage_info *) cpcb->reserved_page;
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
        create_user_stack(0xCC000000, 0x4000, &tss->ebp, &tss->esp, cpcb->page_dir);
        //create_user_heap(cpcb);
        dprintf("new user stack:[%x][%x]", tss->ebp, tss->esp);
        char neko[256], neko2[256];
        strcpy(neko, "Hello");
        strcpy(neko2, "DCat");
        char *args[2] = {neko, neko2};
        char neko3[256], neko4[256];
        strcpy(neko3, "PATH=/");
        strcpy(neko4, "COLOR=black");
        char *envp[3] = {neko3, neko4, NULL};
        kexec(cpid, "/init", 2, args, envp);
        //extern int little_test2();
        //tss->eip = (uint32_t) little_test2;
        tss->cs = 3 << 3 | 3;
        tss->es =
        tss->ss =
        tss->ds =
        tss->fs =
        tss->gs = 4 << 3 | 3;
    } else {
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
    dprintf("proc cs[%x] es[%x]", r->cs, r->es);
    strcpy(cpcb->dir, fpcb->dir);
    tss->eflags = r->eflags | 0x200;
    tss->ss0 = 0x10;
    for (uint32_t s = 0; s < UM_KSTACK_SIZE; s++) {
        alloc_frame(get_page(UM_KSTACK_START + s, true, cpcb->page_dir), false, true);
    }
    tss->esp0 = (uint32_t) (UM_KSTACK_START + UM_KSTACK_SIZE - 0x4);
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