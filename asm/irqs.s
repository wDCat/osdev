
.extern irq_handler
_irq_common_stub:
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
    mov irq_handler,%eax
    call %eax
    pop %eax
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $0x8,%esp
    iret
.macro _irq no
    .global _irq\no
    _irq\no:
        ret
.endm
_irq 0
_irq 1
_irq 2
_irq 3
_irq 4
_irq 5
_irq 6
_irq 7
_irq 8
_irq 9
_irq 10
_irq 11
_irq 12
_irq 13
_irq 14
_irq 15