#include "woody.h"

/* ==========================================================================
 * 🩻 RADIOGRAFÍA 64-BITS: parse_elf64
 * ==========================================================================
 * Analiza un paciente de 64-bits (x86_64) en busca de la sección .text
 * y el Cave (Hueco libre) para el injerto.
 * ========================================================================== */
static int parse_elf64(t_woody *woody)
{
    Elf64_Shdr *shdr = (Elf64_Shdr *)((uint8_t *)woody->addr + woody->ehdr64->e_shoff);
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uint8_t *)woody->addr + woody->ehdr64->e_phoff);
    int         text_found = 0;

    // Validación de integridad
    if (woody->ehdr64->e_shoff == 0 || woody->ehdr64->e_phoff == 0 || 
        woody->ehdr64->e_shoff > woody->size || woody->ehdr64->e_phoff > woody->size)
    {
        fprintf(stderr, "Error: Diseño ELF corrupto detectado.\n");
        return (-1);
    }

    // Buscamos la sección String Table para resolver nombres como ".text"
    if (woody->ehdr64->e_shstrndx >= woody->ehdr64->e_shnum)
    {
        fprintf(stderr, "Error: shstrndx fuera de limites.\n");
        return (-1);
    }
    const char *shstrtab = (const char *)woody->addr + shdr[woody->ehdr64->e_shstrndx].sh_offset;

    // LOCALIZACIÓN DEL CORAZÓN (.text)
    for (int i = 0; i < woody->ehdr64->e_shnum; i++)
    {
        if (strcmp(shstrtab + shdr[i].sh_name, ".text") == 0)
        {
            woody->text_section64 = &shdr[i];
            
            // Traducimos a variables abstractas universales
            woody->text_offset = shdr[i].sh_offset;
            woody->text_size   = shdr[i].sh_size;
            woody->text_vaddr  = shdr[i].sh_addr;

            printf("Encontrada sección genérica: .text. Offset: 0x%lx, Tamaño: 0x%lx\n", 
                   woody->text_offset, woody->text_size);
            text_found = 1;
            break;
        }
    }

    if (!text_found)
    {
        fprintf(stderr, "Error: ¡El paciente no tiene corazón! (No se encontró la sección .text)\n");
        return (-1);
    }

    // LOCALIZACIÓN DEL SEGMENTO LOAD (El flujo sanguíneo donde el .text está inmerso)
    for (int i = 0; i < woody->ehdr64->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD && 
            (phdr[i].p_flags & PF_X) && 
            woody->text_offset >= phdr[i].p_offset && 
            woody->text_offset < (phdr[i].p_offset + phdr[i].p_filesz))
        {
            woody->target_segment64 = &phdr[i];
            woody->segment_vaddr = phdr[i].p_vaddr;

            // BÚSQUEDA DEL "CODE CAVE"
            // El hueco libre se cuenta desde el final real del segmento (archivos en disco)
            // hasta lo que pide en RAM el inicio de la siguiente sección/segmento de alineación.
            size_t segment_end_offset = phdr[i].p_offset + phdr[i].p_filesz;
            size_t available_cave = 0;

            if (i < woody->ehdr64->e_phnum - 1)
            {
                // Espacio entre este segmento y el siguiente segmento
                available_cave = phdr[i + 1].p_offset - segment_end_offset;
            }
            else
            {
                // Si es el último segmento, tenemos hasta el final del archivo
                available_cave = woody->size - segment_end_offset;
            }

            woody->cave_offset = segment_end_offset;
            woody->cave_size   = available_cave;

            printf("Encontrada Code Cave (Genérica) en Offset: 0x%lx con Capacidad: %lu bytes\n", 
                   woody->cave_offset, woody->cave_size);
            return (0);
        }
    }

    fprintf(stderr, "Error: No se encontró el Segmento de Carga (PT_LOAD) Ejecutable.\n");
    return (-1);
}

