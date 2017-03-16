
%macro ISR 2 ; arg1: isr id; arg2 has error code
    global _isr%1
    _isr%1:
    %if %2
        push byte 0
    %endif
    push byte %1
    jmp _isr_common_stub
%endmacro
extern fault_handler
_isr_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10   ; Load the Kernel Data Segment descriptor!
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp   ; Push us the stack
    push eax
    mov eax, fault_handler
    call eax       ; A special call, preserves the 'eip' register
    pop eax
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8     ; Cleans up the pushed error code and pushed ISR number
    iret           ; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP!
ISR 0,0
ISR 1,0
ISR 2,0
ISR 3,0
ISR 4,0
ISR 5,0
ISR 6,0
ISR 7,0
ISR 8,1
ISR 9,0
ISR 10,1
ISR 11,1
ISR 12,1
ISR 13,1
ISR 14,1
ISR 15,0
ISR 16,0
ISR 17,0
ISR 18,0
ISR 19,0
ISR 20,0
;.....
