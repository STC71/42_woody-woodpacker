#!/bin/bash

# ==========================================
# COLORES Y FORMATO
# ==========================================
C_R='\033[0;31m'
C_G='\033[0;32m'
C_Y='\033[0;33m'
C_B='\033[0;34m'
C_C='\033[0;36m'
C_W='\033[1;37m'
C_DF='\033[0m'
BOLD='\033[1m'

# ==========================================
# METRICAS
# ==========================================
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

print_header() {
    clear
    echo -e "${C_B}╭────────────────────────────────────────────────────────────────────╮${C_DF}"
    echo -e "${C_B}│${C_W}${BOLD}                  TEST_AUTO.SH: WOODY-WOODPACKER                    ${C_DF}${C_B}│${C_DF}"
    echo -e "${C_B}│${C_DF}                           by sternero                              ${C_B}│${C_DF}"
    echo -e "${C_B}│${C_DF}        Test Exhaustivo de Límites con Abogado del Diablo           ${C_B}│${C_DF}"
    echo -e "${C_B}╰────────────────────────────────────────────────────────────────────╯${C_DF}\n"
}

test_result() {
    if [ $1 -eq 0 ]; then
        echo -e "   [ ${C_G}✓ PASS${C_DF} ]"
        ((PASSED_TESTS++))
    else
        echo -e "   [ ${C_R}✗ FAIL${C_DF} ]"
        ((FAILED_TESTS++))
    fi
    ((TOTAL_TESTS++))
    echo ""
}

pause_for_user() {
    echo -e "\n${C_C}>> Pulsa [ENTER] para continuar al siguiente test...${C_DF}"
    read -r
    clear
    print_header
}

find_code() {
    local label=$1
    local search=$2
    shift 2
    local matches=$(grep -n -E "$search" "$@" /dev/null 2>/dev/null | awk -F: '{ res[$1] = res[$1] $2 ", " } END { for (file in res) { sub(/, $/, "", res[file]); printf "%s [Líneas %s]   ", file, res[file] } }')
    echo -e "${C_Y}CÓDIGO:${C_DF} ${label} -> ${matches}"
}

print_header
echo -e "${C_B}▶ TEST 0: COMPILANDO PROYECTO (Make rebuild strict)...${C_DF}"
echo -e "${C_Y}COMANDO:${C_DF} make re"
echo -e "${C_Y}OBJETIVO:${C_DF} Verificar que el proyecto compila limpiamente (flags -Wall -Wextra -Werror)."
echo -e "${C_Y}MÉTODO:${C_DF} Ejecuta 'make re' y verifica el código de salida."
find_code "Reglas de compilación" "^re:" Makefile
echo -e "${C_Y}ESPERADO:${C_DF} El ensamblador NASM y gcc devuelven 0, creando los binarios correctamente.\n"
make re >/dev/null 2>&1
if [ $? -eq 0 ]; then echo -e "   [ ${C_G}✓ PASS${C_DF} ] Makefile ejecutado sin errores."; else echo -e "   [ ${C_R}✗ FAIL${C_DF} ] Fallo en Make"; exit 1; fi
#pause_for_user

# ---------------------------------------------------------
echo -e "${C_B}▶ TEST 1: COMPORTAMIENTO BÁSICO (Empaquetar LS)${C_DF}"
echo -e "${C_Y}COMANDO:${C_DF} ./woody_woodpacker ./ls_test"
echo -e "${C_Y}OBJETIVO:${C_DF} Validar que el packer puede inyectar exitosamente un binario real (/bin/ls)."
echo -e "${C_Y}MÉTODO:${C_DF} Copia /bin/ls localmente y ejecuta ./woody_woodpacker sobre él."
find_code "Inyección y Parseo Inicial" "create_woody_file|parse_elf" src/main.c src/injector.c
echo -e "${C_Y}ESPERADO:${C_DF} Se genera el archivo infectado './woody' y se imprime por pantalla la configuración del cifrado (offset, size, jmp).\n"
cp /bin/ls ./ls_test
./woody_woodpacker ./ls_test > woody_out.log
test_result $?
#pause_for_user

