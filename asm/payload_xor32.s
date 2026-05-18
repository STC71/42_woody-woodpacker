bits 32
section .text
global _start

_start:
    push dword 0
    pushfd
    pushad

    ; ptrace anti-debugging
    mov eax, 26
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
    int 0x80
    test eax, eax
    js silent_exit

    ; get_vars_ptr
    call get_vars_ptr
get_vars_ptr:
    pop ebp
    add ebp, (vars - get_vars_ptr)

    ; printf woody
    mov eax, 4
    mov ebx, 1
    lea ecx, [ebp + (msg - vars)]
    mov edx, 14
    int 0x80

    ; mprotect
    mov esi, [ebp + 0]  ; rel_text
    add esi, ebp        ; esi = runtime _text

    mov ebx, esi
    and ebx, ~0xFFF     ; Page align (4KB)
    
    mov ecx, esi
    add ecx, [ebp + 4]  ; + text_size
    sub ecx, ebx        ; total size aligned
    
    mov eax, 125        ; sys_mprotect
    mov edx, 7          ; PROT_READ|PROT_WRITE|PROT_EXEC
    int 0x80

    ; XOR Algorithm
    xor ecx, ecx        ; i = 0 (text_size counter)

xor_loop:
    cmp ecx, [ebp + 4]  ; text_size
    je finish_xor

    ; key_index = i % 16
    mov eax, ecx
    and eax, 15

    ; key_byte = key[key_index]
    lea ebx, [ebp + 8]
    mov al, [ebx + eax]
    
    ; text[i] ^= key_byte
    xor byte [esi + ecx], al

    inc ecx
    jmp xor_loop

finish_xor:
    ; patch OEP
    mov edi, [ebp + 24] ; rel_oep (offset 24)
    add edi, ebp
    mov [esp + 36], edi

    popad
    popfd
    ret

silent_exit:
    mov eax, 1
    xor ebx, ebx
    int 0x80

align 8
vars:
    dq 0x1122334455667788 ; MUST match magic header for memmem!
    dq 0x0
    dq 0x0
    dq 0x0
    dq 0x0
    
msg:
    db "....WOODY....", 10
