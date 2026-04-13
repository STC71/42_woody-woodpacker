#include "woody.h"

/* ==========================================================================
 * 🔎 EL EXPLORADOR Y ANALISTA FORENSE: elf_parser.c
 * ==========================================================================
 * Los programas en Linux (archivos ELF) no son un revoltijo de datos; tienen
 * una estructura ultra-estricta compuesta por "Secciones" y "Segmentos", 
 * casi como si fueran los capítulos de un manual de instrucciones de IKEA.
 *
 * El trabajo de este archivo es ponerse las gafas de aumento y:
 * 1. Encontrar las instrucciones vitales de la víctima (la sección ".text").
 * 2. Buscar un rincón vacío o "Code Cave" (cueva) donde podamos esconder 
 *    nuestro virus polizón parasitario sin alterar artificialmente el peso.
 * ========================================================================== */

/*
 * 🛡️ is_safe_ptr (El Guardia de Seguridad Perimetral)
 * ---------------------------------------------------
 * Antes de que pongamos el dedo en cualquier punto de la Memoria RAM del archivo,
 * este guardia revisa que no vayamos a "tocar fuera del límite". 
 * Los archivos corruptos o creados por analistas de antivirus a propósito 
 * usan cabeceras falsas que apuntan a zonas inválidas de la RAM para hacer 
 * que nuestro virus crashee (Core Dump) al intentar leer allí.
 */
static int is_safe_ptr(t_woody *woody, void *ptr, size_t size_needed)
{
    // Si la "dirección + lo que queremos leer" supera el Final del Archivo (addr + size)...
    if ((void *)ptr + size_needed > woody->addr + woody->size)
    {
        fprintf(stderr, "Error: Diseño ELF corrupto detectado (Acceso fuera de límites)\n");
        return (0); // ¡Peligro, abortar misión!
    }
    return (1); // Zona segura
}

/*
 * 📜 find_text_section (Buscando la Lógica del Programa)
 * ------------------------------------------------------
 * En el mundo ELF, la lógica de los if/else y el código fuente escrito
 * original reside dentro de una sección llamada estrictamente ".text".
 * Nuestro objetivo es atrapar sus coordenadas exactas para que luego, el 
 * "Cerrajero" (crypto.c), lo destroce matemáticamente y lo encripte.
 */
static int find_text_section(t_woody *woody)
{
    Elf64_Shdr  *shdr;          // Puntero genérico a Secciones (Section Header)
    Elf64_Shdr  *strtab_sh;     // Puntero a la sección del Diccionario (String Table)
    char        *strtab;        // El diccionario propiamente dicho que traduce los nombres.

    // 1. Ir a la Tabla General de Secciones (SHT - Section Header Table)
    shdr = (Elf64_Shdr *)(woody->addr + woody->ehdr->e_shoff);
    if (!is_safe_ptr(woody, shdr, woody->ehdr->e_shnum * sizeof(Elf64_Shdr)))
        return (-1);

    // 2. Para saber cómo se llama cada Sección, no podemos leer el nombre directamente,
    //    tenemos que buscarlo en un diccionario (String Table) cuyo índice lo tiene la cabecera (e_shstrndx).
    if (woody->ehdr->e_shstrndx >= woody->ehdr->e_shnum)
        return (-1);
    
    strtab_sh = &shdr[woody->ehdr->e_shstrndx];
    if (!is_safe_ptr(woody, woody->addr + strtab_sh->sh_offset, strtab_sh->sh_size))
        return (-1);
    
    strtab = (char *)(woody->addr + strtab_sh->sh_offset);

    // 3. Ya teniendo diccionario en mano, recorremos hoja por hoja (todas las secciones).
    for (int i = 0; i < woody->ehdr->e_shnum; i++)
    {
        // ¿El nombre de la sección número "i" es exactamente igual a ".text"?
        if (strcmp(strtab + shdr[i].sh_name, ".text") == 0)
        {
            woody->text_section = &shdr[i]; // ¡Encontrada! Guardamos las coordenadas en el expediente médico.
            printf("Encontrada sección: .text. Offset: 0x%lx, Tamaño: 0x%lx\n", 
                   woody->text_section->sh_offset, woody->text_section->sh_size);
            return (0);
        }
    }
    
    // Si recorremos todo y no hay .text, el programa víctima es inútil/inválido.
    fprintf(stderr, "Error: No se encontró la sección \".text\"\\n");
    return (-1);
}

