//
// Created by dcat on 7/7/17.
//

#include "proc.h"

void create_user_stack(uint32_t start_addr, uint32_t size) {
    ASSERT(current_dir != NULL);
    putf_const("creating user stack:%x %x\n", start_addr, size);

    for (int x = size; x >= 0; x -= 0x1000) {
        page_t *page = get_page(start_addr - x, true, current_dir);//栈是高地址到低地址增长的
        putf_const("[%x]page addr:%x dir:%x\n", start_addr - x, page, current_dir)
        alloc_frame(
                page,
                false,
                true);
    }
    putln_const("stack frame allocated;")
    k_delay(5);
    switch_page_directory(current_dir);
    flush_TLB();
    uint32_t old_stack_pointer;
    uint32_t old_base_pointer;
    __asm__ __volatile__("mov %%esp, %0" : "=r" (old_stack_pointer));
    __asm__ __volatile__("mov %%ebp, %0" : "=r" (old_base_pointer));
    uint32_t offset = (uint32_t) start_addr - init_esp;
    putf_const("old ebp:%x esp:%x init_esp:%x\n", old_base_pointer,
               old_stack_pointer,
               init_esp);
    putf_const("offset:%x\n", offset);
    uint32_t new_stack_pointer = old_stack_pointer + offset;
    uint32_t new_base_pointer = old_base_pointer + offset;
    putf_const("copy %x to %x size:%x\n", old_stack_pointer, new_stack_pointer, init_esp - old_stack_pointer);
    uint32_t *p = new_stack_pointer;
    memcpy(new_stack_pointer, old_stack_pointer, init_esp - old_stack_pointer);
    putln_const("copy done.");
    // Backtrace through the original stack, copying new values into
    // the new stack.
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
    putln_const("mov done.")
    k_delay(5);
    change_stack(new_base_pointer, new_stack_pointer);
    putln_const("stack changed.")
    k_delay(5);

}