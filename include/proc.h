//
// Created by dcat on 7/7/17.
//

#ifndef W2_PROC_H
#define W2_PROC_H

#include "intdef.h"
#include "page.h"
#include "tss.h"

#define MAX_PROC_COUNT 256
/*"pop %eax;" \
   "or $0x200,%eax;" \
    "pushl %eax;" \*/
#define enter_user_mode() \
__asm__ __volatile__ ( \
    "movl %%esp,%%eax;" \
    "pushl $0x23;" \
    "pushl %%eax;" \
    "pushfl;" \
    "pushl $0x1b;" \
    "pushl $1f;" \
    "iret;" \
    "1:\tmovl $0x23,%%eax;" \
    "mov %%ax,%%ds;" \
    "mov %%ax,%%es;" \
    "mov %%ax,%%fs;" \
    "mov %%ax,%%gs;" \
    :::"ax")
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
    uint32_t base;
    uint32_t limit;
} ldt_limit_entry_t;
typedef struct {
    bool present;
    proc_status_t status;
    pid_t pid;
    tss_entry_t tss;
    page_directory_t *page_dir;
    uint32_t time_slice;
    ldt_limit_entry_t *ldt_table[];
    uint8_t ldt_table_size;
    uint32_t reserved_page;
} pcb_t;
typedef struct {
    uint32_t count;
    pcb_t *pcbs[1023];
} proc_queue_t;
extern proc_queue_t *proc_avali_queue, *proc_ready_queue;

pid_t getpid();

void proc_install();

void
copy_current_stack(uint32_t start_addr, uint32_t size, uint32_t *new_ebp, uint32_t *new_esp, uint32_t start_esp,
                   page_directory_t *dir);

pid_t fork(regs_t *r);

pcb_t *getpcb(pid_t pid);

void save_proc_state(pcb_t *pcb, regs_t *r);

void switch_to_proc(pcb_t *pcb);

void set_proc_status(pcb_t *pcb, proc_status_t new_status);

void proc_exit(uint32_t ret);

#endif //W2_PROC_H
