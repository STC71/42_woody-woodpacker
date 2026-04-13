bits 64
section .text
global _start

; ==============================================================================
; 🦠 WOODY WOODPACKER PAYLOAD - EL POLIZÓN EN ENSAMBLADOR
; ==============================================================================
; Este es el código que se inyecta directamente en las "Code Caves" (espacios 
; huecos) de los archivos ELF víctimas. Su trabajo es arrancar antes que el 
; programa original, descifrarlo en la memoria RAM y devolverle el control 
; sin causar una violación de segmento (Segmentation Fault).
;
; Desarrollado para operaciones puramente sigilosas.
; ==============================================================================

_start:
    ; ==========================================================================
    ; FASE 1: GUARDAR EL MUNDO (PUSH)
    ; ==========================================================================
    ; Para que la víctima no sospeche nada ni crashee, debemos preservar la 
    ; configuración exacta de cómo estaban los registros del procesador (RDI, 
    ; RSI, RAX...) justo en el momento en el que el sistema operativo iba a 
    ; arrancar el software. Apilamos (push) absolutamente todo.
    pushf           ; Guarda los Flags (Zero Flag, Carry Flag, etc.)
    push rax        ; Acumulador original
    push rbx        ; Base
    push rcx        ; Contador
    push rdx        ; Datos
    push rbp        ; Base Pointer de la pila
    push rsi        ; Índice fuente
    push rdi        ; Índice destino
    push r8         ; Registros de propósito general extendidos (x86_64)
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; ==========================================================================
    ; FASE 2: DEJAR NUESTRA FIRMA (SYSCALL WRITE)
    ; ==========================================================================
    ; En lugar de arriesgarnos a buscar funciones complejas (como printf), 
    ; hablamos directamente con el Sistema Operativo (Kernel) usando 'syscall'.
    call get_msg                 ; Llamamos a get_msg. Esto empuja la dirección de "....WOODY...." a la pila.
msg: db "....WOODY....", 10      ; Nuestra firma seguida de un salto de línea (10 en ASCII)
get_msg:
    pop rsi                      ; Desapilamos la dirección de memoria de nuestro mensaje hacia RSI.
    mov rax, 1                   ; 1 = ID de la System Call 'write' en Linux.
    mov rdi, 1                   ; 1 = File Descriptor para la salida estándar (Stdout / Pantalla).
    mov rdx, 14                  ; 14 = Longitud de nuestro mensaje en bytes.
    syscall                      ; ¡Imprimimos en pantalla!

    ; ==========================================================================
    ; FASE 3: ENCONTRARSE EN EL ESPACIO (Detección de PIE / RIP Relativo)
    ; ==========================================================================
    ; "Position Independent Executable" significa que el programa se carga en 
    ; direcciones aleatorias de la RAM. No sabemos dónde nos han inyectado.
    call get_rip                 ; Al hacer call, empujamos a la pila la dirección de la siguiente línea (get_rip)
get_rip:
    pop rbp                      ; ¡Eureka! 'rbp' ahora sabe exactamente en qué dirección de memoria RAM está el payload.

    ; ==========================================================================
    ; FASE 4: LOCALIZAR LAS COORDENADAS SECRETAS (La Firma Mágica Inyectada)
    ; ==========================================================================
    ; Nuestro empaquetador en 'C' ha escrito al final de este ensamblador 
    ; unos números clave (la llave, cuánto encriptó y dónde está).
    ; Aquí calculamos la distancia entre dónde estamos (rbp) y dónde están
    ; las variables (vars) usando simple matemática de punteros.
    lea r12, [rbp + vars - get_rip]

    ; Leemos los datos de nuestra "Firma Mágica":
    mov r13, [r12 + 0]           ; Leemos Offset relativo hacia la sección `.text` cifrada.
    lea r14, [r12 + r13]         ; r14 = Dirección mental absoluta en RAM de la sección cifrada.

    mov r15, [r12 + 8]           ; r15 = Cuántos bytes ocupa el `.text` original (Tamaño).
    lea r10, [r12 + 16]          ; r10 = Puntero hacia nuestra Llave RC4 secreta de 16 bytes.

    ; ==========================================================================
    ; FASE 5: PREPARAR LA BATIDORA DEL RC4 (Reservar Memoria en la Pila)
    ; ==========================================================================
    ; El algoritmo RC4 necesita una baraja de 256 cartas (256 bytes libres).
    ; En lugar de usar 'malloc', simplemente le robamos espacio a la pila (Stack).
    sub rsp, 256                 ; Hacemos hueco para 256 bytes.
    mov rdi, rsp                 ; rdi = Nuestro Array 'S' (la baraja).

    ; --- KSA (Key-Scheduling Algorithm) inicialización ---
    ; Llenamos la baraja con cartas ordenadas (0, 1, 2, 3... hasta 255).
    xor rcx, rcx                 ; rcx = 0
ksa_init:
    mov byte [rdi + rcx], cl     ; S[rcx] = rcx
    inc rcx                      ; rcx++
    cmp rcx, 256                 ; ¿Hemos llegado a 256?
    jl ksa_init                  ; Si no, repite el bucle.

    ; --- KSA mezcla ---
    ; Desordenamos la baraja usando nuestra llave secreta de 16 bytes.
    xor r8, r8                   ; j = 0
    xor rcx, rcx                 ; i = 0