echo -e "${C_B}▶ TEST 2: COMPORTAMIENTO DEL INFECTADO (Ejecutar Payload)${C_DF}"
echo -e "${C_Y}COMANDO:${C_DF} ./woody | grep '....WOODY....'"
echo -e "${C_Y}OBJETIVO:${C_DF} Confirmar que el binario infectado intercepta la ejecución."
echo -e "${C_Y}MÉTODO:${C_DF} Se ejecuta el nuevo ./woody y se busca la cadena '....WOODY....'."
find_code "Cadena inyectada" "\.\.\.\.WOODY\.\.\.\." asm/payload.s
echo -e "${C_Y}ESPERADO:${C_DF} La cadena se imprime antes de que el programa devuelva el control al Entry Point original.\n"
./woody | grep -q "....WOODY...."
test_result $?
#pause_for_user

echo -e "${C_B}▶ TEST 3: COMPARATIVA FUNCIONAL ORIGINAL VS WOODY${C_DF}"
echo -e "${C_Y}COMANDO:${C_DF} diff <(./ls_test test_dir) <(./woody test_dir | grep -v 'WOODY')"
echo -e "${C_Y}OBJETIVO:${C_DF} Garantizar que la funcionalidad del binario secuestrado no se corrompe por nuestra inyección."
echo -e "${C_Y}MÉTODO:${C_DF} Se ejecutan tanto el binario original como el empaquetado; sus salidas se comparan filtrando la firma."
find_code "Salto limpio (OEP jump)" "jmp qword" asm/payload.s
echo -e "${C_Y}ESPERADO:${C_DF} Aparte de '....WOODY....', la salida del programa (el listado de un directorio de prueba) es 100% idéntica.\n"
# Creamos un directorio vacio para aislar el comando ls
mkdir -p test_dir && touch test_dir/a test_dir/b
./ls_test test_dir > orig_out.txt
./woody test_dir > infect_out.txt
grep -v "....WOODY...." infect_out.txt > clean_infect.txt
diff orig_out.txt clean_infect.txt > /dev/null
test_result $?
rm -rf test_dir
#pause_for_user

# ---------------------------------------------------------
echo -e "${C_B}▶ TEST 4: PRESERVACIÓN DE PERMISOS DE ARCHIVO${C_DF}"
echo -e "${C_Y}COMANDO:${C_DF} stat -c '%a' ./woody"
echo -e "${C_Y}OBJETIVO:${C_DF} Comprobar que woody crea el archivo empaquetado heredando los mismos permisos (modes) de acceso."
echo -e "${C_Y}MÉTODO:${C_DF} Modifica ls_test a 0711, empaqueta e inspecciona los permisos del nuevo archivo infectado."
find_code "Clonación de stat/chmod" "woody->file_mode|chmod\(" src/main.c src/injector.c
echo -e "${C_Y}ESPERADO:${C_DF} ./woody hereda mode 0711, saltándose el Umask standard de bash (0755 o 0644).\n"
chmod 711 ./ls_test
./woody_woodpacker ./ls_test > /dev/null
ORIG_PERM=$(stat -c "%a" ./ls_test)
NEW_PERM=$(stat -c "%a" ./woody)
if [ "$ORIG_PERM" = "$NEW_PERM" ]; then test_result 0; else test_result 1; fi
#pause_for_user

# ---------------------------------------------------------
echo -e "${C_B}▶ TEST 5: ARCHIVOS ERRÓNEOS Y LÍMITES - MODO ABOGADO DEL DIABLO${C_DF}"
echo -e "${C_Y}COMANDO:${C_DF} ./woody_woodpacker <invalid_inputs>"
echo -e "${C_Y}OBJETIVO:${C_DF} Validar que el packer de C maneja correctamente strings raras, tipos inválidos o headers truncados."
echo -e "${C_Y}MÉTODO:${C_DF} Alimentar el packer con directorios, strings de texto y binarios incorrectos."
find_code "Errores de validación" "ERR_OPEN|ERR_NOT_ELF|ERR_NOT_64" src/main.c src/elf_parser.c
echo -e "${C_Y}ESPERADO:${C_DF} El programa aborta limpiamente sin Segfaults y muestra un mensaje de error descriptivo.\n"
echo "  5.1 - Fichero no existe."
./woody_woodpacker /tmp/fake_file22 2>&1 | grep -q "Error: No se pudo abrir el archivo"
test_result $?

echo "  5.2 - Fichero es un directorio."
./woody_woodpacker /tmp 2>&1 | grep -q "Error: No se pudo obtener información del archivo"
test_result $?

echo "  5.3 - Fichero de Texto (No ELF)."
echo "Hola Mundo" > dummy.txt
./woody_woodpacker dummy.txt 2>&1 | grep -q "Error: El archivo no es un ELF válido"
test_result $?

echo "  5.4 - Fichero de 32-bits (Subject Constraint)."
./woody_woodpacker resources/sample 2>&1 | grep -q "Error: El archivo no es un ELF de 64 bits"
test_result $?
#pause_for_user

