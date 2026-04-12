#include "woody.h"

void cleanup_woody(t_woody *woody)
{
    if (woody->addr && woody->addr != MAP_FAILED)
    {
        munmap(woody->addr, woody->size);   // Liberamos la memoria mapeada
        woody->addr = NULL;
    }
}

static int check_elf_format(t_woody *woody)
{
    // Tamaño mínimo para la cabecera ELF
    if (woody->size < sizeof(Elf64_Ehdr))
    {
        fprintf(stderr, ERR_NOT_ELF);   // No es un ELF válido si el tamaño es menor que el de un header
        return (-1);
    }

    woody->ehdr = (Elf64_Ehdr *)woody->addr;

    // Comprobar Número Mágico \x7f E L F
    if (memcmp(woody->ehdr->e_ident, ELFMAG, SELFMAG) != 0)
    {
        fprintf(stderr, ERR_NOT_ELF);   // No es un ELF válido si no tiene la firma mágica correcta
        return (-1);
    }

    // Comprobar si es 64-bit
    if (woody->ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    {
        fprintf(stderr, ERR_NOT_64);    // No es un ELF de 64-bit si el campo EI_CLASS no es ELFCLASS64
        return (-1);
    }

    // Comprobar si es arquitectura x86_64
    if (woody->ehdr->e_machine != EM_X86_64)
    {
        fprintf(stderr, ERR_NOT_X86_64);    // No es un ELF de arquitectura x86_64
        return (-1);
    }

    // Comprobar que es un software ejecutable puro o de carga dinámica (PIE)
    if (woody->ehdr->e_type != ET_EXEC && woody->ehdr->e_type != ET_DYN)
    {
        fprintf(stderr, ERR_NOT_EXEC);
        return (-1);
    }

    return (0);
}

int init_woody(const char *filename, t_woody *woody)
{
    int         fd;
    struct stat st;

    fd = open(filename, O_RDONLY);  // Intentamos abrir el archivo destino en modo lectura para mapearlo posteriormente
    if (fd < 0)
    {
        fprintf(stderr, ERR_OPEN);  // No se pudo abrir el archivo (puede que no exista o no tengamos permisos)
        return (-1);
    }

    if (fstat(fd, &st) < 0 || S_ISDIR(st.st_mode))
    {
        fprintf(stderr, ERR_FSTAT);
        close(fd);      // Cerramos el descriptor de archivo antes de salir
        return (-1);
    }

    woody->size = st.st_size;       // Guardamos el tamaño del archivo para límites de seguridad en accesos posteriores
    woody->file_mode = st.st_mode;  // Guardamos los permisos originales para luego aplicarlos al archivo destino
    // Mapear fichero como privado para sobreescribir memoria cómodamente sin alterar el disco original
    woody->addr = mmap(NULL, woody->size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);      // Cerramos el descriptor de archivo después de mapearlo, ya no lo necesitaremos

    if (woody->addr == MAP_FAILED)
    {
        fprintf(stderr, ERR_MMAP);
        return (-1);
    }

    if (check_elf_format(woody) < 0)
    {
        cleanup_woody(woody);
        return (-1);
    }

    return (0);
}

int main(int argc, char **argv)
{
    t_woody woody;

    memset(&woody, 0, sizeof(t_woody));

    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Uso: ./woody_woodpacker <binario> [clave-hex-de-16-bytes]\n");
        return (1);
    }

    if (init_woody(argv[1], &woody) < 0)
        return (1);

    // Módulos Bonus: Clave paramétrica o Clave Aleatoria 128-bits
    if (argc == 3)
    {
        if (parse_custom_key(&woody, argv[2]) < 0)
        {
            cleanup_woody(&woody);
            return (1);
        }
    }
    else
        generate_random_key(&woody);

    // Impresión de la Key (Formato típico Packer)
    printf("CLAVE [128-bit RC4]: 0x");
    for (size_t i = 0; i < woody.key_len; i++)
        printf("%02X", woody.key[i]);
    printf("\n");

    printf("¡ÉXITO! Archivo '%s' mapeado en la dirección %p\n", argv[1], woody.addr);
    printf("Original Entry Point (OEP) original reside en 0x%lx\n", woody.ehdr->e_entry);

    if (parse_elf(&woody) < 0)
    {
        cleanup_woody(&woody);
        return (1);
    }

    // Ejecuta el descifrado simétrico (RC4 Fase 3)
    encrypt_text_section(&woody);

    // Intectamos y parchaemos el Código Ensamblador (Fases 4 y 5)
    if (generate_and_inject_payload(&woody) < 0)
    {
        cleanup_woody(&woody);
        return (1);
    }

    cleanup_woody(&woody);
    return (0);
}
