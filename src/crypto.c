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
    if (len != 32)
    {
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
 * Los ordenadores apestan inventando el "azar absoluto", así que extraemos
 * magia entrópica termodinámica pura del núcleo de Linux: /dev/urandom.
 */
void generate_random_key(t_woody *woody)
{
    int fd;

    woody->key_len = 16; // 128-Bits de pura paranoia
    
    // Conectamos una manguera a la turbina del caos del Kernel
    fd = open("/dev/urandom", O_RDONLY);
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
            fprintf(stderr, "Advertencia: Lectura de /dev/urandom fallida. Retrocediendo a rand().\n");
            srand(time(NULL));
            for (size_t i = 0; i < woody->key_len; i++)
                woody->key[i] = rand() % 256;
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
    size_t  target_size;

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
