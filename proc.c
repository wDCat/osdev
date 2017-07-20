//
// Created by dcat on 7/7/17.
//

#include "proc.h"

void copy_current_stack(uint32_t start_addr, uint32_t size, uint32_t *new_ebp, uint32_t *new_esp) {
    ASSERT(current_dir != NULL);
    putf_const("creating user stack:%x %x\n", start_addr, size);
    for (int x = size; x >= 0; x -= 0x1000) {
        page_t *page = get_page(start_addr - x, true, current_dir);//栈是高地址到低地址增长的
        ASSERT(page);
        putf_const("[%x]page addr:%x dir:%x\n", start_addr - x, page, current_dir)
        alloc_frame(page, false, true);
        ASSERT(page->frame != NULL);
    }
    flush_TLB();
    uint32_t old_stack_pointer;
    uint32_t old_base_pointer;
    __asm__ __volatile__("mov %%esp, %0" : "=r" (old_stack_pointer));
    __asm__ __volatile__("mov %%ebp, %0" : "=r" (old_base_pointer));
    uint32_t offset = (uint32_t) start_addr - init_esp;
    uint32_t new_stack_pointer = old_stack_pointer + offset;
    uint32_t new_base_pointer = old_base_pointer + offset;
    putf_const("copy %x to %x size:%x\n", old_stack_pointer, new_stack_pointer, init_esp - old_stack_pointer);
    uint32_t *p = new_stack_pointer;
    memcpy(new_stack_pointer, old_stack_pointer, init_esp - old_stack_pointer);
    for (int i = (uint32_t) start_addr; i > (uint32_t) start_addr - size; i -= 4) {
        uint32_t tmp = *(uint32_t *) i;
        // If the value of tmp is inside the range of the old stack, assume it is a base pointer
        // and remap it. This will unfortunately remap ANY value in this range, whether they are
        // base pointers or not.
        if ((old_stack_pointer < tmp) && (tmp < init_esp)) {
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
    ((void (*)()) start_addr)();

}