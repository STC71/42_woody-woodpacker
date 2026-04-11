bits 64
section .text
global _start

_start:
    ; 1. Guardar todos los registros y flags para no alterar el estado inicial esperado por la original
    pushf
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; 2. Imprimir string "....WOODY...."
    call get_msg
msg: db "....WOODY....", 10
get_msg:
    pop rsi           ; rsi = puntero a string
    mov rax, 1        ; syscall_write
    mov rdi, 1        ; fd = stdout
    mov rdx, 14       ; len
    syscall

    ; 3. Obtener el Instruction Pointer actual (RIP) de forma relativa, fundamental para Position Independent Code (PIE)
    call get_rip
get_rip:
    pop rbp           ; rbp ahora contiene la dirección de get_rip!

    ; 4. Localizar nuestras variables (que inyectaremos desde C)
    lea r12, [rbp + vars - get_rip]

    ; 5. Obtener los punteros absolutos en base a los relativos
    mov r13, [r12 + 0]    ; rel_text
    lea r14, [r12 + r13]  ; r14 = dirección en RAM de .text

    mov r15, [r12 + 8]    ; r15 = tamaño de .text
    lea r10, [r12 + 16]   ; r10 = puntero a la llave RC4 (16 bytes)

    ; 6. Preparar entorno para RC4 (256 bytes de stack)
    sub rsp, 256
    mov rdi, rsp          ; rdi = S array

    ; --- RC4: KSA (Key-Scheduling Algorithm) ---
    xor rcx, rcx
ksa_init:
    mov byte [rdi + rcx], cl
    inc rcx
    cmp rcx, 256
    jl ksa_init

    xor r8, r8            ; j = 0
    xor rcx, rcx          ; i = 0
ksa_loop:
    movzx rax, byte [rdi + rcx]  ; rax = S[i]
    
    ; Obtener key[i % 16] (Nuestra clave es siempre 16 bytes)
    mov rbx, rcx
    and rbx, 15
    movzx rdx, byte [r10 + rbx]

    add r8, rax
    add r8, rdx
    and r8, 255           ; j = (j + S[i] + key[i % 16]) % 256
    
    ; Swap S[i] y S[j]
    movzx r9, byte [rdi + r8]
    mov byte [rdi + rcx], r9b
    mov byte [rdi + r8], al

    inc rcx
    cmp rcx, 256
    jl ksa_loop

    ; --- RC4: PRGA (Desencriptado al Vuelo In-situ) ---
    xor rcx, rcx          ; i = 0
    xor r8, r8            ; j = 0
    xor rbx, rbx          ; contador = 0
prga_loop:
    cmp rbx, r15
    jge prga_done

    inc rcx
    and rcx, 255

    movzx rax, byte [rdi + rcx]
    add r8, rax
    and r8, 255

    ; Swap
    movzx r9, byte [rdi + r8]
    mov byte [rdi + rcx], r9b
    mov byte [rdi + r8], al

    add ax, r9w
    and ax, 255
    
    movzx rdx, byte [rdi + rax]

    ; Desencripta directamente en la memoria mapeada de la víctima
    mov al, byte [r14 + rbx]
    xor al, dl
    mov byte [r14 + rbx], al

    inc rbx
    jmp prga_loop

prga_done:
    add rsp, 256          ; Restaurar el S-array 
    
    ; 7. Calcular el Original Entry Point (OEP) y prepararlo para el salto final
    mov r13, [r12 + 32]   ; rel_oep
    lea r14, [r12 + r13]
    mov [rbp + jmp_dest - get_rip], r14

    ; 8. Restaurar el estado milimétrico de todos los registros de la CPU
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
    popf

    ; 9. SALTO INCONDICIONAL al .text desencriptado para continuar la ejecución natural
    jmp qword [rel jmp_dest]

jmp_dest: dq 0

align 8
vars:
    ; *** ESTA FIRMA MAGICA SE BUSCARA Y PARCHEARA DESDE C ***
    rel_text    dq 0x1111111111111111
    text_size   dq 0x2222222222222222
    key1        dq 0x3333333333333333
    key2        dq 0x4444444444444444
    rel_oep     dq 0x5555555555555555
