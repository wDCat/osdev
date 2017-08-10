
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

global _copy_frame_physical
_copy_frame_physical:
    push ebx
    pushf
    mov ebx, [esp+12]
    mov ecx, [esp+16]

    mov edx, cr0
    and edx, 0x7fffffff
    mov cr0, edx

    mov edx, 1024

.loop:
    mov eax, [ebx]
    mov [ecx], eax
    add ebx, 4
    add ecx, 4
    dec edx
    jnz .loop

    mov edx, cr0
    or  edx, 0x80000000
    mov cr0, edx

    popf
    pop ebx
    ret