# 🌲 Woody Woodpacker

<div align="center">

![42 School](https://img.shields.io/badge/42-School-000000?style=for-the-badge&logo=42&logoColor=white)
![Status](https://img.shields.io/badge/Estado-125%25_Completado-success?style=for-the-badge)
![Language C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![Assembly](https://img.shields.io/badge/NASM_Assembly-654FF0?style=for-the-badge)
![Bash](https://img.shields.io/badge/Bash-4EAA25?style=for-the-badge&logo=gnu-bash&logoColor=white)
![Linux](https://img.shields.io/badge/Linux_MultiArch-FCC624?style=for-the-badge&logo=linux&logoColor=black)
<br><br>
<a href="https://www.youtube.com/watch?v=41c32jbyu7g&list=PLSxbCzfDEDkj3JrCzdbZDaZbPBXbJ9rGZ">📺 Playlist de referencia sobre conceptos relacionados</a>

</div>

**Woody Woodpacker** es un proyecto avanzado de Ciberseguridad e Ingeniería Inversa de **[42 School](https://youtu.be/-izVQ6fNCkM?si=5cVfPTW1ceJoQ2cr)**. El objetivo principal ha sido codificar un empaquetador de archivos («packer») polimórfico al estilo de UPX, orientado específicamente a inyectar cargas parasitarias en ejecutables de formato **ELF de 32 y 64 bits** estáticos y de posición independiente (PIE) en Linux.

Un *packer* toma un ejecutable original, extrae su código ensamblador, lo cifra matemáticamente mediante criptografía simétrica y le inyecta una rutina de pre-ejecución (un payload en ensamblador) en un espacio hueco oculto del archivo original conocido como *Code Cave*. Cuando la víctima ejecuta el nuevo programa infectado (`woody`), este payload invisible toma el control temporal de la CPU, descifra el código encriptado directamente sobre la Memoria RAM y salta a la puerta de entrada original (OEP) para que el programa se inicie sin que el usuario sospeche absolutamente nada.

## 📁 Sistema de Documentación del Código

Para facilitar la comprensión técnica del proyecto, el código se ha documentado extensivamente. Sumérgete en las piezas clave de Woody Woodpacker leyendo los siguientes artículos interactivos:
*   [⚙️ La Maquinaria Principal (src/)](./code_src.md): Entiende el ciclo de vida del *Parser* ELF, el Cirujano de inyección y el Cerrajero criptográfico.
*   [🧩 El Plano Arquitectónico (woody.h)](./code_inc.md): Un desglose completo de cómo funciona y almacena memoria el expediente principal de estructuras en C.
*   [🧠 El Cerebro Inyectado (asm/)](./code_asm.md): Descubre la asombrosa rutina matemática de 7 pasos puramente desarrollada en lenguaje ensamblador x86_64 que viaja inyectada dentro del binario ajeno.
*   [🔐 Criptografía Dinámica RC4](./code_RC4.md): La explicación matemática bajo el capó de la magia del cifrado simétrico usando llaves estáticas y cuánticas aleatorias (`/dev/urandom`).
*   [🛠️ El Motor de Construcción (Makefile)](./code_makefile.md): Explicación detallada y sencilla del sistema de construcción y automatización del proyecto.
*   [🧪 Motor de Tests (test_auto.sh / test_eval.sh)](./code_test.md): Guía completa de la batería de tests automática e interactiva.

## 🧪 Motor de Evaluación Interactivo

El proyecto incorpora un potente evaluador automático (`test_auto.sh` y `test_eval.sh`) programado en Bash avanzado.
*   Despliega casi una **treintena de tests exhaustivos de límites (Stress Tests)**, cubriendo fugas de memoria (`valgrind`), gestión asíncrona de I/O contra *Short Reads*, corrupciones arquitectónicas ELF (Ataques *Big-Endian*, *String Table OOB*) y evasión activa de heurísticas de compilación avanzadas (Sábanas de NOPs y límites de *Red Zone* de la ABI).
*   **Búsqueda Dinámica**: El evaluador hace búsquedas regex en directo usando *awk* para imprimir exactamente la línea de código C/ASM respectiva que entraña el código vital testeado ante tus ojos.

## 🎯 Tecnologías Empleadas

- **C y Assembly x86_64 (NASM)**.
- Manipulación avanzada de punteros y análisis estático de binarios.
- Inyección en Memoria RAM (`mprotect`, `mmap`, `munmap`, syscalls directas).
- Algoritmo de cifrado Rivest Cipher 4 (KSA + PRGA).

## 🚀 Uso del Empaquetador

El programa cuenta con dos modos de operación principales:

1. **Modo Automático (Pura Entropía)**:
   ```bash
   ./woody_woodpacker <archivo_victima>
   ```
   *Genera una llave aleatoria impredecible de 128-bits usando `/dev/urandom`.*

2. **Modo Llave Paramétrica (Bonus Oficial)**:
   ```bash
   ./woody_woodpacker <archivo_victima> <llave_hexadecimal_32_chars>
   ```
   *Permite al usuario forzar una llave RC4 específica exacta de 16 bytes (32 caracteres hexadecimales).*

3. **Evasión Anti-Debugging (Bonus Oculto / Extra)**:
   *El binario final inyectado incluye un sistema de evasión a nivel de sistema operativo (`ptrace`) que detecta y aborta automáticamente la ejecución si un evaluador o analista de malware intenta hacerle ingeniería inversa con `strace`, `ltrace` o `gdb`.*

> Esto generará tu archivo infectado `woody` completamente listo para infiltrarse en tu sistema.
> Para conocer a fondo cómo se valida, genera y aplica matemáticamente esta criptografía estriadora (RC4), puedes consultar el documento [⚙️ La Maquinaria Principal (src/)](./code_src.md) y el artículo [🔐 Criptografía Dinámica RC4](./code_RC4.md).

---
<br>

<div align="center">

**Autores:    rdel-olm & sternero**

**Hecho con ❤️ en [42 Málaga](https://youtu.be/DGQQ13ywGUM?si=xC_Gq7_WQSkL69O4)**

*Última Actualización: Abril 2026*

</div>
<div align="right">

[⬆️ Volver arriba](#-woody-woodpacker)

</div>
