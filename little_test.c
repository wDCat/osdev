//
// Created by dcat on 9/7/17.
//
#include "ker/include/isrs.h"
#include "ker/include/irqs.h"
#include "memory/include/kmalloc.h"
#include "memory/include/heap_array_list.h"
#include <ide.h>
#include "memory/include/page.h"
#include <exec.h>
#include <catmfs.h>
#include <catrfmt.h>
#include <catrfmt_def.h>
#include <io.h>
#include <syscall.h>
#include "dev/tty/include/tty.h"
#include <swap_disk.h>
#include <swap.h>
#include <print.h>
#include <heap_array_list.h>
#include <blkqueue.h>
#include "ker/include/gdt.h"
#include "ker/include/idt.h"
#include "ker/include/system.h"
#include "screen.h"
#include "str.h"
#include "timer.h"
#include "keyboard.h"
#include "memory/include/page.h"
#include "memory/include/contious_heap.h"
#include "ker/include/multiboot.h"
#include "fs/catmfs/include/catrfmt.h"
#include "syscall_handler.h"
#include "proc.h"
#include "fs/ext2/include/ext2.h"
#include "syscall.h"

int putTest(int a) {
    putc("12121"[a]);
    return 0;
}


void dump_al(heap_array_list_t *al) {
    dprintf("------------------------");
    for (int x = 0; x < al->size; x++) {
        dprintf("AL[%x] st:%x size:%x used:%x teip:%x", x, al->headers[x].addr,
                al->headers[x].size, al->headers[x].used, al->headers[x].trace_eip);
    }
    dprintf("------------------------");
}


void str_test() {
    char a[1024];
    strcpy(a, STR("Hello%sCat."));
    putln(a);
    char b[1024];
    strformat(b, STR("Need%s:%x"), STR("Hello"), 11);
    //char *p = strfmt_insspace(a, 5, 2, 1);
    //p[0] = 'a';
    //p[1] = 's';
    //p[2] = 'b';
    putln(b);
}


void catmfs_test(uint32_t addr) {
    catrfmt_t *fs = catrfmt_init(addr);
    putf_const("init done.%x", fs);
    fs_node_t *node;
    catrfmt_dumpfilelist(fs);
    if (catrfmt_findbyname(fs, STR("neko.1"), &node)) {
        putf(STR("found:%s\n"), node->name);
        char data[250];
        memset(data, 0xCC, sizeof(char) * 250);
        uint32_t count = catrfmt_read(node, 0, 200, data);
        data[count] = '\0';
        putf(STR("[%d]read data:"), count);
        puts(data);
        putln("");
    } else {
        PANIC("file not found.");
    }
    if (catrfmt_findbyname(fs, STR("a.out"), &node)) {
        putf(STR("found:%s\n"), node->name);
        char data[0x1000];
        memset(data, 0xCC, sizeof(char) * 0x1000);
        uint32_t count = catrfmt_read(node, 0, 0x1000, data);
        data[count] = '\0';
        //putf_const("now exec it.");
        alloc_frame(get_page(0xB0000000, true, current_dir), false, false);
        memcpy(0xB0000000, data, 0x1000);
        //enter_ring3(0xB0000000);

    } else {
        PANIC("file not found.");
    }
}


void get_phy_test() {

    uint32_t phy;
    uint32_t *a = kmalloc_internal(10, false, &phy, true, NULL);
    uint32_t geted_phy = get_physical_address(a);
    *a = 0xFAFAFAFAF;
    putf_const("get:%x right:%x vm:%x\n", geted_phy, phy, a);
    disable_paging();
    PANIC("Get phy test done.");
}


void little_test2() {
    printf("Nekoya");
    pid_t cpid = (pid_t) syscall_fork();
    printf("fork done.\n");
    char buff[256];
    switch (cpid) {
        case 0:
            strformat(buff, "[child] [%x]\n", syscall_getpid());
            printf(buff);
            for (int x = 0;; x++) {
                strformat(buff, "[child] [count:%x]\n", x);
                printf(buff);
                if (x == 9999) {
                    int cpid = syscall_fork();
                    if (cpid != 0)return;
                }
                syscall_delay(1);
            }
            break;
        default:
            strformat(buff, "[father] [%x]child:[%x]\n", syscall_getpid(), cpid);
            printf(buff);
            for (int x = 0;; x++) {
                strformat(buff, "[father] [count:%x]\n", x);
                printf(buff);
                if (x == 9999) {
                    int cpid = syscall_fork();
                    if (cpid == 0) {
                        for (int x = 0; x < 100; x++) {
                            printf("NEKONEKONE!!!");
                        }
                        syscall_exit(1);
                    } else {

                    }
                }
                //syscall_hello_switcher(3);
                syscall_delay(1);
            }
            break;
    }
}

void usermode() {
    uint32_t ebp, esp;
    __asm__ __volatile__("mov %%esp, %0" : "=r" (esp));
    __asm__ __volatile__("mov %%ebp, %0" : "=r" (ebp));
    set_kernel_stack(esp);
    long a;
    __asm__ __volatile__("int $0x60" : "=a" (a) : "0" (3));//It will enter user flags and clear the stack.
    //Never exec..
}

void ide_test() {
    cli();
    uint8_t data[1024];
    //memset(data, 0, sizeof(uint8_t) * 1024);
    uint8_t err;
    err = ide_ata_access(ATA_READ, 1, 2, 1, data);
    if (err != 0) {
        putf_const("read error.[%x]", err);
        //ide_print_error(1, err);
        return;
    }
    putf_const("read done.");
    /*
    for (int x = 0; x < 200; x++) {
        puts_const("[");
        puthex(data[x]);
        puts_const("]");

    }*/
    extern void ext2_test();
    ext2_test();


}

int sinterrupt_test() {
    dprintf("testing ic");
    sti();
    int ints;
    ssti(&ints);
    dprintf("ic status:%x", get_interrupt_status());
    srestorei(&ints);
    dprintf("ic status:%x", get_interrupt_status());
}

int blkqueue_test() {
    blkqueue_t b;
    blkqueue_init(&b, 2);
    for (int x = 0; x < 10; x++) {
        blkqueue_insert(&b, 0xA000 + x);
    }
    uint32_t val;
    blkqueue_remove_first(&b, &val);
    dprintf("remove first:%x", val);
    blkqueue_remove_last(&b, &val);
    dprintf("remove last:%x", val);
    for (int x = 0; x < 8; x++) {
        uint32_t addr = blkqueue_get(&b, x);
        //dprintf("[%x]got addr:%x", x, addr);
    }
    blkqueue_iter_t i;
    blkqueue_iter_begin(&i, &b);
    while (true) {
        uint32_t addr = blkqueue_iter_next(&i);
        if (addr == NULL)break;
        dprintf("iter got addr:%x", addr);
    }
    dprintf("blkqueue test done~");
}

int little_test() {
    usermode();
    for (int x = 0;; x++) {
        dprintf("test time:%x", x);
        uint32_t *a = kmalloc_paging(0x1000, NULL);
        kfree(a);
    }
    //
}