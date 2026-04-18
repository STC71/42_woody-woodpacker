#include "woody.h"

/* ==========================================================================
 * 🎬 EL DIRECTOR: main.c
 * ==========================================================================
 * Este archivo funciona como el portero y el director del hospital.
 * Es el primer código que se ejecuta. Su trabajo es:
 * 1. Comprobar que el "paciente" (el archivo a encriptar) existe y es válido.
 * 2. Clonar todo el archivo en una camilla de la Memoria RAM (mmap) para
 *    poder operarlo sin dañar el disco duro original.
 * 3. Delegar el trabajo al resto de especialistas (Explorador, Cirujano, etc.).
 * ========================================================================== */

/* 
 * 🧹 cleanup_woody
 * Como buenos médicos, siempre limpiamos el quirófano antes de irnos.
 * Esta función libera la Memoria RAM que habíamos reservado para operar al archivo.
 */
void cleanup_woody(t_woody *woody)
{
    if (woody->addr && woody->addr != MAP_FAILED)
    {
        munmap(woody->addr, woody->size);   // munmap = "Memory Un-Map" (Desmapear memoria)
        woody->addr = NULL;
    }
}

/* 
 * 🛂 check_elf_format
 * El portero de discoteca. Revisa el carné de identidad (la cabecera) del 
 * archivo para asegurarse de que es un software válido de Linux de 64-bits.
 */
