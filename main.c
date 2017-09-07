

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

uint32_t init_esp;
#ifndef _BUILD_TIME
#define _BUILD_TIME 00
#endif

extern void little_test();

extern void vga_install();

void int_assert() {
    ASSERT(sizeof(uint8_t) == 1);
    ASSERT(sizeof(uint16_t) == 2);
    ASSERT(sizeof(uint32_t) == 4);
}

void install_all() {
    int_assert();
    serial_install();
    gdt_install();
    idt_install();
    isrs_install();
    irq_install();
    timer_install();
    keyboard_install();
    paging_install();
    syscall_install();
    proc_install();
    vfs_install();
    blk_dev_install();
    ide_install();
    vga_install();
}

int move_kernel_stack(uint32_t start_addr, uint32_t size) {
    //proc 1
    uint32_t ebp, esp;
    for (int32_t x = size; x >= 0; x -= 0x1000) {
        page_t *page = get_page(start_addr - x, true, kernel_dir);
        ASSERT(page);
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
    screen_clear();
    putln_const("")
    putln_const("            DCat's Kernel")
    putln_const("")
    putln_const("--------------------------------------------------------------------------------")
    putln_const("")
    dprintf("init esp:%x", init_esp);
    if (mul.mods_count <= 0) {
        PANIC("module not found..")
    }
    sti();
    mount_rootfs(initrd_start);
    move_kernel_stack(0xBB0000, 0x10000);
    PANIC("Hey!I'm here!");
}
