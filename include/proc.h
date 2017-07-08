//
// Created by dcat on 7/7/17.
//

#ifndef W2_PROC_H
#define W2_PROC_H

#include "intdef.h"
#include "page.h"

#define change_stack(ebp, esp){\
__asm__ __volatile__("mov %0, %%esp" : : "r" (esp));\
__asm__ __volatile__("mov %0, %%ebp" : : "r" (ebp));\
}

void create_user_stack(uint32_t start_addr, uint32_t size);
#endif //W2_PROC_H
