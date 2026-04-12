# 🌲 Woody Woodpacker

**Woody Woodpacker** es un proyecto avanzado de Ciberseguridad e Ingeniería Inversa de 42 School. El objetivo principal ha sido codificar un empaquetador de archivos («packer») polimórfico al estilo de UPX, orientado específicamente a inyectar cargas parasitarias en ejecutables de formato **ELF de 64 bits** estáticos y de posición independiente (PIE) en Linux.

Un *packer* toma un ejecutable original, extrae su código ensamblador, lo cifra matemáticamente mediante criptografía simétrica y le inyecta una rutina de pre-ejecución (un payload en ensamblador) en un espacio hueco oculto del archivo original conocido como *Code Cave*. Cuando la víctima ejecuta el nuevo programa infectado (`woody`), este payload invisible toma el control temporal de la CPU, descifra el código encriptado directamente sobre la Memoria RAM y salta a la puerta de entrada original (OEP) para que el programa se inicie sin que el usuario sospeche absolutamente nada.

## 📁 Sistema de Documentación del Código

Para facilitar la comprensión técnica del proyecto, el código se ha documentado extensivamente al español. Sumérgete en las piezas clave de Woody Woodpacker leyendo los siguientes artículos interactivos:
*   [⚙️ La Maquinaria Principal (src/)](./code_src.md): Entiende el ciclo de vida del *Parser* ELF, el Cirujano de inyección y el Cerrajero criptográfico.
*   [🧩 El Plano Arquitectónico (woody.h)](./code_inc.md): Un desglose completo de cómo funciona y almacena memoria el expediente principal de estructuras en C.
*   [🧠 El Cerebro Inyectado (asm/)](./code_asm.md): Descubre la asombrosa rutina matemática de 7 pasos puramente desarrollada en lenguaje ensamblador x86_64 que viaja inyectada dentro del binario ajeno.
*   [🔐 Criptografía Dinámica RC4](./code_RC4.md): La explicación matemática bajo el capó de la magia del cifrado simétrico usando llaves estáticas y cuánticas aleatorias (`/dev/urandom`).

## 🧪 Motor de Evaluación Interactivo

El proyecto incorpora un potente evaluador automático (`test_auto.sh` y `test_eval.sh`) programado en Bash avanzado.
*   Ofrece más de 20 tests exhaustivos de límites, cubriendo fugas de memoria (`valgrind`), gestión asíncrona de I/O, encriptación manual y seguridad (detección de cabeceras ELF maliciosas, `buffer overflows`, etc.).
*   **Búsqueda Dinámica**: El evaluador hace búsquedas regex en directo usando *awk* para imprimir exactamente la línea de código C/ASM respectiva que entraña el código vital testeado ante tus ojos.

## 🎯 Tecnologías Empleadas

- **C y Assembly x86_64 (NASM)**.
- Manipulación avanzada de punteros y análisis estático de binarios.
- Inyección en Memoria RAM (`mprotect`, `mmap`, `munmap`, syscalls directas).
- Algoritmo de cifrado Rivest Cipher 4 (KSA + PRGA).

## 🚀 Uso del Empaquetador

```bash
make re
./woody_woodpacker <archivo_victima_elf_64> [llave_custom_hexadecimal_16_bytes]
```
> Esto generará tu archivo `woody` completamente listo para infiltrarse en tu sistema.
