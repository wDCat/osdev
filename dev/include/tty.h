//
// Created by dcat on 9/8/17.
//

#ifndef W2_TTY_H
#define W2_TTY_H

#include "intdef.h"
#include "proc.h"
#include "console.h"
#include "mutex.h"

#define TTY_BUFF_SIZE 256
#define TTY_MAX_COUNT 5
#define TTY_MAX_WAIT_PROC 20

#define TTY_IS_EOF(x) ((x)=='\n')
typedef struct {
    bool present;
    pid_t pid;
    int32_t size;
    uchar_t *buff;
} tty_wait_entry_t;
typedef struct {
    bool inited;
    uint32_t head;
    uint32_t foot;
    mutex_lock_t mutex;
    tty_wait_entry_t proc_wait[TTY_MAX_WAIT_PROC];
    uint8_t proc_wait_num;
    uchar_t buff[TTY_BUFF_SIZE];
} tty_queue_t;
typedef struct tty_struct {
    console_t *console;
    tty_queue_t *read;
    tty_queue_t *write;
    tty_queue_t *kinput;
} tty_t;
extern tty_t ttys[TTY_MAX_COUNT];

void tty_install();

void tty_kb_handler(int code);

int32_t tty_read(tty_t *tty, pid_t pid, int32_t size, uchar_t *buff);

int32_t tty_write(tty_t *tty, pid_t pid, int32_t size, uchar_t *buff);

#endif //W2_TTY_H
