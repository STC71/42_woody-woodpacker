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

    ; RC4: Init S-Box
    sub esp, 256
    mov edi, esp
    
    xor ecx, ecx
init_sbox:
    mov [edi + ecx], cl
    inc ecx
    cmp ecx, 256
    jne init_sbox

    ; RC4: KSA
    xor ecx, ecx        ; i
    xor edx, edx        ; j
ksa_loop:
    movzx eax, byte [edi + ecx]
    add edx, eax
    
    mov eax, ecx
    and eax, 15         ; i % 16 (key is 16 bytes)
    lea ebx, [ebp + 8]  ; key starts at offset 8
    movzx eax, byte [ebx + eax]
    add edx, eax
    and edx, 0xFF

    mov al, [edi + ecx]
    mov bl, [edi + edx]
    mov [edi + ecx], bl
    mov [edi + edx], al

    inc ecx
    cmp ecx, 256
    jne ksa_loop

    ; RC4: PRGA
    xor ecx, ecx        ; i
    xor edx, edx        ; j
    xor ebx, ebx        ; total bytes counter

prga_loop:
    cmp ebx, [ebp + 4]  ; ebx == text_size?
    je finish_rc4

    inc ecx
    and ecx, 0xFF
    
    movzx eax, byte [edi + ecx]
    add edx, eax
    and edx, 0xFF

    ; Swap S-Box[i] y S-Box[j]
    mov ah, [edi + ecx] ; Using ah instead of al so we keep i value free
    mov al, [edi + edx] ; ... ah wait, we're using al for S-Box[j]
    ; Let's just use bl...? No ebx is our counter.
    ; esi is text addr.
    ; ebp is vars.
    ; edi is S-box.
    ; ecx is i, edx is j.

    push ebx ; save counter temporarily for swap
    mov bl, [edi + ecx]
    mov bh, [edi + edx]
    mov [edi + ecx], bh
    mov [edi + edx], bl

    ; k = S-Box[j] + S-Box[i]
    add bl, bh
    movzx eax, bl
    mov al, [edi + eax]
    
    pop ebx ; restore counter

    ; XOR con texto
    xor byte [esi + ebx], al

    inc ebx
    jmp prga_loop

finish_rc4:
    add esp, 256

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
