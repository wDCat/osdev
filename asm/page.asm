
global _switch_page_dir_internal
_switch_page_dir_internal:
    push ebx
	sub esp,4h
	mov cr3,eax
	mov ebx,cr0
	or ebx,0x80000000
	mov cr0,ebx
	add esp,4h
	pop ebx
	iret
	