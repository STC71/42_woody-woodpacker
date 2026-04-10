# 🌲 Woody Woodpacker

**Woody Woodpacker** es un proyecto enfocado en la ciberseguridad y la ingeniería inversa. El objetivo principal es codificar un empaquetador de archivos («packer») al estilo de UPX, especialmente orientado a ejecutables de formato **ELF de 64 bits** en Linux. 

Un *packer* toma un ejecutable original, comprime y/o cifra sus instrucciones y añade una rutina de pre-ejecución (el extractor/desencriptador). Cuando se ejecuta el nuevo programa "empaquetado", este fragmento extrañado de código se encarga de descifrar el bloque principal en memoria antes de saltar al código real del programa original.

## 📁 Estado Actual y Contenido del Repositorio

Actualmente el repositorio contiene los archivos iniciales del proyecto:

- `en.subject.pdf`: Documento con todas las especificaciones y requisitos del proyecto de 42.
- `42 EvalHub.pdf`: Escala de evaluación oficial.
- `resources/`: Binarios y muestras de prueba iniciales.
  - `sample.c` / `sample`: Archivo víctima para empaquetar de prueba.
  - `woody`: Código binario ya empaquetado para estudiar su comportamiento.

## 🎯 Objetivos de Aprendizaje

- Entender el formato ELF detalladamente (Sections, Segments, Headers).
- Manipulación avanzada de punteros y análisis estático de binarios.
- Inyección de código (generación de un stub / payload ejecutable).
- Implementación de algoritmos de cifrado simétricos (e.g. cifrado RC4 o custom).
- Arquitectura x86_64, Assembly y la API del sistema operativo a bajo nivel (uso de syscalls como `mprotect`).

## ⚙️ Descripción del Proyecto a Desplegar

Cuando terminemos, nuestro programa funcionará de la siguiente manera:

```bash
# Uso esperado
./woody_woodpacker <binary>
```

Esto generará un nuevo archivo ejecutable llamado `woody`.
Al ejecutar este `woody`:
1. Debe imprimir la cadena `....WOODY....` seguida de un salto de línea en la salida estándar (`STDOUT`).
2. Desencriptar el código empaquetado del binario en tiempo de ejecución.
3. Ejecutar el programa original con un comportamiento totalmente transparente para el usuario final.

## 🛠️ Tecnologías y Lenguajes (Por definir)

- **Lenguaje Principal:** C / Assembly
- **Sistema Operativo Objetivo:** Linux (archivos ELF)
- **Compilador / Herramientas:** `gcc`, `NASM` (si se usa Assembly para el stub), `make`.

---

> *Desarrollado para el currículo de sistemas y ciberseguridad de la escuela 42.*