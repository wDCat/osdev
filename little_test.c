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
#include <tty.h>
#include <swap_disk.h>
#include <swap.h>
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
    putln_const("------------------------");
    for (int x = 0; x < al->size; x++) {
        puts_const("AL[");
        putint(x);
        puts_const("] st:");
        puthex(al->headers[x].addr);
        puts_const("  size:");
        puthex(al->headers[x].size);
        puts_const("  used:");
        putint(al->headers[x].used);
        putn();
    }
    putln_const("------------------------");
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
    uint32_t *a = kmalloc_internal(10, false, &phy, true);
    uint32_t geted_phy = get_physical_address(a);
    *a = 0xFAFAFAFAF;
    putf_const("get:%x right:%x vm:%x\n", geted_phy, phy, a);
    disable_paging();
    PANIC("Get phy test done.");
}


void usermode_test() {
    syscall_screen_print("Nekoya");
    pid_t cpid = (pid_t) syscall_fork();
    syscall_screen_print("fork done.\n");
    char buff[256];
    switch (cpid) {
        case 0:
            strformat(buff, "[child] [%x]\n", syscall_getpid());
            syscall_screen_print(buff);
            for (int x = 0;; x++) {
                strformat(buff, "[child] [count:%x]\n", x);
                syscall_screen_print(buff);
                //syscall_hello_switcher(2);
                if (x == 3) {
                    int cpid = syscall_fork();
                    if (cpid != 0)return;
                }
                syscall_delay(1);
            }
            break;
        default:
            strformat(buff, "[father] [%x]child:[%x]\n", syscall_getpid(), cpid);
            syscall_screen_print(buff);
            for (int x = 0;; x++) {
                strformat(buff, "[father] [count:%x]\n", x);
                syscall_screen_print(buff);
                if (x == 3) {
                    int cpid = syscall_fork();
                    if (cpid == 0) {
                        for (int x = 0; x < 100; x++) {
                            syscall_screen_print("NEKONEKONE!!!");
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
    __asm__ __volatile__("int $0x60" : "=a" (a) : "0" (3));//It will enter user mode and clear the stack.
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

int little_test() {

    usermode();
    for (int x = 0;; x++) {
        dprintf("test time:%x", x);
        uint32_t *a = kmalloc_paging(0x1000, NULL);
        kfree(a);
    }
    //
}