/*
 * 🕳️ find_code_cave (Buscando la Cueva del Tesoro)
 * ------------------------------------------------
 * Cuando los compiladores de Linux (como gcc) ensamblan un ejecutable, 
 * por cuestiones de velocidad y eficiencia de lectura del disco duro, 
 * "alinean" los trozos del programa rellenando lo que sobra con ceros (Padding).
 * Esto crea zonas huecas gigantes entre el fin de un bloque y el inicio del siguiente.
 * A este relleno inútil los Hackers lo llamamos "Code Caves" (Cuevas de Código). 
 * Ahí es donde meteremos a nuestro pasajero Polizón sin engordar el fichero.
 */
static int find_code_cave(t_woody *woody)
{
    Elf64_Phdr  *phdr;              // Punteros a los "Program Headers" (Segmentos cargables en RAM)
    Elf64_Phdr  *next_phdr;         // Puntero para comparar el siguiente segmento y calcular el espacio libre entre ambos.
    size_t      lowest_next_offset; // Variable para guardar el offset del siguiente segmento más cercano después del segmento ejecutable.

    // Localizamos la lista maestra de Segmentos
    // phdr = ... "Ve a la dirección base del archivo mapeado en RAM, súmale el offset donde empiezan los Program Headers, y ahí encontrarás el primer segmento".
    // Elf64_Phdr pertenece a <elf.h> y es la estructura que define cada segmento en el formato ELF. 
    // Elf es un formato de archivo con una estructura muy rígida, y esta estructura nos dice exactamente dónde están los segmentos que el sistema operativo va a cargar en memoria para ejecutar el programa.
    // offset es la distancia en bytes desde el inicio del archivo hasta donde empieza la tabla de segmentos (Program Headers).
    // la tabla de segmentos es como un índice que le dice al sistema operativo qué partes del archivo cargar en memoria y con qué permisos (ejecución, lectura, escritura).
    phdr = (Elf64_Phdr *)(woody->addr + woody->ehdr->e_phoff);
    if (!is_safe_ptr(woody, phdr, woody->ehdr->e_phnum * sizeof(Elf64_Phdr)))
        return (-1);

    lowest_next_offset = woody->size; // Empezamos asumiendo que el límite es el fin del archivo.
    woody->target_segment = NULL;

    // 1. Buscar el segmento que la CPU va a CARGAR (PT_LOAD) y que tiene permisos
    //    totales de ejecución (PF_X). Si inyectamos un virus en un segmento de
    //    "sólo lectura", el SO lo fulminará con un Segmentation Fault al intentar arrancarlo.
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
        fprintf(stderr, "Error: No se encontró un segmento PT_LOAD ejecutable\n");
        return (-1);
    }

    lowest_next_offset = woody->size;
    // 2. Ahora que tenemos el Segmento Ejecutable, ¿cuánto espacio libre 
    //    tenemos hasta chocar contra el inicio del *siguiente* segmento útil?
    for (int i = 0; i < woody->ehdr->e_phnum; i++)
    {
        next_phdr = &phdr[i];
        if (next_phdr->p_offset > woody->target_segment->p_offset && 
            next_phdr->p_offset < lowest_next_offset)
        {
            lowest_next_offset = next_phdr->p_offset;
        }
    }

    // Inicio exacto de nuestra cueva: Justo donde acaban los datos "vitales" del segmento ejecutable
    woody->cave_offset = woody->target_segment->p_offset + woody->target_segment->p_filesz;
    
    // ¿Cuánta capacidad tiene nuestra cueva? (Final del hueco - Inicio del hueco).
    if (lowest_next_offset >= woody->cave_offset)
        woody->cave_size = lowest_next_offset - woody->cave_offset;
    else
        woody->cave_size = 0;

    printf("Encontrada Code Cave en Offset: 0x%lx con Capacidad: %lu bytes\n", 
           woody->cave_offset, woody->cave_size);
    
    return (0);
}

/*
 * 🚦 parse_elf (El Plan de Acción General)
 * -----------------------------------------
 * Función que manda a ejecutar orgánicamente todas las exploraciones. 
 * Se le invoca formalmente desde main.c.
 */
int parse_elf(t_woody *woody)
{
    // Fase 1 del Explorador: ¡Asegura el bloque de código a encriptar! (.text)
    if (find_text_section(woody) < 0)
        return (-1);

    // Fase 2 del Explorador: ¡Busca una sala vacía donde podamos esconder el virus!
    if (find_code_cave(woody) < 0)
        return (-1);

    return (0); // Ambos pasos completados con éxito rotundo.
}
