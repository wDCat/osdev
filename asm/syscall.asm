extern _isr_common_stub
global _isr_syscall
_isr_syscall:
    push byte 0
    push byte 0x60
    jmp _isr_common_stub