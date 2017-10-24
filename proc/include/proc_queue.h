//
// Created by dcat on 10/13/17.
//

#ifndef W2_PROC_QUEUE_H
#define W2_PROC_QUEUE_H

#include "system.h"

typedef struct proc_queue_entry {
    struct pcb_struct *pcb;
    struct proc_queue_entry *next;
} proc_queue_entry_t;
typedef struct {
    uint32_t count;
    struct proc_queue_entry *first;
} proc_queue_t;
typedef struct {
    proc_queue_t *queue;
    proc_queue_entry_t *current;
} proc_queue_iter_t;
extern proc_queue_t *proc_avali_queue,
        *proc_ready_queue,
        *proc_wait_queue, *proc_died_queue;
#define proc_queue_count(q) ((q)->count)
int proc_queue_insert(proc_queue_t *ns, struct pcb_struct *pcb);

struct pcb_struct *proc_queue_next(proc_queue_t *ns);

int proc_queue_remove(proc_queue_t *old, struct pcb_struct *pcb);

int proc_queue_wakeupall(proc_queue_t *ns, bool clear_queue);

int proc_queue_iter_begin(proc_queue_iter_t *iter, proc_queue_t *ns);

struct pcb_struct *proc_queue_iter_next(proc_queue_iter_t *iter);

int proc_queue_iter_end(proc_queue_iter_t *iter);

#endif //W2_PROC_QUEUE_H
