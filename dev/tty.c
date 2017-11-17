//
// Created by dcat on 9/8/17.
//

#include <kmalloc.h>
#include <tty.h>
#include <str.h>
#include <schedule.h>
#include <mutex.h>
#include <keyboard.h>
#include <vfs.h>
#include <signal.h>
#include <devfs.h>

tty_t ttys[TTY_MAX_COUNT];
uint8_t cur_tty_id;
fs_t ttyfs;

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
    if (c_out)
        *c_out = queue->buff[queue->head];
    queue->head++;
    if (queue->head >= TTY_BUFF_SIZE)
        queue->head = 0;
    if (tty_queue_isempty(queue)) {
        queue->inited = false;
    }
    return true;
}

inline bool tty_queue_remove_last(tty_queue_t *queue, uchar_t *c_out) {
    if (tty_queue_isempty(queue)) {
        return false;
    }
    if (c_out)
        *c_out = queue->buff[queue->foot];
    queue->foot--;
    queue->buff[queue->foot] = 0;
    return true;
}

void tty_empty_wait(tty_queue_t *queue, pid_t pid) {
    bool empty;
    mutex_lock(&queue->mutex);
    empty = tty_queue_isempty(queue);
    mutex_unlock(&queue->mutex);
    if (empty) {
        mutex_lock(&queue->mutex);
        proc_queue_insert(&queue->proc_wait, getpcb(pid));
        mutex_unlock(&queue->mutex);
        set_proc_status(getpcb(pid), STATUS_WAIT);
        do_schedule(NULL);
    }
}

void tty_full_wait(tty_queue_t *queue, pid_t pid) {
    TODO;
    while (true) {
        bool empty;
        mutex_lock(&queue->mutex);
        empty = tty_queue_isempty(queue);
        mutex_unlock(&queue->mutex);
        if (empty) {
            mutex_lock(&queue->mutex);
            proc_queue_insert(&queue->proc_wait, getpcb(pid));
            mutex_unlock(&queue->mutex);
            set_proc_status(getpcb(pid), STATUS_WAIT);
            do_schedule(NULL);
        }
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
    //tty_full_wait(queue, pid);
    mutex_lock(&queue->mutex);
    ASSERT(tty->console);
    console_putns(tty->console, buff, size);
    mutex_unlock(&queue->mutex);
    return size;
}

pcb_t *tty_queue_next_pcb(tty_queue_t *target, bool remove) {
    pcb_t *pcb = proc_queue_next(&target->proc_wait);
    if (remove && pcb != NULL)
        proc_queue_remove(&target->proc_wait, pcb);
    return pcb;
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
        switch (c) {
            case '\b':
                tty_queue_remove_last(target, NULL);
                break;
            default:
                tty_queue_putc(target, c);
        }
        count++;
    }
    pcb_t *pcb = tty_queue_next_pcb(target, true);
    mutex_unlock(&target->mutex);
    mutex_unlock(&source->mutex);
    if (pcb != NULL) {
        set_proc_status(pcb, STATUS_READY);
        do_schedule(NULL);
    }
    return count;
}

void tty_kb_handler(int code, kb_status_t *kb_status) {
    tty_queue_t *queue = ttys[cur_tty_id].kinput;
    uchar_t c;
    c = kb_status->shift ? kbdusupper[code] : kbdus[code];
    tty_write(&ttys[cur_tty_id], 0, 1, &c);
    if (kb_status->ctrl == true) {
        //FIXME
        char op[2];
        op[0] = '^';
        op[1] = kbdusupper[code];
        switch (kbdusupper[code]) {
            case 'C':
                tty_write(&ttys[cur_tty_id], 0, 2, op);
                pcb_t *npcb =
                        getpid() == 0 ? tty_queue_next_pcb(&ttys[cur_tty_id].read, false)
                                      : getpcb(getpid());
                if (npcb != NULL) {
                    send_sig(npcb, SIGINT);
                    dprintf("Ctrl^C send SIGINT to %x", npcb->pid);
                }
                tty_cook(&ttys[cur_tty_id]);
                return;
            case 'D':
                tty_write(&ttys[cur_tty_id], 0, 2, op);
                //TODO
                return;
        }
    }
    if (code == 0x1C) {
        tty_cook(&ttys[cur_tty_id]);
    } else if (c != 0) {
        mutex_lock(&queue->mutex);
        if (!tty_queue_putc(queue, c)) {
            deprintf("Ignore input:%x", code);
        }
        mutex_unlock(&queue->mutex);
    }
}

int32_t tty_fs_node_read(fs_node_t *node, __fs_special_t *fsp_, uint32_t size, uint8_t *buff) {
    uint8_t ttyid = (uint8_t) fsp_;
    if (ttyid > TTY_MAX_COUNT) {
        deprintf("tty not exist:%x", ttyid);
        return -1;
    }
    return tty_read(&ttys[ttyid], getpid(), size, buff);
}

int32_t tty_fs_node_write(fs_node_t *node, __fs_special_t *fsp_, uint32_t size, uint8_t *buff) {
    uint8_t ttyid = (uint8_t) fsp_;
    if (ttyid > TTY_MAX_COUNT) {
        deprintf("tty not exist:%x", ttyid);
        return 1;
    }
    //dprintf("write tty:%x %s", ttyid, buff);
    return tty_write(&ttys[ttyid], getpid(), size, buff);
}

__fs_special_t *tty_fs_node_mount(int ttyid, fs_node_t *node) {
    node->inode = (uint32_t) ttyid;
    return ttys;
}

void tty_fg_switch(uint8_t ttyid) {
    if (ttyid > TTY_MAX_COUNT) {
        deprintf("tty not exist:%x", ttyid);
        return;
    }
    cur_tty_id = ttyid;
    console_switch(ttys[cur_tty_id].console);
}

void tty_create_fstype() {
    memset(&ttyfs, 0, sizeof(fs_t));
    strcpy(ttyfs.name, "TTY_TEST");
    ttyfs.mount = (mount_type_t) tty_fs_node_mount;
    ttyfs.read = tty_fs_node_read;
    ttyfs.write = tty_fs_node_write;
}

file_operations_t ttydev = {
        .read=tty_fs_node_read,
        .write=tty_fs_node_write
};

void tty_install() {
    memset(ttys, 0, sizeof(tty_t) * TTY_MAX_COUNT);
    for (int x = 0; x < TTY_MAX_COUNT; x++) {
        ttys[x].console = console_alloc();
        if (ttys[x].console == NULL) {
            PANIC("fail to alloc console for tty%s", x);
        }
        ttys[x].read = (tty_queue_t *) kmalloc(sizeof(tty_queue_t));
        memset(ttys[x].read, 0, sizeof(tty_queue_t));
        ttys[x].write = (tty_queue_t *) kmalloc(sizeof(tty_queue_t));
        memset(ttys[x].write, 0, sizeof(tty_queue_t));
        ttys[x].kinput = (tty_queue_t *) kmalloc(sizeof(tty_queue_t));
        memset(ttys[x].kinput, 0, sizeof(tty_queue_t));
        mutex_init(&ttys[x].read->mutex);
        mutex_init(&ttys[x].write->mutex);
        mutex_init(&ttys[x].kinput->mutex);
    }
    cur_tty_id = 0;


}

int tty_register_self() {
    for (int x = 0; x < TTY_MAX_COUNT; x++) {
        char ttyfn[20];
        strformat(ttyfn, "tty%d", x);
        CHK(devfs_register_dev(ttyfn, &ttydev, (void *) x), "");
    }
    return 0;
    _err:
    deprintf("failed");
    return 1;
}