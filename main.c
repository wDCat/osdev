#include <isrs.h>
#include <irqs.h>
#include <kmalloc.h>
#include <heap_array_list.h>
#include <ide.h>
#include "gdt.h"
#include "idt.h"
#include "include/system.h"
#include "include/screen.h"
#include "include/str.h"
#include "include/timer.h"
#include "include/keyboard.h"
#include "page.h"
#include "heap.h"
#include "multiboot.h"

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

void outportb(unsigned short _port, unsigned char _data) {
    __asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
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
}

int main(multiboot_info_t *mul_arg) {
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
    uint8_t *initrd_raw = (uint8_t *) initrd_start;
    for (int x = 0; x < initrd_end - initrd_start; x++) {
        putc(initrd_raw[x]);
    }
    puts_const("[+] main done.");
    for (;;);
}
