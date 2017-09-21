.section .text
.extern fault_handler
.global _isr_common_stub
_isr_common_stub:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov $0x10,%ax
    mov %ax,%dx
    mov %ax,%es
    mov %ax,%fs
    mov %ax,%gs
    mov %esp,%eax
    push %eax
    mov $fault_handler,%eax
    call %eax
    pop %eax
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $0x8,%esp
    iret
.macro isr_nec no
    .global _isr\no
    _isr\no:
        push $0
        push $\no
        jmp _isr_common_stub
.endm
.macro isr_ec no
    .global _isr\no
    _isr\no:
        push $\no
        jmp _isr_common_stub
.endm
isr_nec 0
isr_nec 1
isr_nec 2
isr_nec 3
isr_nec 4
isr_nec 5
isr_nec 6
isr_nec 7
isr_nec 8
isr_nec 9
isr_ec 10
isr_ec 11
isr_ec 12
isr_ec 13
isr_ec 14
isr_nec 15
isr_nec 16
isr_nec 17
isr_nec 18
isr_nec 19
isr_nec 20