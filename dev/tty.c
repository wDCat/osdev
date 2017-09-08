//
// Created by dcat on 9/8/17.
//

#include <kmalloc.h>
#include <tty.h>
#include <str.h>
#include <schedule.h>
#include <mutex.h>
#include <keyboard.h>

tty_t ttys[TTY_MAX_COUNT];
uint8_t cur_tty_id;

void tty_install() {
    memset(ttys, 0, sizeof(tty_t) * TTY_MAX_COUNT);
    for (int x = 0; x < TTY_MAX_COUNT; x++) {
        ttys[x].console = console_alloc(&ttys[x]);
        ttys[x].read = (tty_queue_t *) kmalloc(sizeof(tty_queue_t));
        memset(ttys[x].read, 0, sizeof(tty_queue_t));
        ttys[x].write = (tty_queue_t *) kmalloc(sizeof(tty_queue_t));
        memset(ttys[x].write, 0, sizeof(tty_queue_t));
        ttys[x].kinput = (tty_queue_t *) kmalloc(sizeof(tty_queue_t));
        memset(ttys[x].kinput, 0, sizeof(tty_queue_t));
        ttys[x].read->proc_wait_num = 1;
        ttys[x].write->proc_wait_num = 1;
        ttys[x].kinput->proc_wait_num = 1;
        mutex_init(&ttys[x].read->mutex);
        mutex_init(&ttys[x].write->mutex);
        mutex_init(&ttys[x].kinput->mutex);
    }
    cur_tty_id = 0;
}

inline bool tty_queue_isempty(tty_queue_t *queue) {
    return queue->head == queue->foot;
}

inline bool tty_queue_isfull(tty_queue_t *queue) {
    return queue->head == queue->foot && queue->inited;
}

inline bool tty_queue_putc(tty_queue_t *queue, uchar_t c) {
    if (tty_queue_isfull(queue)) {
        deprintf("Out of TTY queue!");
        LOOP();//debug
        return false;
    }
    queue->inited = true;
    queue->buff[queue->foot] = c;
    queue->foot++;
    if (queue->foot >= TTY_BUFF_SIZE)
        queue->foot = 0;
    return true;
}

inline bool tty_queue_getc(tty_queue_t *queue, uchar_t *c_out) {
    if (tty_queue_isempty(queue)) {
        return false;
    }
    *c_out = queue->buff[queue->head];
    queue->head++;
    if (queue->head >= TTY_BUFF_SIZE)
        queue->head = 0;
    return true;
}


void tty_empty_wait(tty_queue_t *queue, pid_t pid) {
    bool empty;
    mutex_lock(&queue->mutex);
    empty = tty_queue_isempty(queue);
    mutex_unlock(&queue->mutex);
    if (empty) {
        set_proc_status(getpcb(pid), STATUS_WAIT);
        do_schedule(NULL);
    }
}

void tty_full_wait(tty_queue_t *queue, pid_t pid) {
    while (true) {
        bool empty;
        mutex_lock(&queue->mutex);
        empty = tty_queue_isempty(queue);
        mutex_unlock(&queue->mutex);
        if (empty) {
            set_proc_status(getpcb(pid), STATUS_WAIT);
            do_schedule(NULL);
        } else return;
    }
}

int32_t tty_read(tty_t *tty, pid_t pid, int32_t size, uchar_t *buff) {
    tty_queue_t *queue = tty->read;
    tty_empty_wait(queue, pid);
    mutex_lock(&queue->mutex);
    int32_t ret = 0;
    for (int x = 0; x < size; x++) {
        if (!tty_queue_getc(queue, buff) || TTY_IS_EOF(buff)) {
            ret = x + 1;
            goto _ret;
        }
        buff++;
    }
    ret = size;
    _ret:
    mutex_unlock(&queue->mutex);
    return ret;
}

int32_t tty_write(tty_t *tty, pid_t pid, int32_t size, uchar_t *buff) {
    tty_queue_t *queue = tty->write;
    mutex_lock(&queue->mutex);
    console_putns(tty->console, buff, size);
    mutex_unlock(&queue->mutex);
}

int32_t tty_cook(tty_t *tty) {
    dprintf("now cook it");

    tty_queue_t *source = tty->kinput, *target = tty->read;
    mutex_lock(&source->mutex);
    mutex_lock(&target->mutex);
    int count = 0;
    while (!tty_queue_isempty(source)) {
        uchar_t c;
        if (!tty_queue_getc(source, &c))
            break;
        tty_queue_putc(target, c);
        count++;
    }
    mutex_unlock(&target->mutex);
    mutex_unlock(&source->mutex);
    set_proc_status(getpcb(2), STATUS_READY);
    return count;
}

void tty_kb_handler(int code) {
    tty_queue_t *queue = ttys[cur_tty_id].kinput;
    uchar_t c = kbdus[code];
    tty_write(&ttys[cur_tty_id], 0, 1, &c);
    if (code == 0x1C) {
        tty_cook(&ttys[cur_tty_id]);
    } else {
        mutex_lock(&queue->mutex);
        if (!tty_queue_putc(queue, c)) {
            deprintf("Ignore input:%x", code);
        }
        mutex_unlock(&queue->mutex);
    }
}