ksa_loop:
    movzx rax, byte [rdi + rcx]  ; rax = S[i]
    
    ; Obtener letra de la llave: key[i % 16]
    mov rbx, rcx
    and rbx, 15                  ; rbx = i % 16 (la and rápida para base 16)
    movzx rdx, byte [r10 + rbx]  ; rdx = valor de la letra de la llave

    ; Fórmula RC4 KSA: j = (j + S[i] + key[i % 16]) % 256
    add r8, rax
    add r8, rdx
    and r8, 255                  
    
    ; Intercambio (Swap) entre S[i] y S[j]
    movzx r9, byte [rdi + r8]
    mov byte [rdi + rcx], r9b
    mov byte [rdi + r8], al

    inc rcx                      ; i++
    cmp rcx, 256
    jl ksa_loop                  ; Seguir desordenando hasta 256

    ; ==========================================================================
    ; FASE 6: PRGA - DESENCRIPTADO AL VUELO IN-SITU (XOR Mágico)
    ; ==========================================================================
    ; Ahora recorremos el código bloqueado del programa original byte a byte 
    ; y lo desencriptamos matemáticamente directamente en la propia RAM.
    xor rcx, rcx                 ; i = 0
    xor r8, r8                   ; j = 0
    xor rbx, rbx                 ; rbx servirá de contador principal (de 0 a text_size)
prga_loop:
    cmp rbx, r15                 ; ¿Hemos desencriptado ya todo el bloque?
    jge prga_done                ; Si sí, ¡hemos terminado, volvemos a la normalidad!

    inc rcx                      ; i = (i + 1)
    and rcx, 255                 ; % 256

    movzx rax, byte [rdi + rcx]  ; rax = S[i]
    add r8, rax
    and r8, 255                  ; j = (j + S[i]) % 256

    ; Intercambio dinámico (Swap) de la baraja PRGA
    movzx r9, byte [rdi + r8]
    mov byte [rdi + rcx], r9b
    mov byte [rdi + r8], al

    add ax, r9w
    and ax, 255                  ; (S[i] + S[j]) % 256
    
    movzx rdx, byte [rdi + rax]  ; Letra mágica extraída de la baraja

    ; ** EL MILAGRO (XOR) **
    ; Tomamos el byte infectado de la aplicación víctima (r14 + rbx)...
    mov al, byte [r14 + rbx]
    ; ... lo empujamos contra nuestra Letra Mágica (XOR)...
    xor al, dl
    ; ... y sobrescribimos el hueco de la memoria con el código natural ya limpio.
    mov byte [r14 + rbx], al

    inc rbx                      ; Siguiente byte del programa original
    jmp prga_loop

prga_done:
    ; Termina el cifrado. Devolvemos los 256 bytes que la quitamos a la pila al principio.
    add rsp, 256                 
    
    ; ==========================================================================
    ; FASE 7: CALCULAR EL OEP (Original Entry Point)
    ; ==========================================================================
    ; Ya hemos arreglado el programa víctima. Ahora hay que averiguar cuál era
    ; su puerta de entrada original para empujar la ejecución hacia allí.
    mov r13, [r12 + 32]          ; Leemos el offset del Original Entry Point
    lea r14, [r12 + r13]         ; Calculamos su dirección absoluta real en RAM
    mov [rbp + jmp_dest - get_rip], r14 ; La escribimos en la variable de salto final

    ; ==========================================================================
    ; FASE 8: RESTAURAR EL MUNDO (POP) Y EL BORRADO DE HUELLAS
    ; ==========================================================================
    ; Sacamos de la pila, EN ORDEN INVERSO EXACTO al principio, todos y cada  
    ; uno de los registros. Para la aplicación víctima parecerá que el tiempo 
    ; se congeló por un milisegundo y volvió a arrancar como si nada.
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
    popf                         ; Lo último es restaurar los Flags inalterados.

    ; ==========================================================================
    ; FASE 8: EL SALTO INCONDICIONAL (JMP) AL PROGRAMA REAL
    ; ==========================================================================
    ; Y con este simple salto "JMP", nos transportamos mágicamente a las entrañas
    ; sanas del programa original. ¡Misión cumplida!
    jmp qword [rel jmp_dest]

jmp_dest: dq 0                   ; Aquí guardamos temporalmente el OEP real.

; ==============================================================================
; FIRMA MÁGICA Y VARIABLES HUECAS
; ==============================================================================
; Esto no son instrucciones para procesar, son "Agujeros" que nuestro programa 
; en Lenguaje C rellenará obligatoriamente a la fuerza cuando inyecte este código. 
; Los 0x111... son simplemente un patrón hiper-fácil de buscar con el C (strnstr).
align 8
vars:
    rel_text    dq 0x1111111111111111   ; [0]  Distancia desde vars a la sección .text encriptada
    text_size   dq 0x2222222222222222   ; [8]  Número total de bytes a desencriptar
    key1        dq 0x3333333333333333   ; [16] Primera mitad de la Llave RC4 secreta
    key2        dq 0x4444444444444444   ; [24] Segunda mitad de la Llave RC4 secreta
    rel_oep     dq 0x5555555555555555   ; [32] Distancia desde vars a la puerta original (OEP)
