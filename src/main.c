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
        // Un archivo ELF es un formato de archivo ejecutable común en sistemas Linux. 
        // La cabecera ELF contiene información crucial sobre el archivo, como su tipo, su arquitectura, su punto de entrada, etc.
        // ELF64_Ehdr provine de la biblioteca <elf.h> y define el tamaño de la cabecera ELF de 64 bits.
        // Si el tamaño del archivo es menor que el tamaño de la cabecera, entonces no es un 
        // archivo ELF válido, porque un archivo ELF siempre debe tener al menos el tamaño de 
        // su cabecera.
        // woody->size es una variable que contiene el tamaño del archivo que hemos mapeado en memoria y que 
        // está alojada en la estructura t_woody. Esta variable se establece al principio del programa cuando leemos 
        // el archivo del disco duro y lo mapeamos en memoria.
        fprintf(stderr, ERR_NOT_ELF);   
        // fprintf es una función de la biblioteca estándar de C que se utiliza para imprimir mensajes de error 
        // en la salida estándar de errores (stderr). La diferencia con printf es que fprintf permite especificar 
        // el flujo de salida, en este caso stderr, que es comúnmente utilizado para mensajes de error.
        // stderr es un flujo de salida estándar que se utiliza para imprimir mensajes de error. 
        // Es una variable global definida en la biblioteca estándar de C que representa la salida de errores del programa. 
        // Al usar fprintf(stderr, ...), estamos enviando el mensaje de error a la salida de errores en lugar de a la 
        // salida estándar (stdout).
        // ERR_NOT_ELF es una macro definida en el archivo woody.h que contiene el mensaje de error específico 
        // para indicar que el archivo no es un ELF válido.
        return (-1);    // No es un archivo ELF válido, abortamos la operación. -1 = algo salió mal.
    }

    // Leemos la cabecera (Ehdr = Executable Header)
    woody->ehdr = (Elf64_Ehdr *)woody->addr;

    // 2. Comprobar el Número Mágico. Todos los programas Linux empiezan por "\x7f E L F"
    if (memcmp(woody->ehdr->e_ident, ELFMAG, SELFMAG) != 0)
    {
        // memcmp es una función de la biblioteca estándar de C que compara dos bloques de memoria byte por byte.
        // En este caso, estamos comparando los primeros bytes de la cabecera ELF (woody->ehdr->e_ident) con la 
        // secuencia mágica definida por ELFMAG (que es "\x7f E L F") y SELFMAG (que es 4, el tamaño de la secuencia mágica).
        // Si memcmp devuelve 0, significa que los primeros bytes de la cabecera coinciden con la secuencia mágica 
        // de un archivo ELF, lo que indica que es un archivo ELF válido. 
        // Si memcmp devuelve un valor diferente de 0, significa que los primeros bytes no coinciden,
        // (secuencia mágica = "magic number" = conjunto de bytes al inicio de un archivo que identifica su formato)
        // memcmp es una función de la biblioteca estándar de C que compara dos bloques de memoria byte por byte. 
        // En este caso, estamos comparando los primeros bytes de la cabecera ELF
        // woody->ehdr->e_ident es un array que forma parte de la estructura de la cabecera ELF (Elf64_Ehdr) 
        // y contiene información de identificación del archivo ELF, incluyendo el número mágico.
        fprintf(stderr, ERR_NOT_ELF);
        return (-1);
    }

    // 3. Comprobar si es un programa de 64-bits (El proyecto Woody Woodpacker exige 64-bits)
    if (woody->ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    {
        // EI_CLASS es un índice en el array e_ident que indica la clase de archivo ELF, es decir, 
        // si es de 32 bits o de 64 bits.
        // ELFCLASS64 es una constante que indica que el archivo es de 64 bits.
        // Si el valor en e_ident[EI_CLASS] no es igual a ELFCLASS64, entonces el archivo no es un programa de 64 bits, 
        // lo cual es un requisito para nuestro proyecto. 
        fprintf(stderr, ERR_NOT_64);
        return (-1);
    }

    // 3.5. Comprobar Little Endian (El procesador leería los Offsets al revés si es Big Endian)
    if (woody->ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    {
        // EI_DATA es un índice en el array e_ident que indica el tipo de codificación de datos del archivo ELF, es decir,
        // si es Little Endian o Big Endian.
        // ELFDATA2LSB es una constante que indica que el archivo utiliza codificación de datos Little Endian, lo cual 
        // es común en sistemas x86_64.
        // Si el valor en e_ident[EI_DATA] no es igual a ELFDATA2LSB, entonces el archivo utiliza codificación de datos 
        // Big Endian, lo cual no es compatible con nuestro proyecto
        fprintf(stderr, ERR_NOT_LITTLE_ENDIAN);
        return (-1);
    }

    // 4. Comprobar que el tipo de procesador es compatible (x86_64 = ordenadores modernos comunes)
    if (woody->ehdr->e_machine != EM_X86_64)
    {
        // e_machine es un campo en la cabecera ELF que indica el tipo de arquitectura de máquina para la cual está 
        //destinado el archivo ELF.
        // EM_X86_64 es una constante que indica que el archivo ELF está destinado para la arquitectura x86_64, 
        // que es común en ordenadores modernos.
        // Si el valor en e_machine no es igual a EM_X86_64, entonces el archivo ELF no está destinado para la 
        // arquitectura x86_64, lo cual es un requisito para nuestro proyecto.
        fprintf(stderr, ERR_NOT_X86_64);
        return (-1);
    }

    // 5. Comprobar si es Ejecutable (ET_EXEC) o Libre de Posición (ET_DYN aka PIE)
    // No podemos inyectar virus en simples archivos de texto, tiene que ser un programa.
    if (woody->ehdr->e_type != ET_EXEC && woody->ehdr->e_type != ET_DYN)
    {
        // e_type es un campo en la cabecera ELF que indica el tipo de archivo ELF, como si es un ejecutable, 
        // un objeto compartido, etc.
        // ET_EXEC es una constante que indica que el archivo ELF es un ejecutable, es decir, un programa que puede 
        // ser ejecutado directamente por el sistema operativo.
        // ET_DYN es una constante que indica que el archivo ELF es un objeto compartido (shared object), 
        // que es un tipo de archivo ELF que puede ser cargado por otros programas en tiempo de ejecución, como una
        // biblioteca compartida. Los archivos PIE (Position Independent Executable) también se clasifican como ET_DYN.
        // Si el valor en e_type no es igual a ET_EXEC ni a ET_DYN, entonces el archivo ELF no es un ejecutable 
        // ni un objeto compartido, lo cual significa que no puede ser infectado.
        // Un objeto compartido es aquel que no está destinado a ser ejecutado directamente, sino que está diseñado 
        // para ser cargado por otros programas, como una biblioteca compartida (.so). Estos archivos no tienen un punto 
        // de entrada definido como los ejecutables. Un ejemplo sería el archivo libc.so, que es una biblioteca compartida 
        // que contiene funciones estándar de C.
        // Si el archivo, por ejemplo, es un archivo de texto o un script, no tendrá el formato ELF ni el tipo de archivo 
        // adecuado, por lo que no podría ser infectado por nuestro virus. Nuestro virus está diseñado para infectar 
        // programas ejecutables, no archivos de texto o scripts.
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
    // woody->file_mode se usa para asegurarnos de que el nuevo archivo 'woody' tenga los mismos permisos que el original, 
    // para que el usuario pueda ejecutarlo sin problemas después de la infección. Almacenamos los permisos originales del 
    // archivo para replicarlos en el nuevo archivo infectado, manteniendo así la misma "ropa" que el paciente llevaba 
    // puesta originalmente.
    // st.st_mode guarda el modo del archivo, incluyendo permisos y tipo de archivo.
    // st. es la estructura que contiene toda la información del archivo obtenida por fstat, incluyendo su tamaño (st_size), 
    // sus permisos (st_mode), su propietario (st_uid), etc.
    
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
    int arg_idx = 1;

    // Inicializamos nuestro "Expediente Médico" (la estructura t_woody) llenándola de ceros vacíos.
    memset(&woody, 0, sizeof(t_woody));
    woody.crypto_algo = 0; // Default RC4

    // Parseo de Banderas de Multi-Algoritmo (Fase 2 Bonus)
    if (argc >= 2)
    {
        if (strcmp(argv[arg_idx], "--xor") == 0)
        {
            woody.crypto_algo = 1;
            arg_idx++;
        }
        else if (strcmp(argv[arg_idx], "--rc4") == 0)
        {
            woody.crypto_algo = 0;
            arg_idx++;
        }
    }

    // Si el usuario no escribe "./woody_woodpacker [--xor|--rc4] programa_victima", le regañamos amablemente.
    if (argc < arg_idx + 1 || argc > arg_idx + 2)
    {
        fprintf(stderr, "Uso: ./woody_woodpacker [--xor|--rc4] <binario> [clave-hex-de-16-bytes]\n");
        return (1);
    }

    // 1. Tumbamos al paciente en la camilla de la RAM
    if (init_woody(argv[arg_idx], &woody) < 0)
        return (1);

    // 2. ¿Me han entregado una llave hecha a mano, o inventamos una llave secreta aleatoria? (Módulo RC4/XOR)
    if (argc == arg_idx + 2)
    {
        if (parse_custom_key(&woody, argv[arg_idx + 1]) < 0)
        {
            cleanup_woody(&woody);
            return (1);
        }
    }
    else
        generate_random_key(&woody);

    // Mostramos la llave secreta inyectable en pantalla para fardar (estilo Hacker clásico).
    if (woody.crypto_algo == 1)
        printf("CLAVE [128-bit XOR]: 0x");
    else
        printf("CLAVE [128-bit RC4]: 0x");
    for (size_t i = 0; i < woody.key_len; i++)
        printf("%02X", woody.key[i]);
    printf("\n");

    printf("¡ÉXITO! Archivo '%s' mapeado en la dirección %p\n", argv[arg_idx], woody.addr);
    printf("Original Entry Point (OEP) original reside en 0x%lx\n", woody.ehdr->e_entry);

    // 3. Llamamos al EXPLORADOR (elf_parser.c) para que analice las entrañas 
    //    del programa y decida DÓNDE hay un "Code Cave" (agujero sobrante) para esconder nuestro virus.
    if (parse_elf(&woody) < 0)
    {
        // La funcion parse_elf busca la seección .text que es donde reside el código original del programa, 
        // y también busca un "Code Cave" (espacio libre) donde podamos inyectar nuestro virus.
        // Encontramos el .text mediante el uso del código ensamblador en payload.s, que nos da las 
        // coordenadas relativas al inicio del programa, y luego las convertimos a Offsets absolutos.
        // Offsets nos indica a qué distancia del inicio del programa se encuentra cada sección, cada segmento, etc.
        // Esto se hace en elf_parser.c, que es el "Explorador" que se encarga de mapear el terreno y encontrar 
        // un buen lugar para operar.
        // Si parse_elf falla, (< 0) es que no hemos podido encontrar el corazón del paciente o un buen lugar para 
        // operar, lo cual es peligroso. No queremos operar a ciegas, así que abortamos la operación.
        // &woody es un puntero a la estructura t_woody que contiene toda la información del paciente, 
        // incluyendo su dirección en memoria, su tamaño, sus cabeceras ELF, etc. 
        // La estructura t_woody es como el expediente médico del paciente, donde guardamos toda la información 
        // relevante para la cirugía como su dirección en memoria, su tamaño, sus cabeceras ELF, la ubicación de la 
        // sección .text, la ubicación del code cave, la clave de cifrado, etc.
        // El explorador necesita esta información para hacer su trabajo de análisis y planificación de la cirugía.
        cleanup_woody(&woody);  // Limpiamos el quirófano porque no queremos dejar al paciente medio operado en la RAM.
        return (1);             // Abortamos la operación porque no podemos seguir sin un buen plan de cirugía.
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