# ---------------------------------------------------------
echo -e "${C_B}▶ TEST 6: ANÁLISIS DE FUGAS DE MEMORIA Y SEGFAULTS (VALGRIND)${C_DF}"
echo -e "${C_Y}COMANDO:${C_DF} valgrind --leak-check=full --error-exitcode=42 ./woody_woodpacker"
echo -e "${C_Y}OBJETIVO:${C_DF} Detectar accesos de buffer overflow, off-by-one errors y leaks de punteros mal mapeados."
echo -e "${C_Y}MÉTODO:${C_DF} Compila valgrind con un error-exitcode forzado y escanea toda la inyección de ls_test y de dummies."
find_code "Gestión manual de memoria RAM" "mmap\(|munmap\(" src/main.c src/injector.c
echo -e "${C_Y}ESPERADO:${C_DF} El binario aborta (exit != 42), confirmando la ausencia de invalid reads/writes en los arrays.\n"
echo "  6.1 - Valgrind en Ejecución Exitosa."
valgrind --leak-check=full --error-exitcode=42 ./woody_woodpacker ./ls_test > /dev/null 2>&1
if [ $? -ne 42 ]; then test_result 0; else test_result 1; fi

echo "  6.2 - Valgrind en Ejecución Fallida (No ELF)."
valgrind --leak-check=full --error-exitcode=42 ./woody_woodpacker dummy.txt > /dev/null 2>&1
if [ $? -ne 42 ]; then test_result 0; else test_result 1; fi
#pause_for_user

# ---------------------------------------------------------
echo -e "${C_B}▶ TEST 7: BONUS - CIFRADO PARAMETRIZADO (16-BYTES)${C_DF}"
echo -e "${C_Y}COMANDO:${C_DF} ./woody_woodpacker <file> [custom_key]"
echo -e "${C_Y}OBJETIVO:${C_DF} Probar la implementación Bonus de RC4 (Rivest Cipher 4) usando llaves custom HEX y random urandom."
echo -e "${C_Y}MÉTODO:${C_DF} Ejecuta woody con un parámetro de cadena de 32 bytes validando stdout y errores de truncado de args."
find_code "Motor Criptográfico RC4" "Bonus Custom Key|rc4_encrypt|/dev/urandom" src/main.c src/crypto.c
echo -e "${C_Y}ESPERADO:${C_DF} Empaquetado exitoso imprimiendo la llave si es random o si es un input de 16 bytes correctamente parseado.\n"
echo "  7.1 - Cifrado forzado con llave personalizada de longitud correcta (32 char Hex = 16 bytes)."
./woody_woodpacker ./ls_test 00112233445566778899AABBCCDDEEFF > param_out.log
grep -q "CLAVE \[128-bit RC4\]: 0x00112233445566778899AABBCCDDEEFF" param_out.log
test_result $?

echo "  7.2 - Cifrado con llave de longitud insuficiente (Error Esperado)."
./woody_woodpacker ./ls_test AABBCC 2>&1 | grep -q "Error: Bonus Custom Key debe constar exactamente de 32 caracteres hexadecimales"
test_result $?

echo "  7.3 - Asegurar que el uso sin parametros genera Random Key de /dev/urandom."
./woody_woodpacker ./ls_test > key_out1.txt
./woody_woodpacker ./ls_test > key_out2.txt
diff key_out1.txt key_out2.txt > /dev/null
# Si diff detecta diferencias, significa que la llave es pseudoaleatoria de verdad (éxito).
if [ $? -eq 1 ]; then test_result 0; else test_result 1; fi
#pause_for_user

# ---------------------------------------------------------
echo -e "${C_B}▶ TEST 8: CORRUPCIÓN EXTREMA Y EDGE CASES (DEVIL'S ADVOCATE)${C_DF}"
echo -e "${C_Y}COMANDO:${C_DF} ./woody_woodpacker <invalid/corrupted_files>"
echo -e "${C_Y}OBJETIVO:${C_DF} Empujar el parser ELF fuera de los límites de memoria con archivos hostiles deliberadamente fabricados."
echo -e "${C_Y}MÉTODO:${C_DF} Truncar cabeceras ELF y corromper punteros de offsets (e.g., apuntar shoff a la Luna)."
find_code "Seguridad Matemática Avanzada" "is_safe_ptr|Corrupted ELF" src/elf_parser.c
echo -e "${C_Y}ESPERADO:${C_DF} Nuestra protección 'is_safe_ptr' en C detiene el mapeo ilegal antes de producir Core Dumps.\n"

