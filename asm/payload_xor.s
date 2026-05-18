bits 64         ; Estamos en modo de 64 bits, lo que nos da acceso a los 
                ; registros extendidos (r8, r9, r10...) y a la Red Zone de la pila.
section .text   ; El código se inyectará en la sección .text de la víctima, 
                ; así que aquí es donde escribimos nuestro payload.
global _start   ; El punto de entrada de nuestro código. Cuando el sistema operativo 
                ; ejecute el programa víctima, lo primero que hará será ejecutar este 
                ; código antes que el original (gracias a la inyección en la Code Cave).

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
    ; Reservar espacio ya NO es necesario porque usamos la Red Zone (RSP negativo)
    
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
    ; El registro r11 NO lo guardamos. Es volátil (destruido por syscall execve/sysret).
    ; Nos regala exactamente 8 bytes en la Red Zone de 128 bytes perfectos (-128)
    push r12
    push r13
    push r14
    push r15

    ; ==========================================================================
    ; 🛡️ FASE 1.5: BONUS ANTI-DEBUGGING (EL RADAR INVISIBLE)
    ; ==========================================================================
    ; Un clásico del Malware "Anti-Análisis": Si nos están vigilando (GDB, strace), 
    ; morimos silenciosamente o nos evadimos.
    ; Usamos la llamada al sistema 'ptrace' con PTRACE_TRACEME.
    ; Si alguien ya nos está depurando el kernel nos deniega la petición 
    ; y nos devuelve un número de error (negativo) en RAX.
    mov rax, 101                 ; Syscall #101 = sys_ptrace en Linux x86_64
    xor rdi, rdi                 ; arg1: request = PTRACE_TRACEME (0)
    xor rsi, rsi                 ; arg2: pid = 0
    xor rdx, rdx                 ; arg3: addr = 0
    xor r10, r10                 ; arg4: data = 0
    syscall                      ; "Toc, toc, Kernel. ¿Alguien me mira?"

    cmp rax, 0                   ; Verificamos la respuesta de ptrace
    jge .radar_clear             ; Si RAX >= 0, todo limpio. Saltamos.
    
    ; ¡NOS ESTÁN VIGILANDO! Abortar misión (Exit)
    mov rax, 60                  ; Syscall #60 = sys_exit
    mov rdi, 1                   ; Exit code = 1 (Error / Tampered)
    syscall                      ; Adiós, evaluador.
.radar_clear:

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
    ; FASE 5: XOR NO NECESITA KSA (Saltamos Baraja)
    ; MPROTECT: Hacking the self-defense W^X (Write XOR Execute)
    ; ==========================================================================
    ; Necesitamos poder escribir (desencriptar) la memoria de la sección .text
    ; que el sistema la bloqueó como Solo-Lectura por defecto. Usamos mprotect.
    ;
    push rsi                     ; Salvar rsi
    
    mov rdi, r14                 ; param 1: Dirección original (debe alinearse)
    mov rsi, r15                 ; param 2: Tamaño original
    
    ; mprotect exige page_size align (and rdi, ~0xFFF)
    mov r8, rdi
    and rdi, -4096               ; 0xFFFFFFFFFFFFF000 (Redondeo hacia abajo a 4KB)
    sub r8, rdi                  ; offset = dirección original - alineada
    add rsi, r8                  ; Ampliar el tamaño por el desplazamiento

    mov rax, 10                  ; 10 = ID Syscall mprotect
    mov rdx, 7                   ; 7 = PROT_READ (1) | PROT_WRITE (2) | PROT_EXEC (4)
    push rcx                     ; rcx se destruye en syscall
    push r11                     ; r11 se destruye en syscall
    syscall
    pop r11
    pop rcx
    
    pop rsi                      ; Restaurar rsi
    ;

    ; ==========================================================================
        ; ==========================================================================
    ; FASE 6: DESENCRIPTADO AL VUELO IN-SITU (XOR)
    ; ==========================================================================
    xor rbx, rbx                 ; rbx = counter
