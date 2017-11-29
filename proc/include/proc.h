//
// Created by dcat on 7/7/17.
//

#ifndef W2_PROC_H
#define W2_PROC_H

#include <contious_heap.h>
#include <mutex.h>
#include "intdef.h"
#include "page.h"
#include "tss.h"
#include "vfs.h"
#include "proc_queue.h"

#define MAX_PROC_COUNT 64
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
#define UM_KSTACK_START 0x10000000
#define UM_KSTACK_SIZE  0x1000
#define UM_STACK_START 0xF000000
#define UM_STACK_SIZE  0x4000
#define get_proc_status(pcb) ((pcb)->status)

typedef enum proc_status {
    STATUS_NEW = 0x0,
    STATUS_RUN = 0x1,
    STATUS_READY = 0x2,
    STATUS_WAIT = 0x3,
    STATUS_ZOMBIE = 0x4,
    STATUS_DIED = 0x5

} proc_status_t;
typedef int16_t pid_t;
typedef struct {
    uint32_t base;
    uint32_t limit;
} ldt_limit_entry_t;
typedef struct pcb_struct {
    bool present;
    proc_status_t status;
    pid_t pid;
    tss_entry_t tss;
    page_directory_t *page_dir;
    uint32_t time_slice;
    ldt_limit_entry_t *ldt_table;
    uint8_t ldt_table_count;
    uint32_t reserved_page;
    struct spage_info *spages;
    uint8_t spages_count;
    int exit_sig;
    int exit_val;
    uint32_t signal_handler[25];
    uint32_t blocked;
    uint32_t signal;
    bool rejmp;
    vfs_t vfs;
    char dir[256];
    char name[256];
    char cmdline[512];
    bool heap_ready;
    heap_t heap;
    uint8_t hold_proc;//Can this pcb be free?
    int8_t tty_no;
    struct pcb_struct *fpcb;
    struct pcb_struct *cpcb;
    //same group proc linked list
    struct pcb_struct *next_pcb;
    struct elf_digested *edg;
    struct dynlib_inctree *dynlibs;
    uint32_t dynlibs_end_addr;
    file_handle_t fh[MAX_FILE_HANDLES];
    void *tls;
    mutex_lock_t lock;
    regs_t *lastreg;//for soft switch
    uint32_t program_break;
} pcb_t;

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

void setpid(pid_t pid);

int destory_user_heap(pcb_t *pcb);

int create_user_heap(pcb_t *pcb, uint32_t start, uint32_t size);

void save_cur_proc_reg(regs_t *r);

uint32_t *get_errno_location(pcb_t *pcb);

#endif //W2_PROC_H
