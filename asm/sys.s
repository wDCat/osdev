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
.global _do_switch
_do_switch:
    push %esp
    mov 8(%esp),%eax
