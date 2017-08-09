[BITS 32]
enter_real_mode:
	push 0x800
	push 0x900
	pushfd
	or dword [esp],(1<<17)
	push 0
	push neko
neko:
	mov ah,0x0e
    mov al,'I'
    int 0x10
	jmp $
	
	