/* ==========================================================================
 * 🩻 RADIOGRAFÍA 32-BITS: parse_elf32
 * ==========================================================================
 * Analiza un paciente de 32-bits (i386) en busca de la sección .text
 * ========================================================================== */
static int parse_elf32(t_woody *woody)
{
    Elf32_Shdr *shdr = (Elf32_Shdr *)((uint8_t *)woody->addr + woody->ehdr32->e_shoff);
    Elf32_Phdr *phdr = (Elf32_Phdr *)((uint8_t *)woody->addr + woody->ehdr32->e_phoff);
    int         text_found = 0;

    if (woody->ehdr32->e_shoff == 0 || woody->ehdr32->e_phoff == 0 || 
        woody->ehdr32->e_shoff > woody->size || woody->ehdr32->e_phoff > woody->size)
    {
        fprintf(stderr, "Error: Diseño ELF corrupto detectado (32-bits).\n");
        return (-1);
    }

    if (woody->ehdr32->e_shstrndx >= woody->ehdr32->e_shnum)
    {
        fprintf(stderr, "Error: shstrndx fuera de limites (32-bits).\n");
        return (-1);
    }
    const char *shstrtab = (const char *)woody->addr + shdr[woody->ehdr32->e_shstrndx].sh_offset;

    // LOCALIZACIÓN DEL CORAZÓN (.text)
    for (int i = 0; i < woody->ehdr32->e_shnum; i++)
    {
        if (strcmp(shstrtab + shdr[i].sh_name, ".text") == 0)
        {
            woody->text_section32 = &shdr[i];
            
            // Abstracción universal en uint64_t
            woody->text_offset = (uint64_t)shdr[i].sh_offset;
            woody->text_size   = (uint64_t)shdr[i].sh_size;
            woody->text_vaddr  = (uint64_t)shdr[i].sh_addr;

            printf("Encontrada sección genérica: .text. Offset: 0x%lx, Tamaño: 0x%lx\n", 
                   woody->text_offset, woody->text_size);
            text_found = 1;
            break;
        }
    }

    if (!text_found)
    {
        fprintf(stderr, "Error: ¡El paciente no tiene corazón! (No se encontró la sección .text)\n");
        return (-1);
    }

    // LOCALIZACIÓN DEL SEGMENTO LOAD
    for (int i = 0; i < woody->ehdr32->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD && 
            (phdr[i].p_flags & PF_X) && 
            woody->text_offset >= phdr[i].p_offset && 
            woody->text_offset < (phdr[i].p_offset + phdr[i].p_filesz))
        {
            woody->target_segment32 = &phdr[i];
            woody->segment_vaddr = (uint64_t)phdr[i].p_vaddr;

            size_t segment_end_offset = phdr[i].p_offset + phdr[i].p_filesz;
            size_t available_cave = 0;

            if (i < woody->ehdr32->e_phnum - 1)
                available_cave = phdr[i + 1].p_offset - segment_end_offset;
            else
                available_cave = woody->size - segment_end_offset;

            woody->cave_offset = segment_end_offset;
            woody->cave_size   = available_cave;

            printf("Encontrada Code Cave (Genérica) en Offset: 0x%lx con Capacidad: %lu bytes\n", 
                   woody->cave_offset, woody->cave_size);
            return (0);
        }
    }

    fprintf(stderr, "Error: No se encontró el Segmento de Carga (PT_LOAD) Ejecutable.\n");
    return (-1);
}

/* ==========================================================================
 * 🩻 EL EXPLORADOR: parse_elf (ENTRYPOINT)
 * ==========================================================================
 * Delega el análisis morfológico dependiendo de si el paciente es un
 * niño (32-bits) o un adulto (64-bits).
 * ========================================================================== */
int parse_elf(t_woody *woody)
{
    if (woody->is_32bit)
        return parse_elf32(woody);
    else
        return parse_elf64(woody);
}
