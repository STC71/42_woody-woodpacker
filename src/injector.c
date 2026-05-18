#include <errno.h>
#include "woody.h"

/* ==========================================================================
 * 💉 EL CIRUJANO TRAUMATOLÓGICO: injector.c
 * ==========================================================================
 * El "Explorador" nos dijo dónde estaba la herida (.text) y dónde había
 * hueco libre en la cavidad torácica (Code Cave). El "Cerrajero" encriptó
 * (anestesió/bloqueó) la herida original.
 *
 * Ahora llega el Cirujano. Su trabajo es brutal pero preciso:
 * 1. Importar la prótesis de titanio prefabricada (asm/payload.bin o 32.bin).
 * 2. Ajustar los tornillos de la prótesis (Parchear variables en ASM).
 * 3. Insertar la prótesis en el Code Cave.
 * 4. Cambiar el Entry Point del archivo.
 * 5. Cerrar al paciente y crear el clon ("./woody").
 * ========================================================================== */

int generate_and_inject_payload(t_woody *woody)
{
    int         fd;
    struct stat st;
    uint8_t     *payload;
    size_t      payload_size;
    void        *patch_ptr = NULL;

    printf("Inyectando Payload ASM... [Fase del Cirujano]\n");

    // 1. IMPORTAR LA PRÓTESIS (Agnóstico de arquitectura y algoritmo)
    // Asumimos que los archivos 32-bit terminan en _32.bin
    const char *payload_file = "asm/payload.bin";
    
    if (woody->crypto_algo == 1) // Phase 2: XOR Payload
    {
        if (woody->is_32bit)
            payload_file = "asm/payload_xor32.bin";
        else
            payload_file = "asm/payload_xor.bin";
    }
    else // Phase 1: RC4 Payload
    {
        if (woody->is_32bit)
            payload_file = "asm/payload32.bin";
        else
            payload_file = "asm/payload.bin";
    }

    fd = open(payload_file, O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "Error: Prótesis %s no encontrada. ¿Llamaste al orfebre (make para 32-bits aún no implementado)?\n", payload_file);
        return (-1);
    }
    fstat(fd, &st);
    payload_size = st.st_size;
    payload = malloc(payload_size);
    if (!payload)
    {
        fprintf(stderr, "Error: Insuficiente memoria (OOM) para el payload.\n");
        close(fd);
        return (-1);
    }
    
    ssize_t total_read = 0, bytes_read;
    while (total_read < (ssize_t)payload_size)
    {
        bytes_read = read(fd, payload + total_read, payload_size - total_read);
        if (bytes_read < 0)
        {
            if (errno == EINTR) continue;
            fprintf(stderr, "Error: Fallo crítico de I/O al leer el payload.\n");
            free(payload); close(fd); return (-1);
        }
        if (bytes_read == 0) break;
        total_read += bytes_read;
    }
    if (total_read != (ssize_t)payload_size)
    {
        fprintf(stderr, "Error: Lectura incompleta del payload (Short read).\n");
        free(payload); close(fd); return (-1);
    }
    close(fd);

    // 2. COMPROBAR RECHAZO
    if (payload_size > woody->cave_size)
    {
        fprintf(stderr, "Error: La prótesis (%lu bytes) es mayor que la cavidad (%lu bytes).\n", payload_size, woody->cave_size);
        free(payload);
        return (-1);
    }

    // 4. LOCALIZAR LOS TORNILLOS (Buscar la firma genérica 0x1122334455667788)
    // Para simplificar ASM, incluso en 32-bits buscaremos 8 bytes para mantener la firma sólida, o dos de 4.
    for (size_t i = 0; i < payload_size - 8; i++)
    {
        if (*(uint64_t *)(payload + i) == 0x1122334455667788)
        {
            patch_ptr = (payload + i);
            break;
        }
    }

    if (!patch_ptr)
    {
        fprintf(stderr, "Error: ¡Firma de tornillos de fijación no encontrada en el Payload!\n");
        free(payload);
        return (-1);
    }

    // 5. CÁLCULO DE RELATIVIDAD Y PARCHEO (Deltas Agnósticos)
    uint64_t segment_p_offset;
    if (woody->is_32bit)
        segment_p_offset = woody->target_segment32->p_offset;
    else
        segment_p_offset = woody->target_segment64->p_offset;

    size_t payload_vaddr = woody->segment_vaddr + (woody->cave_offset - segment_p_offset);
    uint64_t vars_vaddr = payload_vaddr + ((uint8_t *)patch_ptr - payload);
    size_t text_vaddr = woody->segment_vaddr + (woody->text_offset - segment_p_offset);

    // --- 🔩 AJUSTANDO LOS TORNILLOS DEL PAYLOAD ---
    if (woody->is_32bit)
    {
        // En un malware x86, reservaremos 4_bytes (dd) en lugar de 8_bytes (dq).
        // Sin embargo, si hemos forzado 0x1122334455667788 son 8 bytes.
        // Asumimos para la Fase 3 que la estructura local parcheada será compatible de 32-bits (4 arrays de 4 bytes)
        // Por compatibilidad temporal (mientras no se escriba payload32.s), parcheamos la memoria as-is si 32-bits:
        uint32_t *patch32 = (uint32_t *)patch_ptr;
        patch32[0] = (uint32_t)(text_vaddr - vars_vaddr); // var_text
        patch32[1] = (uint32_t)woody->text_size;          // text_size
        memcpy(&patch32[2], woody->key, 16);              // RC4/XOR Key (Ocupa índices 2, 3, 4, 5 en uint32_t)
        patch32[6] = (uint32_t)(woody->orig_entry - vars_vaddr); // var_oep
    }
    else
    {
        uint64_t *patch64 = (uint64_t *)patch_ptr;
        patch64[0] = (uint64_t)(text_vaddr - vars_vaddr);
        patch64[1] = woody->text_size;
        memcpy(&patch64[2], woody->key, 16);
        patch64[4] = (uint64_t)(woody->orig_entry - vars_vaddr);
    }

    // 6. EL CORTE NERVIOSO SECRETO (Secuestro del Punto de Entrada)
    if (woody->is_32bit)
        woody->ehdr32->e_entry = (uint32_t)payload_vaddr;
    else
        woody->ehdr64->e_entry = payload_vaddr;

    // 7. SUTURAR: Meter la prótesis
    memcpy(woody->addr + woody->cave_offset, payload, payload_size);

    // 8. ENGAÑAR A LOS RAYOS X (Actualizar Segmentos)
    if (woody->is_32bit)
    {
        woody->target_segment32->p_filesz += payload_size;
        woody->target_segment32->p_memsz += payload_size;
    }
    else
    {
        woody->target_segment64->p_filesz += payload_size;
        woody->target_segment64->p_memsz += payload_size;
    }

    // 9. DAR EL ALTA (Crear el clon ejecutable)
    fd = open("woody", O_CREAT | O_TRUNC | O_WRONLY, woody->file_mode);
    if (fd < 0)
    {
        perror("Error al abrir woody");
        fprintf(stderr, "Error: Cirugía fallida. No se pudo crear al paciente resultante 'woody'\n");
        free(payload);
        return (-1);
    }
    
    write(fd, woody->addr, woody->size);
    close(fd);
    free(payload);
    
    // Forzamos los permismos del ejecutable
    chmod("woody", woody->file_mode);

    return (0);
}
