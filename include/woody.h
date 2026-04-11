#ifndef WOODY_H
# define WOODY_H

# include <elf.h>
# include <fcntl.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <stdint.h>
# include <time.h>
# include <sys/mman.h>
# include <sys/stat.h>
# include <unistd.h>

# define ERR_USAGE "Usage: ./woody_woodpacker <binary>\n"
# define ERR_OPEN "Error: Could not open file\n"
# define ERR_FSTAT "Error: Could not retrieve file information (or is a directory)\n"
# define ERR_MMAP "Error: mmap failed\n"
# define ERR_NOT_ELF "Error: File is not a valid ELF\n"
# define ERR_NOT_64 "Error: File is not a 64-bit ELF\n"
# define ERR_NOT_X86_64 "Error: File is not an x86_64 architecture binary\n"
# define ERR_NOT_EXEC "Error: File is neither an executable nor a shared object\n"

typedef struct s_woody
{
    void        *addr;
    size_t      size;
    mode_t      file_mode; // Guardaremos los persmisos originales aquí
    Elf64_Ehdr  *ehdr;
    
    // Tramos encontrados en la Fase 2 (Segmento y Sección Objetivos)
    Elf64_Phdr  *target_segment; // Segmento Ejecutable donde inyectaremos
    Elf64_Shdr  *text_section;   // Sección de Código original a encriptar

    // Información del Cave y Payload
    size_t      cave_offset;
    size_t      cave_size;
    
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
