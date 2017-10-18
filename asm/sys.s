.global _get_interrupt_status
_get_interrupt_status:
    pushf
    pop %eax
    bt $0x9,%eax
    jc __ret
    mov $0,%eax
    ret
__ret:
    mov $1,%eax
    ret
.global _after_interrupt_common

_after_interrupt_common:
     pop %eax
     pop %gs
     pop %fs
     pop %es
     pop %ds
     popa
     add $0x8,%esp
     iret