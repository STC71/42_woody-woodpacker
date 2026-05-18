#include "woody.h"

/* 
 * 🛂 check_elf_format
 * El portero de discoteca. Revisa el carné de identidad (la cabecera) del 
 * archivo para asegurarse de que es un software válido de Linux, ya sea 64-bits o 32-bits.
 */
static int check_elf_format(t_woody *woody)
{
    // 1. ¿Es tan pequeño que ni siquiera tiene cabecera básica?
    if (woody->size < sizeof(Elf32_Ehdr))
    {
        fprintf(stderr, ERR_NOT_ELF);
        return (-1);
    }

    // Leemos la cabecera genérica (asumimos primero 32 o 64 bit, los primeros bytes son idénticos)
    unsigned char *e_ident = (unsigned char *)woody->addr;

    // 2. Comprobar el Número Mágico. Todos los programas Linux empiezan por "\x7f E L F"
    if (memcmp(e_ident, ELFMAG, SELFMAG) != 0)
    {
        fprintf(stderr, ERR_NOT_ELF);
        return (-1);
    }

    // 3. Comprobar la Arquitectura (32-bits o 64-bits) -> La magia real de la Fase 3
    if (e_ident[EI_CLASS] == ELFCLASS32)
        woody->is_32bit = 1;
    else if (e_ident[EI_CLASS] == ELFCLASS64)
        woody->is_32bit = 0;
    else
    {
        fprintf(stderr, ERR_NOT_SUPPORTED);
        return (-1);
    }

    // 4. Mapeo específico según Arquitectura
    if (woody->is_32bit)
    {
        if (woody->size < sizeof(Elf32_Ehdr))
            return (-1);
        woody->ehdr32 = (Elf32_Ehdr *)woody->addr;
        woody->orig_entry = woody->ehdr32->e_entry;
        
        if (woody->ehdr32->e_machine != EM_386)
        {
            fprintf(stderr, ERR_NOT_SUPPORTED);
            return (-1);
        }
        if (woody->ehdr32->e_type != ET_EXEC && woody->ehdr32->e_type != ET_DYN)
        {
            fprintf(stderr, ERR_NOT_EXEC);
            return (-1);
        }
    }
    else
    {
        if (woody->size < sizeof(Elf64_Ehdr))
            return (-1);
        woody->ehdr64 = (Elf64_Ehdr *)woody->addr;
        woody->orig_entry = woody->ehdr64->e_entry;

        if (woody->ehdr64->e_machine != EM_X86_64)
        {
            fprintf(stderr, ERR_NOT_SUPPORTED);
            return (-1);
        }
        if (woody->ehdr64->e_type != ET_EXEC && woody->ehdr64->e_type != ET_DYN)
        {
            fprintf(stderr, ERR_NOT_EXEC);
            return (-1);
        }
    }

    // 5. Comprobar Little Endian
    if (e_ident[EI_DATA] != ELFDATA2LSB)
    {
        fprintf(stderr, ERR_NOT_LITTLE_ENDIAN);
        return (-1);
    }

    return (0); // ¡El paciente es válido para cirugía!
}

/* 
 * 🧹 cleanup_woody
 * Da el alta al paciente expulsándolo de nuestro Quirófano.
 */
void cleanup_woody(t_woody *woody)
{
    if (woody->addr && woody->addr != MAP_FAILED)
        munmap(woody->addr, woody->size);
}

/* 
 * 🏥 init_woody
 * Configura la mesa de operaciones (Mapea el archivo a la Memoria RAM).
 */
int init_woody(const char *filename, t_woody *woody)
{
    int         fd;
    struct stat st;

    // Intentamos abrir el archivo de sólo lectura.
    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, ERR_OPEN);
        return (-1);
    }

    if (fstat(fd, &st) < 0 || S_ISDIR(st.st_mode))
    {
        fprintf(stderr, ERR_FSTAT);
        close(fd);
        return (-1);
    }

    woody->size = st.st_size;
    woody->file_mode = st.st_mode;

    // MAP_PRIVATE para no sobreescribir el disco al alterar la copia de la RAM.
    woody->addr = mmap(NULL, woody->size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);

    if (woody->addr == MAP_FAILED)
    {
        fprintf(stderr, ERR_MMAP);
        return (-1);
    }

    // Comprobamos que el archivo en la RAM cumple todos los requisitos
    if (check_elf_format(woody) < 0)
    {
        cleanup_woody(woody);
        return (-1);
    }

    return (0);
}

/* 
 * 🏁 MAIN
 * El punto de entrada inicial de todo Woody Woodpacker.
 */
int main(int argc, char **argv)
{
    t_woody woody;
    int arg_idx = 1;

    memset(&woody, 0, sizeof(t_woody));
    woody.crypto_algo = 0; // Default RC4

    if (argc >= 2)
    {
        if (strcmp(argv[arg_idx], "--xor") == 0)
        {
            woody.crypto_algo = 1;
            arg_idx++;
        }
        else if (strcmp(argv[arg_idx], "--rc4") == 0)
        {
            woody.crypto_algo = 0;
            arg_idx++;
        }
    }

    if (argc < arg_idx + 1 || argc > arg_idx + 2)
    {
        fprintf(stderr, "Uso: ./woody_woodpacker [--xor|--rc4] <binario> [clave-hex-de-16-bytes]\n");
        return (1);
    }

    // 1. Tumbamos al paciente en la camilla de la RAM
    if (init_woody(argv[arg_idx], &woody) < 0)
        return (1);

    // 2. Le pasamos las LLAVES (aleatorias o manual)
    if (argc == arg_idx + 2)
    {
        if (parse_custom_key(&woody, argv[arg_idx + 1]) < 0)
        {
            cleanup_woody(&woody);
            return (1);
        }
    }
    else
        generate_random_key(&woody);

    if (woody.crypto_algo == 1)
        printf("CLAVE [128-bit XOR]: 0x");
    else
        printf("CLAVE [128-bit RC4]: 0x");
    for (size_t i = 0; i < woody.key_len; i++)
        printf("%02X", woody.key[i]);
    printf("\n");

    printf("¡ÉXITO! Archivo '%s' mapeado en la dirección %p\n", argv[arg_idx], woody.addr);
    printf("Original Entry Point (OEP) original reside en 0x%lx\n", woody.orig_entry);
    printf("Perfil Genético Detectado: ELF %s\n", woody.is_32bit ? "32-bits (x86/i386)" : "64-bits (x86_64)");

    // 3. ANÁLISIS: Explorador busca .text y Code Caves
    if (parse_elf(&woody) < 0)
    {
        cleanup_woody(&woody);
        return (1);
    }

    // 4. ANESTESIA: Encriptamos el código en RAM
    if (encrypt_text_section(&woody) < 0)
    {
        cleanup_woody(&woody);
        return (1);
    }

    // 5. CIRUGÍA: Inyectar Virus y Volcar a Disco
    if (generate_and_inject_payload(&woody) < 0)
    {
        cleanup_woody(&woody);
        return (1);
    }

    printf("¡Cirugía finalizada! El paciente clonado y empaquetado './woody' está preparado.\n");

    // Limpieza final
    cleanup_woody(&woody);
    return (0);
}
