//
// Created by dcat on 7/7/17.
//

#include <str.h>
#include <proc.h>
#include "proc.h"

pid_t pid = 0;
pid_t current_pid = 0;
pcb_t processes[MAX_PROC_COUNT];
uint32_t proc_count = 1;

pid_t getpid() {
    return current_pid;
}

pcb_t *getpcb(pid_t pid) {
    ASSERT(pid > 0);
    return &processes[pid - 1];
}

void proc_install() {
    memset(processes, 0, sizeof(pcb_t) * MAX_PROC_COUNT);
    //kernel proc(name:init pid:1)
    pcb_t *init_pcb = getpcb(1);
    init_pcb->present = true;
    init_pcb->status = STATUS_RUN;
    init_pcb->pid = 1;
    init_pcb->page_dir = kernel_dir;
    current_pid = 1;
}

void switch_to_task(pcb_t *pcb) {
    ASSERT(pcb->present);
    switch_page_directory(pcb->page_dir);

}

pid_t find_free_pcb() {
    for (pid_t x = 0; x < MAX_PROC_COUNT; x++) {
        if (!processes[x].present)return x + 1;
    }
    return 0;//pid 0 is not available.
}

void neko() {
    /*
    for (;;);
    __asm__ __volatile__("mov $0x12,%eax;"
            "push %eax;"
            "pop %eax;");*/
    syscall_helloworld();
}

pid_t fork(regs_t *r) {
    uint32_t ebp, esp;
    __asm__ __volatile__("mov %%esp, %0" : "=r" (esp));
    __asm__ __volatile__("mov %%ebp, %0" : "=r" (ebp));
    putf_const("self addr:%x\n", fork);
    cli();
    pid_t fpid = getpid();
    pid_t cpid;
    cpid = find_free_pcb();
    extern tss_entry_t tss_entry;
    if (cpid == 0) PANIC("Out of PCB///");
    putf_const("%x try to fork.child %x\n", fpid, cpid);
    pcb_t *fpcb = getpcb(fpid);
    pcb_t *cpcb = getpcb(cpid);
    proc_count++;
    cpcb->present = true;
    cpcb->pid = cpid;
    cpcb->page_dir = clone_page_directory(fpcb->page_dir);
    //copy_current_stack(0xCCC00000, 0x2000, &cpcb->tss.ebp, &cpcb->tss.esp, r->ebp, cpcb->page_dir);
    //putf_const("stack cloned.");
    cpcb->status = STATUS_READY;
    tss_entry_t *tss = &cpcb->tss;
    memcpy(tss, &tss_entry, sizeof(tss_entry_t));
    tss->cr3 = current_dir->physical_addr;//cpcb->page_dir->physical_addr;
    tss->eax = 0;
    tss->ebx = r->ebx;
    tss->ecx = r->ecx;
    tss->edx = r->edx;
    tss->ebp = r->ebp;
    tss->esp = r->esp;
    tss->esi = r->esi;
    tss->edi = r->edi;
    tss->eip = r->eip;
    tss->ss = r->ss;
    tss->es = r->es;
    tss->cs = r->cs;
    tss->ds = r->ds;
    tss->fs = r->fs;
    tss->gs = r->gs;
    tss->eflags = r->eflags;
    tss->ss0 = 0x10;
    tss->esp0 = kmalloc_paging(0x1000, NULL) + 0x1000;
    tss->ldt = 0;
    write_tss(TSS_ID + 1, 0x10, 0);
    uint32_t base = (uint32_t) tss;
    uint32_t limit = base + sizeof(tss_entry);
    gdt_set_gate(TSS_ID + 1, base, limit, 0xE9, 0x00);
    _gdt_flush();
    sti();
    uint32_t ss = get_eip();
    //k_delay(6);
    putf_const("dump[%x][%x]", tss->esp, r->useresp);
    /*
    __asm__ __volatile__ (""
            "ltr %%ax"::"a"((TSS_ID + 1) << 3));
   */
    putf_const("tr flushed[%x].\n", tss->esp0);
    struct {
        uint32_t a;
        uint32_t b;
    } lj;
    __asm__ __volatile__("movw %%dx,%1;"
            "ljmp %0;"::"m"(*&lj.a), "m"(*&lj.b), "d"((TSS_ID + 1) << 3));
    return cpid;
}


void
copy_current_stack(uint32_t start_addr, uint32_t size, uint32_t *new_ebp, uint32_t *new_esp, uint32_t start_esp,
                   page_directory_t *dir) {
    ASSERT(current_dir != NULL);
    putf_const("creating user stack:%x %x\n", start_addr, size);
    uint32_t old_stack_pointer;
    uint32_t old_base_pointer;
    __asm__ __volatile__("mov %%esp, %0" : "=r" (old_stack_pointer));
    __asm__ __volatile__("mov %%ebp, %0" : "=r" (old_base_pointer));
    for (int x = size; x >= 0; x -= 0x1000) {
        page_t *page = get_page(start_addr - x, true, dir);//栈是高地址到低地址增长的
        ASSERT(page);
        putf_const("[%x]page addr:%x dir:%x\n", start_addr - x, page, dir)
        alloc_frame(page, false, true);
    }
    flush_TLB();
    int32_t offset = (uint32_t) start_addr - start_esp;
    putf_const("A[%x][%x][%d]B", old_base_pointer, old_stack_pointer, offset);
    uint32_t new_stack_pointer = old_stack_pointer + offset;
    uint32_t new_base_pointer = old_base_pointer + offset;
    putf_const("copy %x to %x size:%x\n", old_stack_pointer, new_stack_pointer, start_esp - old_stack_pointer);
    uint32_t *p = new_stack_pointer;
    memcpy(new_stack_pointer, old_stack_pointer, start_esp - old_stack_pointer);

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
}

void enter_ring3(uint32_t start_addr) {
    __asm__ __volatile__(
    "cli;"
            "mov $0x23,%ax;"
            "mov %ax,%ds;"
            "mov %ax,%es;"
            "mov %ax,%fs;"
            "mov %ax,%gs;"
            "mov %esp,%eax;"
            "push $0x23;"
            "push %eax;"
            "pushf;"
            "pop %eax;"
            "or $0x200,%eax;"/*自动开中断*/
            "pushl %eax;"
            "push $0x1B;"
            "push $1f;"
            "iret;"
            "1:");
    /*
    if (start_addr)
        ((void (*)()) start_addr)();*/

}