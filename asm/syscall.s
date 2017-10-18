.section .text
.extern _isr_common
.extern choose_next_task
.global _isr_syscall
.global _isr_taskswitch
_isr_syscall:
    push $0
    push $0x60
    jmp _isr_common_stub
_isr_taskswitch:
    push $0
    push $0x61
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
    mov $choose_next_task,%eax
    call %eax
    pop %eax
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $0x8,%esp
    iret