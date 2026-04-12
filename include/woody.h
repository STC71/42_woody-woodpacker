#ifndef WOODY_H
# define WOODY_H

# include <elf.h>           // Para estructuras ELF y constantes relacionadas con el formato ELF
# include <fcntl.h>         // Para open() y constantes de flags de archivos (O_RDONLY, O_WRONLY, etc.)
# include <stdio.h>         // Para funciones de entrada/salida estándar (printf, fprintf, etc.)
# include <stdlib.h>        // Para funciones de utilidad general (malloc, free, exit, etc.)
# include <string.h>        // Para funciones de manipulación de cadenas (strlen, memcpy, memcmp, etc.)
# include <stdint.h>        // Para tipos de enteros con tamaños específicos (uint8_t, uint64_t, etc.)
# include <time.h>          // Para funciones relacionadas con el tiempo (time, etc.)
# include <sys/mman.h>      // Para funciones de mapeo de memoria (mmap, munmap, etc.)
# include <sys/stat.h>      // Para funciones relacionadas con el estado de archivos (fstat, struct stat, etc.)
# include <unistd.h>        // Para funciones de bajo nivel (close, read, write, etc.)

# define ERR_USAGE "Uso: ./woody_woodpacker <binario>\n"
# define ERR_OPEN "Error: No se pudo abrir el archivo\n"
# define ERR_FSTAT "Error: No se pudo obtener información del archivo (o es un directorio)\n"
# define ERR_MMAP "Error: Fallo al ejecutar mmap\n"
# define ERR_NOT_ELF "Error: El archivo no es un ELF válido\n"
# define ERR_NOT_64 "Error: El archivo no es un ELF de 64 bits\n"
# define ERR_NOT_X86_64 "Error: El binario no tiene una arquitectura x86_64\n"
# define ERR_NOT_EXEC "Error: El archivo no es un ejecutable ni un objeto compartido (shared object)\n"

typedef struct s_woody
{
    void        *addr;      // Dirección base del archivo mapeado en memoria
    size_t      size;       // Tamaño del archivo mapeado
    mode_t      file_mode;  // Guardaremos los persmisos originales aquí
    Elf64_Ehdr  *ehdr;      // Puntero al ELF Header para acceso rápido
    
    // Tramos encontrados en la Fase 2 (Segmento y Sección Objetivos)
    Elf64_Phdr  *target_segment; // Segmento Ejecutable donde inyectaremos
    Elf64_Shdr  *text_section;   // Sección de Código original a encriptar

    // Información del Cave y Payload
    size_t      cave_offset;    // Offset dentro del archivo donde se encuentra la cueva
    size_t      cave_size;      // Tamaño de la cueva (espacio disponible para inyección)
    
    // Criptografía (RC4)
    uint8_t     key[16];   // Clave de cifrado de 128-bit
    size_t      key_len;   // Siempre será 16 para esta implementación
} t_woody;

int     init_woody(const char *filename, t_woody *woody);
void    cleanup_woody(t_woody *woody);
int     parse_elf(t_woody *woody);

void    generate_random_key(t_woody *woody);
int     parse_custom_key(t_woody *woody, const char *hex_str);
int     encrypt_text_section(t_woody *woody);

int     generate_and_inject_payload(t_woody *woody);

#endif