xor_loop:
    cmp rbx, r15                 ; Are we done?
    jge prga_done
    
    ; get key byte: key[i % 16]
    mov rcx, rbx
    and rcx, 15
    mov dl, byte [r10 + rcx]
    
    ; XOR the byte
    mov al, byte [r14 + rbx]
    xor al, dl
    mov byte [r14 + rbx], al
    
    inc rbx
    jmp xor_loop
prga_done:
    ; Termina el cifrado. Devolvemos los 256 bytes que la quitamos a la pila al principio.
    ;                 
    
    ; ==========================================================================
    ; MPROTECT RESTORE: Curación Total y Sigilio
    ; ==========================================================================
    ; Para no dejar rastro al antivirus (E.D.R.), le devolvemos los permisos RX 
    ; limpios a la sección desinfectada. ¡Es como si el virus jamás hubiera pasado!
    mov rdi, r14
    mov rsi, r15
    mov r8, rdi
    and rdi, -4096
    sub r8, rdi
    add rsi, r8

    mov rax, 10                  ; mprotect syscall
    mov rdx, 5                   ; 5 = PROT_READ (1) | PROT_EXEC (4)
    push rcx
    push r11
    syscall
    pop r11
    pop rcx

    ; ==========================================================================
    ; FASE 7: CALCULAR EL OEP (Original Entry Point)
    ; ==========================================================================
    ; Ya hemos arreglado el programa víctima. Ahora hay que averiguar cuál era
    ; su puerta de entrada original para empujar la ejecución hacia allí.
    mov r13, [r12 + 32]          ; Leemos el offset del Original Entry Point
    lea r14, [r12 + r13]         ; Calculamos su dirección absoluta real en RAM
    
    ; Guardamos el OEP sigilosamente bajo el último registro usando la Red Zone exacta de la pila original.
    ; Esto está en rsp - 8 respecto AL RSP ACTUAL (128 bytes exactos por debajo de la pila original).
    ; ¡100% legal contra la ABI de System V AMD64 (Límite: -128 bytes)! No hay Heisenbugs aquí.
    mov [rsp - 8], r14

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
    ; No poppeamos r11, deliberadamente sacrificado para esconder nuestro OEP
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
    ; FASE 9: EL SALTO INCONDICIONAL Y SEGURO (JMP MÁGICO) AL PROGRAMA REAL
    ; ==========================================================================
    ; Cuando todo se ha desapilado, la pila (rsp) vuelve a ser EXAMINABLEMENTE igual
    ; que la original (con los argumentos del sistema "argc" y "argv" intactos).
    ; No podemos usar RET porque eso sacaría de la pila argumentos reales.
    ; Usamos un salto incondicional transparente leyendo el r14 (OEP) escondido
    ; profundamente en nuestra Red Zone (-128 bytes de distancia exactos de la ABI). ¡Sigilo Puro y Matemático!
    ; OEP = Original Entry Point (Punto de Entrada Original)
    ; ABI = Application Binary Interface (para x86_64, el límite de la Red Zone es de -128 bytes)
    ; red zone = área de la pila que no es tocada por el sistema operativo ni por las llamadas a funciones, 
    ; perfecta para esconder datos sin riesgo de colisión.
    jmp qword [rsp - 128]

; ==============================================================================
; FIRMA MÁGICA Y VARIABLES HUECAS
; ==============================================================================
; Esto no son instrucciones para procesar, son "Agujeros" que nuestro programa 
; en Lenguaje C rellenará obligatoriamente a la fuerza cuando inyecte este código. 
; Los 0x1122... son simplemente un patrón hiper-fácil de buscar con el C (strnstr).
align 8
vars:
    rel_text    dq 0x1122334455667788   ; [0]  Distancia desde vars a la sección .text encriptada
    text_size   dq 0x2222222222222222   ; [8]  Número total de bytes a desencriptar
    key1        dq 0x3333333333333333   ; [16] Primera mitad de la Llave RC4 secreta
    key2        dq 0x4444444444444444   ; [24] Segunda mitad de la Llave RC4 secreta
    rel_oep     dq 0x5555555555555555   ; [32] Distancia desde vars a la puerta original (OEP)