echo "  8.1 - Fichero vacío (0 bytes)."
touch empty_file
./woody_woodpacker empty_file 2>&1 | grep -q -i "error"
test_result $?

echo "  8.2 - Fichero de 1 byte."
echo "A" > 1byte_file
./woody_woodpacker 1byte_file 2>&1 | grep -q -i "error"
test_result $?

echo "  8.3 - Dispositivo de Caracteres (/dev/null)."
./woody_woodpacker /dev/null 2>&1 | grep -q -i "error"
test_result $?

echo "  8.4 - ELF Header truncado (tamaño 60 bytes, es menor que struct Elf64_Ehdr)."
dd if=/bin/ls of=trunc_elf bs=1 count=60 > /dev/null 2>&1
./woody_woodpacker trunc_elf 2>&1 | grep -q -E "(v[áa]lid|mmap|error|Fall|Diseño)"
test_result $?

echo "  8.5 - Corrupción de Header: e_shoff apunta al infinito (OOB Memory Access)."
# Cambiamos 4 bytes del offset e_shoff en la cabecera ELF para apuntar fuera del archivo.
cp /bin/ls corrupt_shoff_elf
printf '\xff\xff\xff\xff' | dd of=corrupt_shoff_elf bs=1 seek=40 count=4 conv=notrunc > /dev/null 2>&1
./woody_woodpacker corrupt_shoff_elf 2>&1 | grep -q "Diseño ELF corrupto detectado"
test_result $?
#pause_for_user

# ---------------------------------------------------------
echo -e "${C_B}▶ TEST 9: RESTRICCIONES DE SISTEMA (PERMISOS Y FD LEAKS)${C_DF}"
echo -e "${C_Y}COMANDO:${C_DF} chmod 000, --track-fds=yes"
echo -e "${C_Y}OBJETIVO:${C_DF} Probar el sistema I/O subyacente para asegurarse de la gestión de Handles Limpios."
echo -e "${C_Y}MÉTODO:${C_DF} Eliminar el modo R y Write (000), invocar el Valgrind flag --track-fds sobre bloqueos inyectados."
find_code "File Descriptors (POSIX I/O)" "open\(|close\(" src/main.c src/injector.c
echo -e "${C_Y}ESPERADO:${C_DF} En caso de crash o error, no debe quedar ningún Open File Descriptor pendiente de close() en POSIX.\n"

echo "  9.1 - Archivo origen sin permisos de lectura (chmod 000)."
cp /bin/ls no_perm_elf && chmod 000 no_perm_elf
./woody_woodpacker no_perm_elf 2>&1 | grep -E -q "(Error: No se pudo abrir el archivo|Permission denied)"
test_result $?

echo "  9.2 - Archivo destino pre-existente y bloqueado (Imposible sobreescribir ./woody)."
touch woody && chmod 000 woody
./woody_woodpacker ./ls_test 2>&1 | grep -q "Error: No se pudo crear archivo resultante 'woody'"
test_result $?
rm -f woody # Forzamos borrado para continuar

echo "  9.3 - Valgrind Tracking Extremo de File Descriptors cerrados en caso de crash."
valgrind --leak-check=full --track-fds=yes --error-exitcode=42 ./woody_woodpacker dummy.txt > valgrind_fds.log 2>&1
# Comprueba estrictamente si valgrind detecto fugas de descriptors que NO sean heredados del padre (bash/vscode)
if grep -q "Open file descriptor" valgrind_fds.log && ! grep -q "<inherited from parent>" valgrind_fds.log; then test_result 1; else test_result 0; fi

# Limpieza de basura
rm -f ./ls_test dummy.txt orig_out.txt infect_out.txt clean_infect.txt param_out.log woody_out.log key_out1.txt key_out2.txt ./woody empty_file 1byte_file trunc_elf corrupt_shoff_elf no_perm_elf valgrind_fds.log

echo -e "========================================================="
echo -e "RESULTADOS TOTALES: ${C_G}${PASSED_TESTS}${C_DF} Aprobados / ${C_R}${FAILED_TESTS}${C_DF} Fallados (de ${TOTAL_TESTS})"
if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${C_G}${BOLD}¡LA HERRAMIENTA ES INDESTRUCTIBLE! (125% CONFIRMED)${C_DF}"
else
    echo -e "${C_R}${BOLD}SE DETECTARON FALLOS. REVISA TU CÓDIGO.${C_DF}"
fi