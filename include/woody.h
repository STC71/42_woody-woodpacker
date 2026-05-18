#ifndef WOODY_H
# define WOODY_H

# include <elf.h>           // Para estructuras ELF y constantes relacionadas con las "Radiografías"
# include <fcntl.h>         // Para abrir y escribir pacientes en disco (O_RDONLY, O_WRONLY)
# include <stdio.h>         // Para mostrar el parte médico por pantalla
# include <stdlib.h>        // Para pedir memoria para la mesa de operaciones temporal (malloc)
# include <string.h>        // Para cortar y coser bytes puros (memcpy, memcmp)
# include <stdint.h>        // Para asegurar tamaños quirúrgicos idénticos (uint64_t)
# include <time.h>          // Usado si falla la entropía del Cerrajero (rand)
# include <sys/mman.h>      // El Quirófano Virtual (mmap, munmap)
# include <sys/stat.h>      // Para conocer el peso real del paciente antes de abrirlo (fstat)
# include <unistd.h>        // Para cerrar expedientes y accesos de Kernel (close, read)

/* ==========================================================================
 * 🛡️ EL MANUAL MÉDICO: ERRORES COMUNES
 * ========================================================================== */
# define ERR_USAGE "Uso: ./woody_woodpacker [--xor|--rc4] <binario> [clave-hex-16-bytes]\n"
# define ERR_OPEN "Error: No se pudo abrir el archivo\n"
# define ERR_FSTAT "Error: No se pudo obtener información del archivo (o es un directorio)\n"
# define ERR_MMAP "Error: Fallo al ejecutar mmap\n"
# define ERR_NOT_ELF "Error: El archivo no es un ELF válido\n"
# define ERR_NOT_SUPPORTED "Error: La arquitectura no es ni x86_64 (64-bits) ni i386 (32-bits)\n"
# define ERR_NOT_EXEC "Error: El archivo no es un ejecutable ni un objeto compartido (shared object)\n"
# define ERR_NOT_LITTLE_ENDIAN "Error: Archivo en formato Big-Endian. Peligro de corrupción de Offsets. Abortado.\n"

/* ==========================================================================
 * 📋 EL EXPEDIENTE CLÍNICO: struct s_woody
 * ==========================================================================
 * Un virus necesita saber orientarse entre las entrañas del ordenador.
 * Esta estructura es como la pizarra al pie de cama de un paciente. Todo
 * analista ("elf_parser"), anestesista ("crypto") o cirujano ("injector")
 * la consultará para saber en qué punto actuar y qué coordenadas usar.
 * Ahora adaptado también para pacientes infantiles (32-bits).
 * ========================================================================== */
typedef struct s_woody
{
    // -- El Quirófano --
    void        *addr;      // (Mesa de operaciones) Dirección de memoria donde mapeamos al paciente
    size_t      size;       // (Talla) El tamaño total del original
    mode_t      file_mode;  // (Ropa del paciente) Permisos originales (ej. chmod 755)

    // -- Perfil Genético (Fase 3: Multi-Arquitectura) --
    int         is_32bit;   // Bandera que indica si el paciente es x86_32 (1) o x86_64 (0)

    // -- Las Radiografías (Cabeceras 64-bit) --
    Elf64_Ehdr  *ehdr64;
    Elf64_Phdr  *target_segment64;
    Elf64_Shdr  *text_section64;

    // -- Las Radiografías (Cabeceras 32-bit) --
    Elf32_Ehdr  *ehdr32;
    Elf32_Phdr  *target_segment32;
    Elf32_Shdr  *text_section32;

    // -- Coordenadas Universales Abstractas --
    uint64_t    orig_entry;     // Punto de entrada original (e_entry)
    uint64_t    text_offset;    // Dónde empieza la carne .text en disco
    uint64_t    text_size;      // Cuantos bytes pesa el .text
    uint64_t    text_vaddr;     // Dirección de ejecución virtual del .text
    uint64_t    segment_vaddr;  // Dirección de ejecución virtual del segmento LOAD base
    
    // -- Planimetría de Poda / Cueva --
    size_t      cave_offset;    // (Coordenada X en disco) Distancia hasta la bolsa de aire (el Code Cave)
    size_t      cave_size;      // (Capacidad) ¿Cuántos mililitros de virus/prótesis caben aquí?
    
    // -- Departamento de Criptografía (El Cerrajero) --
    int         crypto_algo;    // (Multi-Algoritmo) 0 = RC4 (Default), 1 = XOR
    uint8_t     key[16];        // (El Veneno/Antídoto RC4/XOR) Clave simétrica de cifrado 128-bit
    size_t      key_len;        // Siempre será 16 bytes.
} t_woody;

/* --- ORDENANZA PRINCIPAL (main.c) --- */
int     init_woody(const char *filename, t_woody *woody);
void    cleanup_woody(t_woody *woody);
int     parse_elf(t_woody *woody);

/* --- EL CERRAJERO (crypto.c) --- */
void    generate_random_key(t_woody *woody);
int     parse_custom_key(t_woody *woody, const char *hex_str);
int     encrypt_text_section(t_woody *woody);

/* --- EL CIRUJANO (injector.c) --- */
int     generate_and_inject_payload(t_woody *woody);

#endif
