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
# define ERR_USAGE "Uso: ./woody_woodpacker <binario>\n"
# define ERR_OPEN "Error: No se pudo abrir el archivo\n"
# define ERR_FSTAT "Error: No se pudo obtener información del archivo (o es un directorio)\n"
# define ERR_MMAP "Error: Fallo al ejecutar mmap\n"
# define ERR_NOT_ELF "Error: El archivo no es un ELF válido\n"
# define ERR_NOT_64 "Error: El archivo no es un ELF de 64 bits\n"
# define ERR_NOT_X86_64 "Error: El binario no tiene una arquitectura x86_64\n"
# define ERR_NOT_EXEC "Error: El archivo no es un ejecutable ni un objeto compartido (shared object)\n"

/* ==========================================================================
 * 📋 EL EXPEDIENTE CLÍNICO: struct s_woody
 * ==========================================================================
 * Un virus necesita saber orientarse entre las entrañas del ordenador.
 * Esta estructura es como la pizarra al pie de cama de un paciente. Todo
 * analista ("elf_parser"), anestesista ("crypto") o cirujano ("injector")
 * la consulta para saber en qué punto actuar y qué coordenadas usar.
 * ========================================================================== */
typedef struct s_woody
{
    // -- El Quirófano --
    void        *addr;      // (Mesa de operaciones) Dirección de memoria donde mapeamos al paciente
    size_t      size;       // (Talla) El tamaño total del original
    mode_t      file_mode;  // (Ropa del paciente) Permisos originales (ej. chmod 755)

    // -- Las Radiografías (Cabeceras) --
    Elf64_Ehdr  *ehdr;           // Puntero maestro: Las constantes vitales del paciente (Header general)
    Elf64_Phdr  *target_segment; // Dónde hay flujo sanguíneo: Segmento de memoria Ejecutable (Loadable)
    Elf64_Shdr  *text_section;   // El corazón a encriptar: Sección de Código fuente (.text)

    // -- Planimetría de Poda / Cueva --
    size_t      cave_offset;    // (Coordenada X) Distancia hasta la bolsa de aire (el Code Cave)
    size_t      cave_size;      // (Capacidad) ¿Cuántos mililitros de virus/prótesis caben aquí?
    
    // -- Departamento de Criptografía (El Cerrajero) --
    uint8_t     key[16];   // (El Veneno/Antídoto RC4) Clave simétrica de cifrado 128-bit
    size_t      key_len;   // Siempre será 16 bytes.
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
