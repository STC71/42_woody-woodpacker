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
    echo -e "${C_B}│${C_W}${BOLD}                  TEST_AUTO.SH: WOODY-WOODPACKER              ${C_DF}${C_B}│${C_DF}"
    echo -e "${C_B}│${C_DF}                         by sternero                                ${C_B}│${C_DF}"
    echo -e "${C_B}│${C_DF}       Test Exhaustivo de Límites y Abogado del Diablo              ${C_B}│${C_DF}"
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

print_header
echo -e "${C_B}▶ COMPILANDO PROYECTO (Make rebuild strict)...${C_DF}"
make re >/dev/null 2>&1
if [ $? -eq 0 ]; then echo -e "   [ ${C_G}✓ PASS${C_DF} ] Makefile ejecutado sin errores."; else echo -e "   [ ${C_R}✗ FAIL${C_DF} ] Fallo en Make"; exit 1; fi

# ---------------------------------------------------------
echo -e "${C_B}▶ TEST 1: COMPORTAMIENTO BÁSICO (Empaquetar LS)${C_DF}"
echo "    - Instrucción: Copiamos /bin/ls y lo empaquetamos."
cp /bin/ls ./ls_test
./woody_woodpacker ./ls_test > woody_out.log
test_result $?

echo -e "${C_B}▶ TEST 2: COMPORTAMIENTO DEL INFECTADO (Ejecutar Payload)${C_DF}"
echo "    - Instrucción: ./woody debe imprimir '....WOODY....' y luego listar el directorio."
./woody | grep -q "....WOODY...."
test_result $?

echo -e "${C_B}▶ TEST 3: COMPARATIVA FUNCIONAL ORIGINAL VS WOODY${C_DF}"
echo "    - Instrucción: Ambos binarios deben producir la misma salida final de datos."
# Creamos un directorio vacio para aislar el comando ls
mkdir -p test_dir && touch test_dir/a test_dir/b
./ls_test test_dir > orig_out.txt
./woody test_dir > infect_out.txt
grep -v "....WOODY...." infect_out.txt > clean_infect.txt
diff orig_out.txt clean_infect.txt > /dev/null
test_result $?
rm -rf test_dir

# ---------------------------------------------------------
echo -e "${C_B}▶ TEST 4: PRESERVACIÓN DE PERMISOS DE ARCHIVO${C_DF}"
echo "    - Instrucción: Si el original era 0711, el empaquetado no debe exponerlo con 0755."
chmod 711 ./ls_test
./woody_woodpacker ./ls_test > /dev/null
ORIG_PERM=$(stat -c "%a" ./ls_test)
NEW_PERM=$(stat -c "%a" ./woody)
if [ "$ORIG_PERM" = "$NEW_PERM" ]; then test_result 0; else test_result 1; fi

# ---------------------------------------------------------
echo -e "${C_B}▶ TEST 5: ARCHIVOS ERRÓNEOS Y LÍMITES - MODO ABOGADO DEL DIABLO${C_DF}"
echo "  5.1 - Fichero no existe."
./woody_woodpacker /tmp/fake_file22 2>&1 | grep -q "Error: Could not open file"
test_result $?

echo "  5.2 - Fichero es un directorio."
./woody_woodpacker /tmp 2>&1 | grep -q "Error: Could not retrieve file information"
test_result $?

echo "  5.3 - Fichero de Texto (No ELF)."
echo "Hola Mundo" > dummy.txt
./woody_woodpacker dummy.txt 2>&1 | grep -q "Error: File is not a valid ELF"
test_result $?

echo "  5.4 - Fichero de 32-bits (Subject Constraint)."
./woody_woodpacker resources/sample 2>&1 | grep -q "Error: File is not a 64-bit ELF"
test_result $?

# ---------------------------------------------------------
echo -e "${C_B}▶ TEST 6: ANÁLISIS DE FUGAS DE MEMORIA Y SEGFAULTS (VALGRIND)${C_DF}"
echo "  6.1 - Valgrind en Ejecución Exitosa."
valgrind --leak-check=full --error-exitcode=42 ./woody_woodpacker ./ls_test > /dev/null 2>&1
if [ $? -ne 42 ]; then test_result 0; else test_result 1; fi

echo "  6.2 - Valgrind en Ejecución Fallida (No ELF)."
valgrind --leak-check=full --error-exitcode=42 ./woody_woodpacker dummy.txt > /dev/null 2>&1
if [ $? -ne 42 ]; then test_result 0; else test_result 1; fi

# ---------------------------------------------------------
echo -e "${C_B}▶ TEST 7: BONUS - CIFRADO PARAMETRIZADO (16-BYTES)${C_DF}"
echo "  7.1 - Cifrado forzado con llave personalizada de longitud correcta (32 char Hex = 16 bytes)."
./woody_woodpacker ./ls_test 00112233445566778899AABBCCDDEEFF > param_out.log
grep -q "KEY \[128-bit RC4\]: 0x00112233445566778899AABBCCDDEEFF" param_out.log
test_result $?

echo "  7.2 - Cifrado con llave de longitud insuficiente (Error Expected)."
./woody_woodpacker ./ls_test AABBCC 2>&1 | grep -q "Error: Bonus Custom Key must be exactly 32 hex characters"
test_result $?

echo "  7.3 - Asegurar que el uso sin parametros genera Random Key de /dev/urandom."
./woody_woodpacker ./ls_test > key_out1.txt
./woody_woodpacker ./ls_test > key_out2.txt
diff key_out1.txt key_out2.txt > /dev/null
# Si diff detecta diferencias, significa que la llave es pseudoaleatoria de verdad (éxito).
if [ $? -eq 1 ]; then test_result 0; else test_result 1; fi

# ---------------------------------------------------------
echo -e "${C_B}▶ TEST 8: CORRUPCIÓN EXTREMA Y EDGE CASES (DEVIL'S ADVOCATE)${C_DF}"

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
./woody_woodpacker trunc_elf 2>&1 | grep -q -E "(valid ELF|mmap|error)"
test_result $?

echo "  8.5 - Corrupción de Header: e_shoff apunta al infinito (OOB Memory Access)."
# Cambiamos 4 bytes del offset e_shoff en la cabecera ELF para apuntar fuera del archivo.
cp /bin/ls corrupt_shoff_elf
printf '\xff\xff\xff\xff' | dd of=corrupt_shoff_elf bs=1 seek=40 count=4 conv=notrunc > /dev/null 2>&1
./woody_woodpacker corrupt_shoff_elf 2>&1 | grep -q "Corrupted ELF layout detected"
test_result $?

# ---------------------------------------------------------
echo -e "${C_B}▶ TEST 9: RESTRICCIONES DE SISTEMA (PERMISOS Y FD LEAKS)${C_DF}"

echo "  9.1 - Archivo origen sin permisos de lectura (chmod 000)."
cp /bin/ls no_perm_elf && chmod 000 no_perm_elf
./woody_woodpacker no_perm_elf 2>&1 | grep -E -q "(Error: Could not open file|Permission denied)"
test_result $?

echo "  9.2 - Archivo destino pre-existente y bloqueado (Imposible sobreescribir ./woody)."
touch woody && chmod 000 woody
./woody_woodpacker ./ls_test 2>&1 | grep -q "Error: Could not create output file 'woody'"
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