//
// Created by dcat on 9/8/17.
//

#ifndef W2_TTY_H
#define W2_TTY_H

#include "intdef.h"
#include "proc.h"
#include "console.h"
#include "mutex.h"
#include "keyboard.h"

#define TTY_BUFF_SIZE 256
#define TTY_MAX_COUNT 5
#define TTY_MAX_WAIT_PROC 20

#define TTY_IS_EOF(x) ((x)=='\n')
typedef struct {
    uint16_t used;
    uint32_t head;
    uint32_t foot;
    mutex_lock_t mutex;
    proc_queue_t proc_wait;
    uchar_t buff[TTY_BUFF_SIZE];
} tty_queue_t;
typedef struct tty_struct {
    console_t *console;
    tty_queue_t *read;
    tty_queue_t *write;
    tty_queue_t *kinput;
} tty_t;
extern tty_t ttys[TTY_MAX_COUNT];
extern fs_t ttyfs;

void tty_install();

void tty_create_fstype();

void tty_kb_handler(int code, kb_status_t *kb_status);

int32_t tty_read(tty_t *tty, pid_t pid, int32_t size, uchar_t *buff);

int32_t tty_write(tty_t *tty, pid_t pid, int32_t size, uchar_t *buff);

void tty_fg_switch(uint8_t ttyid);

int tty_register_self();

#endif //W2_TTY_H
