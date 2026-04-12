#include "woody.h"

/*
** Convierte un string en formato hexadecimal ("A1B2...") a array de bytes.
** Retorna -1 si la longitud o el formato son incorrectos.
*/
int parse_custom_key(t_woody *woody, const char *hex_str)
{
    size_t len = strlen(hex_str);
    char   byte_str[3] = {0};

    // Para una clave de 16 bytes necesitamos exactamente 32 caracteres hex
    if (len != 32)
    {
        fprintf(stderr, "Error: Bonus Custom Key debe constar exactamente de 32 caracteres hexadecimales (16 bytes).\n");
        return (-1);
    }

    woody->key_len = 16;
    for (size_t i = 0; i < 16; i++)
    {
        // Comprobar que solo haya caracteres hexadecimales válidos
        if (!strchr("0123456789abcdefABCDEF", hex_str[i * 2]) ||
            !strchr("0123456789abcdefABCDEF", hex_str[i * 2 + 1]))
        {
            fprintf(stderr, "Error: Carácter hexadecimal inválido en la clave.\n");
            return (-1);
        }

        byte_str[0] = hex_str[i * 2];
        byte_str[1] = hex_str[i * 2 + 1];
        woody->key[i] = (uint8_t)strtoul(byte_str, NULL, 16);
    }
    return (0);
}

/*
** Genera una clave pseudoaleatoria leyendo desde la máxima entropía del kernel
** de Linux (/dev/urandom). Requisito indispensable según el subject.
*/
void generate_random_key(t_woody *woody)
{
    int fd;

    woody->key_len = 16; // Cifrado a 128-bits
    fd = open("/dev/urandom", O_RDONLY);    // Intentamos abrir la fuente de entropía del sistema
    if (fd < 0)
    {
        // En caso remoto de fallar, fallback a rand() normal
        fprintf(stderr, "Advertencia: /dev/urandom inaccesible. Retrocediendo a rand().\n");
        srand(time(NULL));
        for (size_t i = 0; i < woody->key_len; i++)
            woody->key[i] = rand() % 256;
    }
    else
    {
        if (read(fd, woody->key, woody->key_len) != (ssize_t)woody->key_len)
        {
            fprintf(stderr, "Advertencia: Lectura de /dev/urandom fallida. Retrocediendo a rand().\n");
            srand(time(NULL));
            for (size_t i = 0; i < woody->key_len; i++)
                woody->key[i] = rand() % 256;
        }
        close(fd);
    }
}

/*
** Algoritmo de Cifrado Simétrico RC4 (Rivest Cipher 4)
** Muy profesional, rápido y clásico en desarrollo de Rootkits / Packers.
** Actúa como Stream Cipher (Cifrado de Flujo), por lo que no necesita padding (relleno).
*/
static void rc4_cipher(uint8_t *data, size_t data_len, uint8_t *key, size_t key_len)
{
    uint8_t S[256];
    uint8_t temp;
    size_t  i, j = 0;

    // Fase 1: Key-Scheduling Algorithm (KSA)
    for (i = 0; i < 256; i++)
        S[i] = i;

    for (i = 0; i < 256; i++)
    {
        j = (j + S[i] + key[i % key_len]) % 256;
        temp = S[i];
        S[i] = S[j];
        S[j] = temp;
    }

    // Fase 2: Pseudo-Random Generation Algorithm (PRGA) & Encriptación XOR
    i = 0;
    j = 0;
    for (size_t n = 0; n < data_len; n++)
    {
        i = (i + 1) % 256;
        j = (j + S[i]) % 256;
        
        temp = S[i];
        S[i] = S[j];
        S[j] = temp;
        
        // Hacemos el XOR directo sobre el buffer de memoria del archivo mapeado
        data[n] ^= S[(S[i] + S[j]) % 256];
    }
}

int encrypt_text_section(t_woody *woody)
{
    uint8_t *target_ptr;
    size_t  target_size;

    // Localizamos en nuestro buffer virtual de RAM dónde cae exactamente
    // el código compilado (sección .text) de la víctima.
    target_ptr = (uint8_t *)(woody->addr + woody->text_section->sh_offset);
    target_size = woody->text_section->sh_size;

    printf("Encriptando sección .text desde offset 0x%lx (%lu bytes) mediante RC4...\n", 
           woody->text_section->sh_offset, target_size);

    rc4_cipher(target_ptr, target_size, woody->key, woody->key_len);
    
    printf("Encriptación completada exitosamente.\n");
    return (0);
}
