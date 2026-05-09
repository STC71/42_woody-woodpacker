#include "woody.h"

/* ==========================================================================
 * 🔐 EL CERRAJERO RC4: crypto.c
 * ==========================================================================
 * Si el "Explorador" nos dijo dónde está la lógica del programa (.text), 
 * el "Cerrajero" es quien se encarga de ponerle un candado criptográfico
 * indescifrable a esa zona. 
 *
 * Usamos el algoritmo RC4 (Rivest Cipher 4):
 * - Es simétrico (la misma llave cifra y descifra).
 * - Es de nivel de flujo (Stream Cipher): Cifra byte a byte continuo, 
 *   sin importar el tamaño. No necesita bloques ni rellenos padding.
 * - Es un clásico incombustible del Malware por ser increíblemente rápido.
 * ========================================================================== */

/*
 * 🔑 parse_custom_key (Revisión de Llave por Encargo)
 * ----------------------------------------------------
 * Como "Bonus" del proyecto, permitimos al usuario traer su propia llave
 * maestra (un string hexadecimal). Esta función es una revisión del tasador: 
 * comprueba que la llave no sea falsa y tenga el peso exacto (16 bytes = 32 letras).
 */
int parse_custom_key(t_woody *woody, const char *hex_str)
{
    size_t len = strlen(hex_str);
    char   byte_str[3] = {0};

    // Para una clave firme de 128-bits, necesitamos exactamente 32 letras/números hex.
    // Cada byte de la clave se representa con dos caracteres hexadecimales (00 a FF), 
    // por lo que para una clave de 16 bytes (128 bits)
    if (len != 32)
    {
        // Si la longitud de la cadena hexademinal que hemos extraido midiendo con strlen el string hex_str
        // que tenemos en la función parse_custom_key no es igual a 32, entonces la clave proporcionada 
        // por el usuario no tiene el peso exacto de 16 bytes (128 bits) que se requiere para nuestro proyecto, lo cual es un error. 
        fprintf(stderr, "Error: Bonus Custom Key debe constar exactamente de 32 caracteres hexadecimales (16 bytes).\n");
        return (-1);
    }

    woody->key_len = 16;
    for (size_t i = 0; i < 16; i++)
    {
        // El tasador usa su lupa: ¿Son caracteres hexadecimales legales (0-9, a-f, A-F)?
        if (!strchr("0123456789abcdefABCDEF", hex_str[i * 2]) ||
            !strchr("0123456789abcdefABCDEF", hex_str[i * 2 + 1]))
        {
            fprintf(stderr, "Error: Carácter hexadecimal inválido en la clave.\n");
            return (-1);
        }

        // Traducimos de "Texto visible" a "Byte puro para la máquina" ("FF" -> 255)
        byte_str[0] = hex_str[i * 2];
        byte_str[1] = hex_str[i * 2 + 1];
        woody->key[i] = (uint8_t)strtoul(byte_str, NULL, 16);
    }
    return (0); // Llave admitida por el cerrajero
}

/*
 * 🎲 generate_random_key (Forjando azar puro del Abismo)
 * --------------------------------------------------------
 * Si el usuario no trae llave, forjamos una totalmente impredecible. 
 * Los ordenadores fallan inventando el "azar absoluto", así que extraemos
 * magia entrópica termodinámica pura del núcleo de Linux: /dev/urandom.
 */
void generate_random_key(t_woody *woody)
{
    // Para generar una clave aleatoria, intentamos leer directamente de /dev/urandom, 
    // que es una fuente de datos aleatorios proporcionada por el sistema operativo Linux.
    int fd;

    woody->key_len = 16; // 128-Bits de pura paranoia
    // Guardamos la longitud de la clave en la estructura t_woody para referencia futura, 
    // aunque en este caso siempre será 16 bytes (128 bits), lo cual es un estándar común para claves simétricas de cifrado.
    // Esto es útil para mantener la flexibilidad en caso de que en el futuro se quiera cambiar la longitud de la clave 
    // sin tener que modificar otras partes del código que dependen de esta información.
    
    // Conectamos una manguera a la turbina del caos del Kernel
    fd = open("/dev/urandom", O_RDONLY);
    // Como sabemos /dev/urandom es un dispositivo especial en sistemas Unix que proporciona datos aleatorios.
    // Al abrirlo con O_RDONLY, estamos solicitando acceso de solo lectura a esta fuente de datos aleatorios, 
    // lo que nos permitirá leer bytes aleatorios para generar nuestra clave criptográfica.
    if (fd < 0)
    {
        // ¡Plan B! Si la manguera se rompe, retrocedemos al patético pseudoazar.
        fprintf(stderr, "Advertencia: /dev/urandom inaccesible. Retrocediendo a rand().\n");
        srand(time(NULL));
        for (size_t i = 0; i < woody->key_len; i++)
            woody->key[i] = rand() % 256;
    }
    else
    {
        // Rellenamos el molde de nuestra llave con el estruendo de urandom
        if (read(fd, woody->key, woody->key_len) != (ssize_t)woody->key_len)
        {
            // read toma tres argumentos: el descriptor de archivo (fd), el buffer donde se almacenarán los datos (woody->key) 
            // y la cantidad de bytes a leer (woody->key_len) su longitud (en este caso, 16 bytes para una clave de 128 bits).
            // Si read es diferente de woody->key_len, significa que no se pudieron leer los 16 bytes completos de /dev/urandom, 
            // lo cual es un error crítico para la generación de la clave, por lo que se emite una advertencia y se retrocede 
            // al método de generación de clave menos seguro utilizando rand().
            fprintf(stderr, "Advertencia: Lectura de /dev/urandom fallida. Retrocediendo a rand().\n");
            srand(time(NULL));  
            // srand es una función que se utiliza para establecer la semilla del generador de números aleatorios en C.
            // srand toma como argumento time(NULL), que devuelve el tiempo actual en segundos desde el Epoch (1 de enero de 1970).
            // Al usar el tiempo actual como semilla, se garantiza que cada vez que se ejecute el programa, la secuencia 
            // de números aleatorios generada por rand() será diferente, proporcionando una clave más segura.
            for (size_t i = 0; i < woody->key_len; i++)
                woody->key[i] = rand() % 256;
                // rand() es una función que genera un número pseudoaleatorio. Al usar rand() % 256, se obtiene un número 
                // entre 0 y 255, lo que es adecuado para llenar cada byte de la clave con un valor aleatorio.
                // La relación entre srand() y rand() es que srand() establece la semilla para el generador de números 
                // aleatorios utilizado por rand(). Es como configurar el punto de partida para la secuencia de números 
                // aleatorios que rand() generará. Si no se llama a srand(), rand() utilizará una semilla predeterminada, 
                // lo que resultará en la misma secuencia de números aleatorios cada vez que se ejecute el programa, 
                // lo cual no es deseable para la generación de claves criptográficas.
        }
        close(fd); // Cerramos el grifo
    }
}

