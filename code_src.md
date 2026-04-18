# ⚙️ La Maquinaria Principal: Explorando la Carpeta `src`

Este documento explica de forma clara cómo interactúan los cuatro motores principales escritos en Lenguaje C que dictan toda la lógica del empaquetado de nuestro proyecto. 

Imagina nuestra carpeta `src/` como una línea de ensamblaje en una fábrica de coches.

## Flujo de Ejecución (Visual)

```text
        [main.c: Clonar archivo a Memoria RAM] 
                        |
                        v
        [elf_parser.c: Analizar cabeceras ELF]
                        |
                        v
             ¿Hay una 'Code Cave' suficiente?
                   /                    \
               NO /                      \ SÍ
                 v                        v
         [Abortar con Error]      [crypto.c: Generar Llave RC4]
                                           |
                                           v
                             [crypto.c: Cifrar la sección .text]
                                           |
                                           v
                 [injector.c: Inyectar payload.bin en la Code Cave]
                                           |
                                           v
               [injector.c: Parchear llave y offsets en el Payload]
                                           |
                                           v
             [injector.c: Redirigir el Entry Point original al Payload]
                                           |
                                           v
                 [main.c: Volcar RAM a disco duro nuevo 'woody']
```

---

## 🎬 1. El Director: `main.c`
Es el portero y el director de orquesta. Es la primera pieza de código que se ejecuta al lanzar el proyecto.
* **Control de acceso:** Su primer trabajo es revisar tus credenciales. Asegura que le hayas pasado un archivo válido, verifica si pesa lo suficiente, o si estás intentando inyectar una llave secreta.
* **El Quirófano (Mapeo):** En lugar de intentar modificar el programa víctima directamente en el disco duro, `main.c` clona todo el archivo y lo mete entero dentro de la Memoria RAM privada (`mmap`). Esto permite manipularlo quirúrgicamente sin dañarlo hasta estar 100% seguros de que todo el proceso ha salido bien.
* **Delegación:** Una vez el paciente está en la camilla de la RAM, manda llamar a los demás encargados en orden estricto (Parser -> Crypto -> Injector) y les entrega el expediente.

## 🔎 2. El Explorador y Analista: `elf_parser.c`
Todos los programas en Linux tienen un formato llamado ELF (Executable and Linkable Format). Son como un manual de IKEA; las distintas piezas del programa están organizadas en "Secciones" y "Segmentos".

El `elf_parser` es un analista forense y se dedica a leer ese manual pieza por pieza buscando tres cosas elementales:
1. **La sección de Código (`.text`):** El lugar exacto donde residen las instrucciones matemáticas del programa, es decir, lo que vamos a encriptar.
2. **El segmento de Carga (`PT_LOAD`):** La caja principal a la que este código pertenece, con tal de decirle al sistema operativo si es de lectura o escritura.
3. **El Escondite Mágico (Code Cave):** El analista busca inteligentemente un hueco sobrante y no utilizado por los programadores originales al final de sus piezas, lo suficientemente grande como para poder colar e inyectar ahí mismo nuestra carga viral de lenguaje ensamblador. 
> *Nota: Este analizador incluye sistemas avanzados (`is_safe_ptr`) para evitar que lo engañen archivos corruptos que intenten provocarle a propósito un fallo de memoria, protegiendo al empaquetador contra desbordamientos de enteros (Integer Overflows) y Underflows maliciosos al sumar offsets malintencionados. También verifica físicamente que cada byte de la Code Cave esté lleno de ceros (`0x00`) para no pisar firmas de compilador escondidas.*

## 🔒 3. El Cerrajero: `crypto.c`
Una vez el "Explorador" ubica el código vital de la aplicación original, el cerrajero se pone manos a la obra.
* Genera una llave de máxima seguridad y aleatoriedad nutriéndose de núcleos de ruido cuántico de la máquina (consultando a `/dev/urandom`). Si el usuario metió su propia llave, la revisa y asimila.
* Agarra y destruye visualmente todo el código principal ubicado por el `elf_parser.c`, revolviendo la información por completo usando matemáticas de cifrado (El algoritmo RC4).
* De este modo, cualquier Antivirus o ser humano que abra el archivo, en lugar de ver código claro y legible, solo verá ruido aleatorio carente de sentido.

## 💉 4. El Cirujano: `injector.c`
El trabajo final. Es el más delicado.
* El cirujano lee el virus en Ensamblador (`payload.bin`) y lo inserta limpiamente en el "Escondite Mágico" (la cueva) encontrado por el analista temporalmente en la RAM.
* **El Parcheo Neuronal:** Acto seguido, abre ligeramente la inyección, y le inyecta al propio virus la llave que acaba de generar el *Cerrajero*, y le da al virus las coordenadas de dónde está el código original para que sepa encontrarlo y descifrarlo en el futuro cuando el programa arranque por sí solo. Utilizando firmas de seguridad aleatorias de 64-bits (`0x1122334455667788`).
* **El Secuestro:** Modifica la entrada principal del archivo original. Cambia el letrero de "Entrada" que llevaba al programa hacia el código original natural, y lo redirige hacia nuestra cueva.
* **El Alta Médica y Sigilo W^X:** A diferencia de virus menos avanzados que modifican los permisos de ejecución del disco (`PF_W`) alertando a todos los antivirus, el cirujano vuelca todas las modificaciones quirúrgicas tal cual estaban, dejando que la criatura inyectada auto-modifique en RAM sus propios permisos al arrancar. Vuelca todo al disco duro, creando un nuevo ejecutable limpio (`woody`), con tamaño de sección alineado y preservando idénticamente los accesos del paciente original.

n## 🛡️ Extra: Estándares Posix e Ingeniería Sub-Cero
Para lograr un nivel 125% "Devil\'s Advocate" y construir un packer invencible:

* **Blindaje en capa I/O:** `injector.c` ya no confía en llamadas POSIX inocentes. Protege contra posibles *OOM (Out-of-Memory)* verificando `malloc`, e implementa un bucle `read` reintrant resistente a interrupciones del scheduler (`EINTR`) o *Short Reads* en payloads pesados.
* **Code Cave Scanning de Alta Heurística:** A la hora de rastrear cuevas, `elf_parser.c` no solo comprueba el padding inocente de bytes nulos (`0x00`), sino que incluye comprobaciones tolerantes a opcodes anti-desbordamientos inyectados por compiladores como Clang/GCC modernos en los límites de segmento (`0x90` NOPs y el trap de debugger `0xCC` INT3). De esta manera el analizador se acopla nativamente a cualquier técnica estricta de compilación de binarios.
¡Y el archivo queda empaquetado para siempre!
---

<div align="center">

[⬅️ Volver al README principal](./README.md)

</div>
