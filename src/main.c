#include "woody.h"

void cleanup_woody(t_woody *woody)
{
    if (woody->addr && woody->addr != MAP_FAILED)
    {
        munmap(woody->addr, woody->size);
        woody->addr = NULL;
    }
}

static int check_elf_format(t_woody *woody)
{
    // Minimal size for an ELF header
    if (woody->size < sizeof(Elf64_Ehdr))
    {
        fprintf(stderr, ERR_NOT_ELF);
        return (-1);
    }

    woody->ehdr = (Elf64_Ehdr *)woody->addr;

    // Check Magic Number \x7f E L F
    if (memcmp(woody->ehdr->e_ident, ELFMAG, SELFMAG) != 0)
    {
        fprintf(stderr, ERR_NOT_ELF);
        return (-1);
    }

    // Check if 64-bit
    if (woody->ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    {
        fprintf(stderr, ERR_NOT_64);
        return (-1);
    }

    // Check if x86_64
    if (woody->ehdr->e_machine != EM_X86_64)
    {
        fprintf(stderr, ERR_NOT_X86_64);
        return (-1);
    }

    // Check if it's an executable or relatively linked (PIE) shared object
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
    // Map the file privately so we can write over our memory buffer directly without affecting the original file
    woody->addr = mmap(NULL, woody->size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);

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
        fprintf(stderr, "Usage: ./woody_woodpacker <binary> [16-byte-hex-key]\n");
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
    printf("KEY [128-bit RC4]: 0x");
    for (size_t i = 0; i < woody.key_len; i++)
        printf("%02X", woody.key[i]);
    printf("\n");

    printf("SUCCESS! File '%s' mapped at %p\n", argv[1], woody.addr);
    printf("Original Entry Point (OEP) is 0x%lx\n", woody.ehdr->e_entry);

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