/*
 * ⚙️ rc4_cipher (El Motor Criptográfico Estriador)
 * --------------------------------------------------
 * Aquí sucede la metamorfosis destructiva del tejido del programa.
 * RC4 es mortalmente bonito y funciona con una elegante danza en 2 partes:
 * 1. KSA: Repartir y barajar un mazo de 256 Naipes siguiendo un patrón extraído de la Llave.
 * 2. PRGA: Robar una Carta del mazo para hacer un XOR (Destrucción Binaria Lógica) 
 *          con el byte vital de la víctima. 
 * (La magia del "XOR" ^ es brillante: si a esa basura inofensiva le vuelves a
 * aplicar el XOR de la MISMA carta de antes... ¡se cura y revive el original!)
 */
static void rc4_cipher(uint8_t *data, size_t data_len, uint8_t *key, size_t key_len)
{
    uint8_t S[256];
    uint8_t temp;
    size_t  i, j = 0;

    // FASE 1: KSA (Key-Scheduling Algorithm) - Barajando el Panteón
    // Compramos un mazo de 256 cartas ordenado (del 0 al 255)
    for (i = 0; i < 256; i++)
        S[i] = i;

    // Y lo revolvemos frenéticamente usando nuestra Llave Radiactiva.
    for (i = 0; i < 256; i++)
    {
        j = (j + S[i] + key[i % key_len]) % 256;
        // Efecto SWAP Cásico: Intercambiamos cartas para sumir el mazo en el desorden general
        temp = S[i];
        S[i] = S[j];
        S[j] = temp;
    }

    // FASE 2: PRGA (Pseudo-Random Generation) - Aniquilando la Víctima (XOR)
    i = 0;
    j = 0;
    for (size_t n = 0; n < data_len; n++)
    {
        i = (i + 1) % 256;
        j = (j + S[i]) % 256;
        
        // El mazo de Naipes RC4 cambia en Vuelo al leer
        temp = S[i];
        S[i] = S[j];
        S[j] = temp;
        
        // ¡El tiro en la sien! (XOR En C - ^) Trasforma instrucciones sagradas (ej: 'Mov RAX, 5')
        // en basura irreconocible (ej: '0x9E 0xF7 ...')
        data[n] ^= S[(S[i] + S[j]) % 256];
    }
}

/*
 * 💥 encrypt_text_section (La Autorización de Impacto)
 * ----------------------------------------------------
 * Desencadena la maniobra pesada que descubrió el "Explorador" en elf_parser.c
 * Invoca oficialmente a RC4 sobre la arteria de código sensible.
 */
int encrypt_text_section(t_woody *woody)
{
    uint8_t *target_ptr;    // El Bisturí inyector: Puntero Temporal de RAM a RAM.
    size_t  target_size;    // El Tamaño del tejido a destruir (tamaño de la sección .text, por ende el bloque de código original).

    // Usamos las coordenadas guardadas de Woody para invocar localmente al motor sobre la víctima.
    target_ptr = (uint8_t *)(woody->addr + woody->text_section->sh_offset);
    target_size = woody->text_section->sh_size;

    printf("Encriptando sección .text desde offset 0x%lx (%lu bytes) mediante RC4...\n", 
           woody->text_section->sh_offset, target_size);

    // Activamos la trituradora
    rc4_cipher(target_ptr, target_size, woody->key, woody->key_len);
    
    printf("Encriptación completada exitosamente.\n");
    return (0);
}
