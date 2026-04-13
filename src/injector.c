#include "woody.h"

/* ==========================================================================
 * 💉 EL CIRUJANO TRAUMATOLÓGICO: injector.c
 * ==========================================================================
 * El "Explorador" nos dijo dónde estaba la herida (.text) y dónde había
 * hueco libre en la cavidad torácica (Code Cave). El "Cerrajero" encriptó
 * (anestesió/bloqueó) la herida original.
 *
 * Ahora llega el Cirujano. Su trabajo es brutal pero preciso:
 * 1. Importar la prótesis de titanio prefabricada (asm/payload.bin).
 * 2. Ajustar los tornillos de la prótesis (Parchear variables en ASM: deltas, clave RC4).
 * 3. Insertar la prótesis en el Code Cave.
 * 4. Reconectar el nervio óptico (Cambiar el Entry Point del archivo para que
 *    arranque la prótesis antes que el programa real).
 * 5. Cerrar al paciente y crear el clon ("./woody").
 * ========================================================================== */

int generate_and_inject_payload(t_woody *woody)
{
    int         fd;
    struct stat st;
    uint8_t     *payload;
    size_t      payload_size;
    uint64_t    *patch_ptr = NULL;

    printf("Inyectando Payload ASM... [Fase del Cirujano]\n");

    // 1. IMPORTAR LA PRÓTESIS (Leer payload.bin compilado por NASM en Makefile)
    fd = open("asm/payload.bin", O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "Error: Prótesis asm/payload.bin no encontrada. ¿Llamaste al orfebre (make)?\n");
        return (-1);
    }
    fstat(fd, &st);
    payload_size = st.st_size;
    payload = malloc(payload_size);
    read(fd, payload, payload_size);
    close(fd);

    // 2. COMPROBAR RECHAZO (¿Cabe la prótesis en el hueco del paciente?)
    if (payload_size > woody->cave_size)
    {
        fprintf(stderr, "Error: La prótesis (%lu bytes) es mayor que la cavidad (%lu bytes).\n", 
                payload_size, woody->cave_size);
        fprintf(stderr, "El paciente moriría por sobrepresión. Inyección abortada.\n");
        free(payload);
        return (-1);
    }

    // 3. INMUNOSUPRESORES: Dar permiso de ESCRITURA (PF_W) al segmento de memoria.
    // El SO Linux por defecto marca las zonas de código como "Read-Exec" (RX).
    // Si nuestro Payload intenta auto-desencriptarse (escribirse a sí mismo) en 
    // una zona sin permisos de escritura (W), el Kernel lo asesinaría por "Segfault".
    woody->target_segment->p_flags |= PF_W;

    // 4. LOCALIZAR LOS TORNILLOS (Buscar la firma 0x1111111111111111)
    // En nuestro ensamblador dejamos una marca visual tonta (1111...) para 
    // saber exactamente dónde inyectar nuestras distancias matemáticas y claves.
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
        fprintf(stderr, "Error: ¡Firma de tornillos de fijación no encontrada en el Payload!\n");
        free(payload);
        return (-1);
    }

    // 5. CÁLCULO DE RELATIVIDAD (Deltas de Memoria)
    // Cuando programas virus, no sabes en qué dirección de RAM exacta te va 
    // a cargar el sistema operativo (PIE - Position Independent Executable).
    // Por ende, no trabajamos con direcciones absolutas (ej. "Calle 5"), 
    // sino relativas (ej. "-2 calles para atrás respecto a donde estoy parado").
    size_t payload_vaddr = woody->target_segment->p_vaddr + (woody->cave_offset - woody->target_segment->p_offset);
    uint64_t vars_vaddr = payload_vaddr + ((uint8_t *)patch_ptr - payload);
    size_t text_vaddr = woody->target_segment->p_vaddr + (woody->text_section->sh_offset - woody->target_segment->p_offset);

    // --- 🔩 AJUSTANDO LOS TORNILLOS DEL PAYLOAD ---
    // [0] var_text: Distancia desde 'las variables' hasta el inicio de '.text' a desencriptar.
    patch_ptr[0] = (uint64_t)(text_vaddr - vars_vaddr);

    // [1] text_size: Tamaño exacto del tejido que debe curar/desencriptar.
    patch_ptr[1] = woody->text_section->sh_size;

    // [2] / [3] RC4 Key: Inyectamos los 16 bytes de "suero" (la clave criptográfica).
    memcpy(&patch_ptr[2], woody->key, 16);

    // [4] var_oep: Distancia de salto final (Original Entry Point). Cuando el Payload 
    //     termine, saltará hacia atrás para ejecutar el programa original como si nada.
    patch_ptr[4] = (uint64_t)(woody->ehdr->e_entry - vars_vaddr);

    // 6. EL CORTE NERVIOSO SECRETO (Secuestro del Punto de Entrada)
    // Le cambiamos el Entry Point al ELF. Ahora cuando el usuario haga ./woody, 
    // la CPU no empezará por el main() original, ¡empezará por nuestra prótesis!
    woody->ehdr->e_entry = payload_vaddr;

    // 7. SUTURAR: Meter la prótesis físicamente en el archivo de RAM mapeado
    memcpy(woody->addr + woody->cave_offset, payload, payload_size);

    // 8. ENGAÑAR A LOS RAYOS X (Actualizar cabeceras del sistema)
    // Le decimos a Linux que ahora este segmento pesa un poquito más para 
    // que cargue nuestra prótesis en RAM obligatoriamente.
    woody->target_segment->p_filesz += payload_size;
    woody->target_segment->p_memsz += payload_size;

    // 9. DAR EL ALTA (Guardar en un nuevo archivo físico './woody')
    fd = open("woody", O_CREAT | O_TRUNC | O_WRONLY, woody->file_mode);
    if (fd < 0)
    {
        fprintf(stderr, "Error: Cirugía fallida. No se pudo crear al paciente resultante 'woody'\n");
        free(payload);
        return (-1);
    }
    
    // Volcamos toda nuestra mesa de operaciones (RAM) al disco duro
    write(fd, woody->addr, woody->size);
    close(fd);
    free(payload);
    
    // Forzamos los permismos del ejecutable (ej. chmod +x) por si el sistema nos molesta
    chmod("woody", woody->file_mode);

    printf("¡Cirugía finalizada! El paciente clonado y empaquetado './woody' está preparado.\n");
    return (0);
}
