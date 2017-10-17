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