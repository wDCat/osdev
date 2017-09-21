.set MULTIBOOT_MAGIC ,0x1BADB002
.set MULTIBOOT_FLAG_ALIGN , (1<<0)
.set MULTIBOOT_FLAG_MEMORY , (1<<1)
.set MULTIBOOT_FLAG_VIDEO , (1<<2)
.set MULTIBOOT_FLAG_KSPACE , (1<<16)
.set MULTIBOOT_FLAG , (MULTIBOOT_FLAG_ALIGN | MULTIBOOT_FLAG_MEMORY | MULTIBOOT_FLAG_VIDEO | MULTIBOOT_FLAG_KSPACE)
.set MULTIBOOT_CHECKSUM , -(MULTIBOOT_MAGIC+MULTIBOOT_FLAG)
.extern code,bss,end,main
jmp .
.align 4
.extern start
.global mboot
mboot:
    .long MULTIBOOT_MAGIC
    .long MULTIBOOT_FLAG
    .long MULTIBOOT_CHECKSUM
    .long mboot
    .long code
    .long bss
    .long end
    .long start
    .long 1
    .long 320
    .long 200
    .long 8
.global start
.section .text
start:
    mov $_sys_stack,%esp
    push %esp
    push %ebx
    push %eax
    call main
    hlt
.global _gdt_flush
.extern gp
_gdt_flush:
    lgdt (gp)
    mov $0x10,%ax
    mov %ax,%ds
    mov %ax,%es
    mov %ax,%fs
    mov %ax,%gs
    mov %ax,%ss
    ljmp $0x08, $_gdt_flush_step2
_gdt_flush_step2:
    ret
.global _idt_load
.extern idtp
_idt_load:
    lidt (idtp)
    ret
.section .bss
    .align 16
    .space 4096
.global _sys_stack
_sys_stack: