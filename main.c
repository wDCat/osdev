

#include <intdef.h>
#include <page.h>
#include <multiboot.h>
#include <str.h>
#include <proc.h>
#include <kmalloc.h>
#include <ide.h>
#include <idt.h>
#include <isrs.h>
#include <irqs.h>
#include <timer.h>
#include <keyboard.h>
#include <syscall_handler.h>
#include <blk_dev.h>
#include <io.h>
#include <console.h>
#include "dev/tty/include/tty.h"
#include <swap.h>
#include <print.h>
#include <dynlibs.h>
#include <devfs.h>
#include <cpuid.h>

uint32_t init_esp;
#ifndef _BUILD_TIME
#define _BUILD_TIME 00
#endif
uint32_t initrd_start, initrd_end;

extern void little_test();

extern void vga_install();

void int_assert() {
    //Gone~
}

void install_step0() {
    int_assert();
    serial_install();
    gdt_install();
    idt_install();
    isrs_install();
    irq_install();
    paging_install();
}

void install_step1() {
    cpuid_install();
    timer_install();
    keyboard_install();
    syscall_install();
    proc_install();
    vfs_install();
    blk_dev_install();
    ide_install();
    vga_install();
    console_install();
    swap_install();
    dynlibs_install();
    tty_install();
}

int kernel_init() {
    install_step1();
    setpid(1);
    mount_rootfs(initrd_start);
    screen_clear();
    tty_write(&ttys[0], 1, 50, "\n"
            "            DCat's Kernel"
            "\n\n");

    for (int x = 0; x < SCREEN_MAX_X; x++)
        tty_write(&ttys[0], 1, 1, "-");
    little_test();
    PANIC("Hey!I'm here!");
    for (;;);
}

int load_initrd(multiboot_info_t *mul) {
    if (mul->mods_count <= 0) PANIC("initrd module not found..")
    initrd_start = *((uint32_t *) mul->mods_addr);
    initrd_end = *(uint32_t *) (mul->mods_addr + 4);
    heap_placement_addr = initrd_end;
    dprintf("initrd: start:%x end:%x", initrd_start, initrd_end);
    ASSERTM(initrd_start < 0x01000000, "Bad initrd addr.!!");
}

int move_kernel_stack(uint32_t start_addr, uint32_t size) {
    uint32_t ebp, esp;
    for (int32_t x = size; x >= 0; x -= 0x1000) {
        page_t *page = get_page(start_addr - x, true, kernel_dir);
        ASSERT(page);
        alloc_frame(page, false, true);
    }
    ebp = 0xBB0000 - 0x4;
    esp = 0xBB0000 - 0x8;
    dprintf("new kernel stack:ebp:%x esp:%x", ebp, esp);
    uint32_t old_esp;
    __asm__ __volatile__("mov %%esp,%0;":"=r"(old_esp));
    dprintf("init esp:%x current:%x used:%x", init_esp, old_esp, init_esp - old_esp);
    if (init_esp - old_esp > 1024) PANIC("Some code are covered by kernel init stack.");
    change_stack(ebp, esp);
    __asm__ __volatile__("jmp %%eax;"::"a"(kernel_init));
}

int main(uint32_t magic, multiboot_info_t *mul_arg, uint32_t init_esp_arg) {
    putf("    deep dark fantasy");
    ASSERT(magic == 0x2BADB002);
    init_esp = init_esp_arg;
    dprintf("multiboot info table:%x init_esp:%x", mul_arg, init_esp_arg);
    load_initrd(mul_arg);
    install_step0();
    move_kernel_stack(0xBB0000, 0x10000);
    PANIC("Hey!I'm here!");
}
