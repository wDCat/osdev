#include <isrs.h>
#include <irqs.h>
#include <kmalloc.h>
#include <heap_array_list.h>
#include <ide.h>
#include <page.h>
#include "gdt.h"
#include "idt.h"
#include "include/system.h"
#include "include/screen.h"
#include "include/str.h"
#include "include/timer.h"
#include "include/keyboard.h"
#include "page.h"
#include "contious_heap.h"
#include "multiboot.h"
#include "fs/catmfs/include/catrfmt.h"
#include "syscall.h"
#include "proc.h"
#include "fs/ext2/include/ext2.h"

int putTest(int a) {
    putc("12121"[a]);
    return 0;
}

void intAssert() {
    ASSERT(sizeof(uint8_t) == 1);
    ASSERT(sizeof(uint16_t) == 2);
    ASSERT(sizeof(uint32_t) == 4);
}

void dump_al(heap_array_list_t *al) {
    putln_const("------------------------")
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
    putln_const("------------------------")
}

void heap_test() {
    puts_const("[+] heap test");
    heap_t *heap = create_heap(KHEAP_START, KHEAP_START + KHEAP_SIZE, KHEAP_START + KHEAP_SIZE * 2, kernel_dir);
    void *mem = halloc(heap, 4096, false);
    dumphex("alloced mem addr:", mem);
    void *mem2 = halloc(heap, 4096, false);
    dumphex("alloced mem addr:", mem2);
    void *mem3 = halloc(heap, 4096, false);
    dumphex("alloced mem addr:", mem3);
    dump_al(heap->al);
    dumphex("header size:", HOLE_HEADER_SIZE);
    dumphex("footer size:", HOLE_FOOTER_SIZE);
    pause();
    hfree(heap, mem2);
    dump_al(heap->al);
    pause();
    hfree(heap, mem3);
    dump_al(heap->al);
    pause();
    void *mem4 = halloc(heap, 1024, false);
    dumphex("alloced mem addr:", mem4);
    dump_al(heap->al);
    pause();
    void *mem5 = halloc(heap, 1024 * 200, false);
    dumphex("alloced mem addr:", mem5);
    dump_al(heap->al);
    pause();
    hfree(heap, mem4);
    dump_al(heap->al);
    pause();
    hfree(heap, mem5);
    dump_al(heap->al);
    pause();
    hfree(heap, mem);
    dump_al(heap->al);
    pause();
    puts_const("[+] heap test done.");
}

void install_all() {
    serial_install();
    gdt_install();
    idt_install();
    isrs_install();
    irq_install();
    timer_install();
    keyboard_install();
    intAssert();
    //kmalloc_install();
    paging_install();
    syscall_install();
    proc_install();
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

/*
void catmfs_test(uint32_t addr) {
    catmfs_t *fs = catmfs_init(addr);
    putf_const("init done.%x", fs);
    fs_node_t *node;
    catmfs_dumpfilelist(fs);
    if (catmfs_findbyname(fs, STR("neko.1"), &node)) {
        putf(STR("found:%s\n"), node->name);
        char data[250];
        memset(data, 0xCC, sizeof(char) * 250);
        uint32_t count = read_fs(node, 0, 200, data);
        data[count] = '\0';
        putf(STR("[%d]read data:"), count);
        puts(data);
        putln("");
    } else {
        PANIC("file not found.")
    }
    if (catmfs_findbyname(fs, STR("a.out"), &node)) {
        putf(STR("found:%s\n"), node->name);
        char data[0x1000];
        memset(data, 0xCC, sizeof(char) * 0x1000);
        uint32_t count = read_fs(node, 0, 0x1000, data);
        data[count] = '\0';
        //putf_const("now exec it.");
        alloc_frame(get_page(0xB0000000, true, current_dir), false, false);
        memcpy(0xB0000000, data, 0x1000);
        //enter_ring3(0xB0000000);

    } else {
        PANIC("file not found.")
    }
}

*/
void user_app() {
    syscall_helloworld();
    syscall_screen_print(STR("[]Hello DCat~"));
    int c = 0;
    int a = 24 + 12 / c;
    char result[12];
    strformat(result, "%x", a);
    syscall_screen_print(result);
    for (;;);
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
    putf_const("syscall_fork[%x][%x]", syscall_fork, esp);
    //enter_user_mode();
    //syscall_hello_switcher(1);
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


uint32_t init_esp;
#ifndef _BUILD_TIME
#define _BUILD_TIME 00
#endif

int little_test() {
    extern void vfs_test();
    vfs_test();
    //dprintf("done.");
    for (;;);
}

int move_kernel_stack(uint32_t start_addr, uint32_t size) {
    uint32_t ebp, esp;
    for (int32_t x = 0x4000; x >= 0; x -= 0x1000) {
        page_t *page = get_page(start_addr - x, true, kernel_dir);
        ASSERT(page);
        //dprintf("alloc frame for:%x", start_addr - x);
        alloc_frame(page, false, true);
    }
    ebp = 0xBB0000 - 0x4;
    esp = 0xBB0000 - 0x8;
    dprintf("kernel stack:ebp:%x esp:%x", ebp, esp);
    change_stack(ebp, esp);
    __asm__ __volatile__("jmp %0;"::"m"(little_test));
}

int main(multiboot_info_t *mul_arg, uint32_t init_esp_arg) {
    init_esp = init_esp_arg;
    multiboot_info_t mul;
    uint32_t initrd_start = *((uint32_t *) mul_arg->mods_addr);
    uint32_t initrd_end = *(uint32_t *) (mul_arg->mods_addr + 4);
    heap_placement_addr = initrd_end;
    //mul may lost....
    memcpy(&mul, mul_arg, sizeof(multiboot_info_t));
    //MUL HEADER LOST AFTER INSTALL!!!
    install_all();
    dprintf("init esp:%x", init_esp);
    if (mul.mods_count <= 0) {
        PANIC("module not found..")
    }
    putln_const("[+] main called.");
    ASSERT(strlen(STR("Hello DCat")) == 10);
    putln_const("[+] Super Neko");
    putf_const("[+] Built Time: %d \n", _BUILD_TIME);
    putln_const("[+] Now enable IRQs");
    sti();
    //dumphex("mod count:", mul.mods_count);
    //dumphex("initrd_start:", initrd_start);
    //dumphex("initrd_end:", initrd_end);
    /*
    uint8_t *initrd_raw = (uint8_t *) initrd_start;
    for (int x = 0; x < initrd_end - initrd_start; x++) {
        putc(initrd_raw[x]);
    }*/
    //catmfs_test(initrd_start);
    //delay(2);
    //usermode();
    //ide_test();
    ide_initialize(0x1F0, 0x3F6, 0x170, 0x376, 0x000);
    move_kernel_stack(0xBB0000, 0x10000);
    puts_const("[+] main done.");
    for (;;);
}