static int check_elf_format(t_woody *woody)
{
    // 1. ¿Es tan pequeño que ni siquiera tiene cabecera?
    if (woody->size < sizeof(Elf64_Ehdr))
    {
        fprintf(stderr, ERR_NOT_ELF);
        return (-1);
    }

    // Leemos la cabecera (Ehdr = Executable Header)
    woody->ehdr = (Elf64_Ehdr *)woody->addr;

    // 2. Comprobar el Número Mágico. Todos los programas Linux empiezan por "\x7f E L F"
    if (memcmp(woody->ehdr->e_ident, ELFMAG, SELFMAG) != 0)
    {
        fprintf(stderr, ERR_NOT_ELF);
        return (-1);
    }

    // 3. Comprobar si es un programa de 64-bits (El proyecto Woody Woodpacker exige 64-bits)
    if (woody->ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    {
        fprintf(stderr, ERR_NOT_64);
        return (-1);
    }

    // 3.5. Comprobar Little Endian (El procesador leería los Offsets al revés si es Big Endian)
    if (woody->ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    {
        fprintf(stderr, "Error: Archivo en formato Big-Endian. Peligro de corrupción de Offsets. Abortado.\n");
        return (-1);
    }

    // 4. Comprobar que el tipo de procesador es compatible (x86_64 = ordenadores modernos comunes)
    if (woody->ehdr->e_machine != EM_X86_64)
    {
        fprintf(stderr, ERR_NOT_X86_64);
        return (-1);
    }

    // 5. Comprobar si es Ejecutable (ET_EXEC) o Libre de Posición (ET_DYN aka PIE)
    // No podemos inyectar virus en simples archivos de texto, tiene que ser un programa.
    if (woody->ehdr->e_type != ET_EXEC && woody->ehdr->e_type != ET_DYN)
    {
        fprintf(stderr, ERR_NOT_EXEC);
        return (-1);
    }

    return (0); // ¡El paciente es válido para cirugía!
}

/* 
 * 🛏️ init_woody
 * El camillero. Se encarga de coger el archivo del disco duro y subirlo
 * a la Memoria RAM privada para que los cirujanos trabajen seguros allí.
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

    // Obtenemos sus estadísticas (cuánto pesa, quién es el dueño, etc.)
    // También comprobamos que no nos hayan pasado una 'Carpeta' en lugar de un Archivo.
    if (fstat(fd, &st) < 0 || S_ISDIR(st.st_mode))
    {
        fprintf(stderr, ERR_FSTAT);
        close(fd);
        return (-1);
    }

    woody->size = st.st_size;       // Guardamos su peso (vital para no leer fuera de la memoria luego y crashear).
    woody->file_mode = st.st_mode;  // Guardamos sus permisos (ej. "rwxr-xr-x") para clonarlos intactos al final.
    
    // LA MAGIA: mmap (Memory Map). 
    // Le pedimos al Sistema Operativo que nos suba el archivo del disco duro a la Memoria RAM.
    // MAP_PRIVATE significa: "Cualquier cosa que yo modifique o rompa aquí, NO SE GUARDARÁ en el disco duro".
    woody->addr = mmap(NULL, woody->size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);  // Ya tenemos el archivo en RAM, cerramos el disco duro.

    if (woody->addr == MAP_FAILED)
    {
        fprintf(stderr, ERR_MMAP);
        return (-1);
    }

    // Comprobamos que el archivo en la RAM cumple todos los requisitos de identidad
    if (check_elf_format(woody) < 0)
    {
        cleanup_woody(woody); // Falló, limpiamos el quirófano.
        return (-1);
    }

    return (0); // Todo listo para empezar.
}

/* 
 * 🏁 MAIN
 * El punto de entrada inicial de todo Woody Woodpacker.
 */
int main(int argc, char **argv)
{
    t_woody woody;

    // Inicializamos nuestro "Expediente Médico" (la estructura t_woody) llenándola de ceros vacíos.
    memset(&woody, 0, sizeof(t_woody));

    // Si el usuario no escribe "./woody_woodpacker programa_victima", le regañamos amablemente.
    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Uso: ./woody_woodpacker <binario> [clave-hex-de-16-bytes]\n");
        return (1);
    }

    // 1. Tumbamos al paciente en la camilla de la RAM
    if (init_woody(argv[1], &woody) < 0)
        return (1);

    // 2. ¿Me han entregado una llave hecha a mano, o inventamos una llave secreta aleatoria? (Módulo RC4)
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

    // Mostramos la llave secreta inyectable en pantalla para fardar (estilo Hacker clásico).
    printf("CLAVE [128-bit RC4]: 0x");
    for (size_t i = 0; i < woody.key_len; i++)
        printf("%02X", woody.key[i]);
    printf("\n");

    printf("¡ÉXITO! Archivo '%s' mapeado en la dirección %p\n", argv[1], woody.addr);
    printf("Original Entry Point (OEP) original reside en 0x%lx\n", woody.ehdr->e_entry);

    // 3. Llamamos al EXPLORADOR (elf_parser.c) para que analice las entrañas 
    //    del programa y decida DÓNDE hay un "Code Cave" (agujero sobrante) para esconder nuestro virus.
    if (parse_elf(&woody) < 0)
    {
        cleanup_woody(&woody);
        return (1);
    }

    // 4. Llamamos al CERRAJERO (crypto.c)
    //    El cerrajero toma el bloque del código original del programa y lo fríe (encripta) 
    //    con la Llave Secreta RC4 que le indicamos en el paso 2, para que los antivirus no lo huelan.
    encrypt_text_section(&woody);

    // 5. Llamamos al CIRUJANO INYECTOR (injector.c)
    //    Coge nuestro virus escrito en Ensamblador (payload.bin), le escribe dentro la llave y 
    //    las coordenadas con las que el virus tendrá que desencriptar el programa, 
    //    y lo pega físicamente dentro de la "Code Cave" libre que encontró el Explorador (paso 3).
    //    Finalmente, guarda todos nuestros cambios y crea un nuevo bloque llamado 'woody'.
    if (generate_and_inject_payload(&woody) < 0)
    {
        cleanup_woody(&woody);
        return (1);
    }

    // 6. Fin de la operación. Limpiamos la camilla de la RAM. ¡Paciente dado de alta!
    cleanup_woody(&woody);
    return (0);
}
