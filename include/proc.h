//
// Created by dcat on 7/7/17.
//

#ifndef W2_PROC_H
#define W2_PROC_H

#include "intdef.h"
#include "page.h"

#define MAX_PROC_COUNT 256
#define change_stack(ebp, esp){\
__asm__ __volatile__("mov %0, %%ebp" : : "r" (ebp));\
__asm__ __volatile__("mov %0, %%esp" : : "r" (esp));\
}
typedef enum proc_status {
    STATUS_NEW,
    STATUS_RUN,
    STATUS_READY,
    STATUS_WAIT,
    STATUS_DIED
} proc_status_t;
typedef uint32_t pid_t;
typedef struct {
    bool present;
    proc_status_t status;
    pid_t pid;
    uint32_t eip;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    page_directory_t *page_dir;
} pcb_t;

pid_t getpid();

void proc_install();

void copy_current_stack(uint32_t start_addr, uint32_t size, uint32_t *new_ebp, uint32_t *new_esp);

pid_t fork();

#endif //W2_PROC_H
