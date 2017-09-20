.extern _isr_common
.global _isr_syscall
_isr_syscall:
    push $0
    push $0x60
    jmp _isr_common