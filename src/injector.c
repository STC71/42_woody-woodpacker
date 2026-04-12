#include "woody.h"

/*
** Cargamos el Payload (Stub) ensamblado en RAM, buscamos la firma
** [0x1111111111111111], parcheamos nuestras deltas y claves calculadas,
** volvemos el archivo destino executable y finalmente creamos el clon './woody'.
*/
int generate_and_inject_payload(t_woody *woody)
{
    int         fd;
    struct stat st;
    uint8_t     *payload;
    size_t      payload_size;
    uint64_t    *patch_ptr = NULL;

    printf("Inyectando Payload ASM... Fases 4 y 5\n");

    // 1. Leer el archivo binario del payload pre-ensamblado
    fd = open("asm/payload.bin", O_RDONLY);     // Intentamos abrir el archivo del payload ensamblado
    if (fd < 0)
    {
        fprintf(stderr, "Error: No se pudo abrir asm/payload.bin. ¿Ha ejecutado Make?\n");
        return (-1);
    }
    fstat(fd, &st);
    payload_size = st.st_size;
    payload = malloc(payload_size);
    read(fd, payload, payload_size);
    close(fd);      // Cerramos el descriptor de archivo después de leer el payload

    // Verificamos que quepa dentro de la Cueva que encontramos en la Fase 2 
    if (payload_size > woody->cave_size)
    {
        fprintf(stderr, "Error: Tamaño del payload (%lu bytes) excede la capacidad de la Code Cave (%lu bytes).\n", payload_size, woody->cave_size);
        fprintf(stderr, "Expandir dinámicamente el segmento sería necesario, pero escapa del alcance de esta implementación básica.\n");
        free(payload);
        return (-1);
    }

    // 2. Dar permiso de ESCRITURA (PF_W) al segmento principal que contiene .text
    // Para que nuestro Payload RC4 no crashee con un Segfault al intentar desencriptarlo.
    woody->target_segment->p_flags |= PF_W;

    // 3. Buscar la firma mágica que pusimos en ensamblador: 0x1111111111111111
    for (size_t i = 0; i < payload_size - 8; i++)
    {
        if (*(uint64_t *)(payload + i) == 0x1111111111111111)
        {
            patch_ptr = (uint64_t *)(payload + i);
            break;
        }
    }

    if (!patch_ptr)
    {
        fprintf(stderr, "Error: ¡No se encontró la firma en variables del Payload!\n");
        free(payload);
        return (-1);
    }

    // 4. Calcular el "Delta" relativo en tiempo de ejecución de variables a memoria
    size_t payload_vaddr = woody->target_segment->p_vaddr + (woody->cave_offset - woody->target_segment->p_offset);
    uint64_t vars_vaddr = payload_vaddr + ((uint8_t *)patch_ptr - payload);
    size_t text_vaddr = woody->target_segment->p_vaddr + (woody->text_section->sh_offset - woody->target_segment->p_offset);

    // --- PARCHEAR MEMORIA CRUDA DEL PAYLOAD ---
    // [0] rel_text: Distancia desde 'vars' hasta la sección '.text'
    patch_ptr[0] = (uint64_t)(text_vaddr - vars_vaddr);

    // [1] text_size: Tamaño de encriptado
    patch_ptr[1] = woody->text_section->sh_size;

    // [2] / [3] RC4 Key (Copiamos los 16 bytes de buffer generados en Fase 3)
    memcpy(&patch_ptr[2], woody->key, 16);

    // [4] rel_oep: Distancia hacia el Original Entry Point (OEP) para el salto final
    patch_ptr[4] = (uint64_t)(woody->ehdr->e_entry - vars_vaddr);

    // 5. El Golpe Maestro: Secuestramos el File Entry Point del archivo ELF original
    woody->ehdr->e_entry = payload_vaddr;

    // 6. Copiar el Payload parcheado hacia dentro del Code Cave (archivo mapeado en RAM)
    memcpy(woody->addr + woody->cave_offset, payload, payload_size);

    // 7. Modificamos el Header del PT_LOAD Segment para decirle al OS que cargue en RAM el nuevo tamaño
    woody->target_segment->p_filesz += payload_size;
    woody->target_segment->p_memsz += payload_size;

    // 8. Escribir el nuevo archivo `woody` a disco con las modificaciones y permisos idénticos al original
    fd = open("woody", O_CREAT | O_TRUNC | O_WRONLY, woody->file_mode);
    if (fd < 0)
    {
        fprintf(stderr, "Error: No se pudo crear archivo resultante 'woody'\n");
        free(payload);
        return (-1);
    }
    
    // Volcamos todo el buffer corrupto
    write(fd, woody->addr, woody->size);    // Escribimos el buffer completo (con modificaciones) al nuevo archivo `woody`
    close(fd);                              // Cerramos el descriptor de archivo después de escribirlo
    free(payload);
    
    // Forzamos un chmod en caso de que la máscara Umask del sistema anule el file_mode de open
    chmod("woody", woody->file_mode);

    printf("¡Creación del archivo binario 'woody' completada!\n");
    return (0);
}
