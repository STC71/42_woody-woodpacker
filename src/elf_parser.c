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
    // La función toma como argumentos un puntero a la estructura t_woody, 
    // un puntero genérico (ptr) que queremos verificar y que recibirá la funcion con la dirección que queremos leer, 
    // y el tamaño de bytes que necesitamos leer a partir de ese puntero (size_needed).
    // El objetivo de esta función es asegurarse de que el puntero ptr es seguro de usar, es decir, que no apunta 
    // a una dirección fuera de los límites del archivo mapeado en memoria.
    // O sea prevenir underflows y overflows de memoria que podrían causar que el programa crashee o se comporte de 
    // manera inesperada al intentar acceder a memoria no válida.
    if ((uint8_t *)ptr < (uint8_t *)woody->addr)
    {
        // (uint8_t *)ptr es una conversión de tipo que interpreta el puntero ptr como un puntero a un byte (uint8_t).
        // (uint8_t *)woody->addr es una conversión de tipo que interpreta el puntero woody->addr como un puntero 
        // a un byte (uint8_t).
        // Si el puntero ptr apunta a una dirección que es menor que la dirección base del archivo mapeado en memoria 
        // (woody->addr), entonces ptr estaría apuntando a una dirección fuera de los límites del archivo, lo cual es peligroso. 
        // En este caso, imprimimos un mensaje de error y devolvemos 0 para indicar que el puntero no es seguro.
        fprintf(stderr, "Error: Puntero malicioso por debajo del inicio del archivo\n");
        return (0);
    }
    
    // Prevenir overflow aritmético: asegurar que ptr + size no dé la vuelta
    if ((uintptr_t)ptr + size_needed < (uintptr_t)ptr)
    {
        fprintf(stderr, "Error: Desbordamiento de enteros detectado\n");
        return (0);
    }

    // Si la "dirección + lo que queremos leer" supera el Final del Archivo (addr + size)...
    if ((uint8_t *)ptr + size_needed > (uint8_t *)woody->addr + woody->size)
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
    // Elf64_Shdr es una estructura que representa la cabecera de una sección en un archivo ELF que se define en <elf.h>. 
    // Contiene información sobre la sección, como su nombre, tipo, tamaño, offset, etc.
    char        *strtab;        // El diccionario propiamente dicho que traduce los nombres.
    // Los nombres de los punteros asignados a shdr, strtab_sh y strtab son variables locales que se utilizan para navegar 
    // por la estructura del archivo ELF. Estos nombres son elegidos para reflejar su propósito.
    // shdr se utiliza para recorrer la tabla de secciones y acceder a cada sección individualmente.
    // strtab_sh se utiliza para acceder a la sección que contiene la tabla de cadenas (string table) que almacena los 
    // nombres de las secciones.
    // strtab es un puntero que apunta al inicio de la tabla de cadenas, lo que nos permite acceder a los nombres de las 
    // secciones utilizando los índices proporcionados en la tabla de secciones.

    // 1. Ir a la Tabla General de Secciones (SHT - Section Header Table)
    //  La tabla general de secciones es un índice que nos dice dónde están todas las secciones del archivo ELF.
    //  secciones son como los capítulos de un libro, cada una con su propio propósito (código, datos, símbolos, etc.).
    shdr = (Elf64_Shdr *)(woody->addr + woody->ehdr->e_shoff);
    // e_shoff es el offset (distancia en bytes desde el inicio del archivo hasta la tabla de secciones) 
    // que se encuentra en la cabecera ELF (Elf64_Ehdr).
    if (!is_safe_ptr(woody, shdr, woody->ehdr->e_shnum * sizeof(Elf64_Shdr)))
        return (-1);
        // Antes de acceder a la tabla de secciones, utilizamos la función is_safe_ptr para asegurarnos de que el puntero shdr
        // es seguro de usar, es decir, que no apunta a una dirección fuera de los límites del archivo mapeado en memoria. 
        // woody->ehdr->e_shnum es el número total de secciones en el archivo ELF, y sizeof(Elf64_Shdr) es el tamaño de cada 
        // sección, por lo que estamos verificando que el rango de memoria que aborda la tabla de secciones es seguro de 
        // acceder.

    // 2. Para saber cómo se llama cada Sección, no podemos leer el nombre directamente,
    //    tenemos que buscarlo en un diccionario (String Table) cuyo índice lo tiene la cabecera (e_shstrndx).
    //  El diccionario es una sección especial que contiene los nombres de todas las demás secciones,
    //  que se encuentra alojado dentro del mismo archivo ELF. 
    //  La cabecera ELF tiene un campo llamado e_shstrndx que nos dice en qué sección se encuentra este diccionario.
    //  e_shstrndx es un campo que se almacena en la cabecera ELF (Elf64_Ehdr) y que indica el índice de la sección 
    //  que contiene la tabla de cadenas (string table) con los nombres de las secciones.
    // Vulnerabilidad evitada: el índice del diccionario no puede superar el número total de secciones.
    if (woody->ehdr->e_shstrndx >= woody->ehdr->e_shnum)
        return (-1);
        // Si el índice del diccionario (e_shstrndx) es mayor o igual al número total de secciones (e_shnum),
        // entonces el archivo ELF está corrupto o mal formado, ya que estaría apuntando a una sección que no existe. 
        // En este caso, devolvemos -1 para indicar un error.
    
    strtab_sh = &shdr[woody->ehdr->e_shstrndx];
    if (!is_safe_ptr(woody, woody->addr + strtab_sh->sh_offset, strtab_sh->sh_size))
        return (-1);
    
    strtab = (char *)(woody->addr + strtab_sh->sh_offset);

    // 3. Ya teniendo diccionario en mano, recorremos hoja por hoja (todas las secciones).
    for (int i = 0; i < woody->ehdr->e_shnum; i++)
    {
        // Vulnerabilidad evitada: el índice del diccionario no puede superar el propio tamaño del diccionario.
        if (shdr[i].sh_name >= strtab_sh->sh_size)
            continue; // Atroz corrupción en sh_name dictada por ELF corrupto.

        // ¿El nombre de la sección número "i" es exactamente igual a ".text"? (Seguro con strncmp limitando los carácteres)
        if (strncmp(strtab + shdr[i].sh_name, ".text", 6) == 0)
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
    //    tenemos hasta chocar contra el inicio del *siguiente* segmento tanto
    //    física como virtualmente en RAM?
    for (int i = 0; i < woody->ehdr->e_phnum; i++)
    {
        next_phdr = &phdr[i];
        // Comprobar la colisión física en el disco (Offset)
        if (next_phdr->p_offset > woody->target_segment->p_offset && 
            next_phdr->p_offset < lowest_next_offset)
        {
            lowest_next_offset = next_phdr->p_offset;
        }
        
        // Evitar desalineamientos drásticos de paginación virtual (Memoria)
        if (next_phdr->p_vaddr > woody->target_segment->p_vaddr && 
            next_phdr->p_vaddr - woody->target_segment->p_vaddr + woody->target_segment->p_offset < lowest_next_offset)
        {
            lowest_next_offset = next_phdr->p_vaddr - woody->target_segment->p_vaddr + woody->target_segment->p_offset;
        }
    }

    // Inicio exacto de nuestra cueva: Justo donde acaban los datos "vitales" del segmento ejecutable
    woody->cave_offset = woody->target_segment->p_offset + woody->target_segment->p_filesz;
    
    // ¿Cuánta capacidad tiene nuestra cueva? (Final del hueco - Inicio del hueco).
    if (lowest_next_offset >= woody->cave_offset)
        woody->cave_size = lowest_next_offset - woody->cave_offset;
    else
        woody->cave_size = 0;

    // Verificar iterativamente y exhaustivamente que la zona esté compuesta por bytes de relleno seguros.
    // Entendemos por seguros aquellos bytes que no contienen código ejecutable ni datos importantes, como ceros (0x00) 
    // o instrucciones de NOP (0x90 en x86-64) o INT3 (0xCC), que a menudo se utilizan como relleno en los archivos ELF.
    // 0x00 es un byte de relleno común que no representa una instrucción ejecutable = espacio vacio en memoria.
    // 0x90 es la instrucción NOP (No Operation), que no hace nada y se utiliza a menudo como relleno para alinear el código.
    // 0xCC es la instrucción INT3, que es una interrupción de depuración que también se utiliza a menudo como relleno 
    // para marcar el final de una sección o para alinear el código. Una interrupción de depuración es una instrucción que, 
    // cuando se ejecuta, provoca que el programa se detenga y permita al depurador tomar el control para inspeccionar el 
    // estado del programa en ese momento.
    // Estos bytes son considerados seguros para nuestro proyecto. 
    // bytes de relleno no seguros serían aquellos que contienen código ejecutable o datos importantes, lo cual podría 
    // causar un comportamiento inesperado o un crash si nuestro virus intenta escribir allí, por ejemplo:
    // 0xC3 (ret - instrucción de retorno en x86-64)
    // 0xE8 (call - instrucción de llamada a función en x86-64)
    // 0xFF (jmp, call indirecto, etc. - instrucciones de salto o llamada en x86-64)
    // 0x48 (prefijo de instrucciones de 64 bits en x86-64, como mov rax, rbx)
    // 0x89 (mov r64, r64 - para mover datos entre registros o entre memoria y registros en x86-64)
    // 0x50-0x5F (push/pop rax, rbx, etc. - instrucciones de manipulación de la pila en x86-64) ...
    size_t zero_padding_size = 0;
    uint8_t *cave_ptr = (uint8_t *)(woody->addr + woody->cave_offset);
    while (zero_padding_size < woody->cave_size && 
          (cave_ptr[zero_padding_size] == 0x00 || 
           cave_ptr[zero_padding_size] == 0x90 || 
           cave_ptr[zero_padding_size] == 0xCC))
        zero_padding_size++;
    
    if (zero_padding_size < woody->cave_size)
    {
        // zero_padding_size es la cantidad de bytes de relleno seguros (0x00, 0x90 o 0xCC) que hemos encontrado en la cueva.
        // Si zero_padding_size es menor que woody->cave_size, significa que no toda la cueva está compuesta por bytes de 
        // relleno seguros, lo cual es un riesgo para nuestro virus. En este caso, ajustamos el tamaño de la cueva a la 
        // cantidad de bytes de relleno seguros que hemos encontrado y emitimos una advertencia al usuario indicando que 
        // la cueva contiene datos basura y que se ha ajustado el tamaño de la cueva a los bytes puros de relleno seguros.
        printf("Advertencia: El hueco contiene datos basuras en %lu bytes. Ajustando tamaño de la cave a %lu bytes puros 0x00.\n", 
               woody->cave_size, zero_padding_size);
        woody->cave_size = zero_padding_size;
        // Redimensionamos la cueva al tamaño de bytes de relleno seguros encontrados.
        // Esta situación no es ideal, pero es mejor tener una cueva más pequeña y segura que arriesgar un crash del programa 
        // al intentar escribir en una zona con datos basura.
    }

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
