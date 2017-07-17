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
#include "catmfs.h"
#include "syscall.h"
#include "proc.h"

unsigned char *memcpy(unsigned char *dest, const unsigned char *src, int count) {
    for (int x = 0; x < count; x++) {
        dest[x] = src[x];
    }
}

unsigned char *dmemcpy(unsigned char *dest, const unsigned char *src, int count) {

    for (int x = count - 1; x >= 0; x--) {
        dest[x] = src[x];
    }
}

unsigned char *memset(unsigned char *dest, unsigned char val, int count) {
    for (int x = 0; x < count; x++) {
        dest[x] = val;
    }
    return NULL;
}

unsigned char inportb(unsigned short _port) {
    unsigned char rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

void outportb(uint16_t portid, uint8_t value) {
    __asm__ __volatile__("outb %%al, %%dx": :"d" (portid), "a" (value));
}

void outportw(uint16_t portid, uint16_t value) {
    __asm__ __volatile__("outw %%ax, %%dx": :"d" (portid), "a" (value));
}

void outportl(uint16_t portid, uint32_t value) {
    __asm__ __volatile__("outl %%eax, %%dx": :"d" (portid), "a" (value));
}

void k_delay(uint32_t time) {
    for (int x = 0; x < time * 1000; x++) {
        for (int y = 0; y < 100000; y++) {
            int al = 2, bl = 4;
            if (al + bl == 1024) {
                break;
            }
            __asm__ __volatile__("nop");
        }
    }
}

inline const char *itoa(int i, char result[]) {
    int po = i;
    int x = 9;
    result[10] = '\0';
    for (; x >= 0; x--) {
        int l = po % 10;
        result[x] = l + 0x30;
        po /= 10;
        if (po == 0) {
            memcpy(result, &result[x], x + 1);
            break;
        }
    }
    return &result[x];
}

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

void catmfs_test(uint32_t *addr) {

    catmfs_t *fs = catmfs_init(addr);
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
}

void user_app() {
    __asm__ __volatile__("mov $0x1,%eax");
    __asm__ __volatile__("int $0x60;");
    for (;;);
}

void ring3() {
    dumphex("ring3:", ring3);
    __asm__ __volatile__(
    "cli;"
            "mov $0x23,%ax;"
            "mov %ax,%ds;"
            "mov %ax,%es;"
            "mov %ax,%fs;"
            "mov %ax,%gs;"
            "mov %esp,%eax;"
            "push $0x23;"
            "push %eax;"
            "pushf;"
            "pop %eax;"
            "or $0x200,%eax;"
            "pushl %eax;"
            "push $0x1B;"
            "push $1f;"
            "iret;"
            "1:");
    user_app();
    for (;;);
}

uint32_t get_eip() {
    __asm__ __volatile__ (""
            "pop %eax;"
            "jmp %eax;");
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

void kernel_vep_heap_test() {
    //putf_const("[KVH]Test begin//\n");
    uint32_t *a = kmalloc_internal(0x20, false, NULL, kernel_vep_heap);
    putf_const("[KVH]a vm:%x phy:%x\n", a, get_physical_address(a));
    //putf_const("[KVH]Test done//\n");
    //for(;;);
}

void usermode_test() {
    kernel_vep_heap_test();
    cli();
    uint32_t ebp, esp;
    putf_const("cloning the stack..\n")
    copy_current_stack(0xE1000000, 0x1000, &ebp, &esp);
    set_kernel_stack(ebp);
    change_stack(ebp, esp);
    putf_const("cloning the page directory..\n")
    page_directory_t *pd = clone_page_directory(current_dir);
    putf_const("[%x -> %x][%x]switch to cloned page directory...\n", current_dir->physical_addr,
               pd->physical_addr,
               pd->table_physical_addr);
    switch_page_directory(pd);
    putf_const("pd switched.");
    extern void enter_usermode();
    putf_const("enter_usermode:%x\n", enter_usermode);
    __asm__ __volatile__("mov $0x24,%eax;"
            "int $0x60;");
    putf_const("syscall done.\n");
    //k_delay(5);
    //enter_usermode();
    ring3();
    for (;;);
}

uint32_t init_esp;

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
    putln_const("NEKO");
    putln_const("[+] main called.");
    ASSERT(strlen(STR("Hello DCat")) == 10);
    putln_const("[+] Super Neko");
    puts_const("[+] Time:");
    putint(12450);
    putln_const("+1s");
    putln_const("[+] Now enable IRQs");
    __asm__ __volatile__ ("sti");
    void *amem = (void *) kmalloc(4096);
    dumphex("amem:", amem);
    if (mul.mods_count <= 0) {
        PANIC("module not found..")
    }
    dumphex("mod count:", mul.mods_count);
    dumphex("initrd_start:", initrd_start);
    dumphex("initrd_end:", initrd_end);
    /*
    uint8_t *initrd_raw = (uint8_t *) initrd_start;
    for (int x = 0; x < initrd_end - initrd_start; x++) {
        putc(initrd_raw[x]);
    }*/
    //catmfs_test(initrd_start);
    usermode_test();
    puts_const("[+] main done.");
    for (;;);
}
