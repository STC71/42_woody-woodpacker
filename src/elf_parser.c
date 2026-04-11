#include "woody.h"

/*
** Asegurarse de que un puntero apunta a una zona "legal" de memoria
** dentro de los límites del archivo mapeado para prevernir Core Dumps
*/
static int is_safe_ptr(t_woody *woody, void *ptr, size_t size_needed)
{
    if ((void *)ptr + size_needed > woody->addr + woody->size)
    {
        fprintf(stderr, "Error: Corrupted ELF layout detected (Out of bounds access)\n");
        return (0);
    }
    return (1);
}

/*
** Localizar la sección .text dentro de la Section Header Table.
** .text contiene el código del programa que encriptaremos más adelante.
*/
static int find_text_section(t_woody *woody)
{
    Elf64_Shdr  *shdr;
    Elf64_Shdr  *strtab_sh;
    char        *strtab;

    // Localizar el Section Header Table (SHT)
    shdr = (Elf64_Shdr *)(woody->addr + woody->ehdr->e_shoff);
    if (!is_safe_ptr(woody, shdr, woody->ehdr->e_shnum * sizeof(Elf64_Shdr)))
        return (-1);

    // Obtener la tabla de strings para buscar la etiqueta ".text"
    if (woody->ehdr->e_shstrndx >= woody->ehdr->e_shnum)
        return (-1);
    
    strtab_sh = &shdr[woody->ehdr->e_shstrndx];
    if (!is_safe_ptr(woody, woody->addr + strtab_sh->sh_offset, strtab_sh->sh_size))
        return (-1);
    
    strtab = (char *)(woody->addr + strtab_sh->sh_offset);

    // Recorrer las secciones
    for (int i = 0; i < woody->ehdr->e_shnum; i++)
    {
        if (strcmp(strtab + shdr[i].sh_name, ".text") == 0)
        {
            woody->text_section = &shdr[i];
            printf("Found section: .text. Offset: 0x%lx, Size: 0x%lx\n", 
                   woody->text_section->sh_offset, woody->text_section->sh_size);
            return (0);
        }
    }
    
    fprintf(stderr, "Error: '.text' section not found\n");
    return (-1);
}

/*
** Encontrar el "Code Cave". Normalmente al final del segmento ejecutable
** en el que se ubicará nuestra inyección (Payload). Este segmento es un 
** Program Header de tipo PT_LOAD con privilegios PF_X (eXecutable).
*/
static int find_code_cave(t_woody *woody)
{
    Elf64_Phdr  *phdr;
    Elf64_Phdr  *next_phdr;
    size_t      lowest_next_offset;

    phdr = (Elf64_Phdr *)(woody->addr + woody->ehdr->e_phoff);
    if (!is_safe_ptr(woody, phdr, woody->ehdr->e_phnum * sizeof(Elf64_Phdr)))
        return (-1);

    lowest_next_offset = woody->size; // Empezamos asumiendo que el límite es el fin del archivo.
    woody->target_segment = NULL;

    // Buscar el Segmento Ejecutable
    for (int i = 0; i < woody->ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD && (phdr[i].p_flags & PF_X))
        {
            woody->target_segment = &phdr[i];
            break;
        }
    }

    if (!woody->target_segment)
    {
        fprintf(stderr, "Error: No executable PT_LOAD segment found\n");
        return (-1);
    }

    lowest_next_offset = woody->size;
    // Encontrar el tamaño de la cueva: miramos el offset del SIGUIENTE segmento para ver
    // qué "padding" hueco (cueva) queda al final del nuestro.
    for (int i = 0; i < woody->ehdr->e_phnum; i++)
    {
        next_phdr = &phdr[i];
        if (next_phdr->p_offset > woody->target_segment->p_offset && 
            next_phdr->p_offset < lowest_next_offset)
        {
            lowest_next_offset = next_phdr->p_offset;
        }
    }

    woody->cave_offset = woody->target_segment->p_offset + woody->target_segment->p_filesz;
    
    // Validar el cálculo de la cueva
    if (lowest_next_offset >= woody->cave_offset)
        woody->cave_size = lowest_next_offset - woody->cave_offset;
    else
        woody->cave_size = 0;

    printf("Found Code Cave at Offset: 0x%lx with Capacity: %lu bytes\n", 
           woody->cave_offset, woody->cave_size);
    
    return (0);
}

int parse_elf(t_woody *woody)
{
    // Localizar primero el código que encriptaremos después (.text)
    if (find_text_section(woody) < 0)
        return (-1);

    // Buscar dónde inyectar nuestro payload (Code Cave)
    if (find_code_cave(woody) < 0)
        return (-1);

    return (0);
}
