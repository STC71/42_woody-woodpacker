# ⚙️ La Maquinaria Principal: Explorando la Carpeta `src`

Este documento explica de forma clara cómo interactúan los cuatro motores principales escritos en Lenguaje C que dictan toda la lógica del empaquetado de nuestro proyecto. 

Imagina nuestra carpeta `src/` como una línea de ensamblaje en una fábrica de coches.

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
> *Nota: Este analizador incluye sistemas avanzados (`is_safe_ptr`) para evitar que lo engañen archivos corruptos que intenten provocarle a propósito un fallo de memoria.*

## 🔒 3. El Cerrajero: `crypto.c`
Una vez el "Explorador" ubica el código vital de la aplicación original, el cerrajero se pone manos a la obra.
* Genera una llave de máxima seguridad y aleatoriedad nutriéndose de núcleos de ruido cuántico de la máquina (consultando a `/dev/urandom`). Si el usuario metió su propia llave, la revisa y asimila.
* Agarra y destruye visualmente todo el código principal ubicado por el `elf_parser.c`, revolviendo la información por completo usando matemáticas de cifrado (El algoritmo RC4).
* De este modo, cualquier Antivirus o ser humano que abra el archivo, en lugar de ver código claro y legible, solo verá ruido aleatorio carente de sentido.

## 💉 4. El Cirujano: `injector.c`
El trabajo final. Es el más delicado.
* El cirujano lee el virus en Ensamblador (`payload.bin`) y lo inserta limpiamente en el "Escondite Mágico" (la cueva) encontrado por el analista temporalmente en la RAM.
* **El Parcheo Neuronal:** Acto seguido, abre ligeramente la inyección, y le inyecta al propio virus la llave que acaba de generar el *Cerrajero*, y le da al virus las coordenadas de dónde está el código original para que sepa encontrarlo y descifrarlo en el futuro cuando el programa arranque por sí solo.
* **El Secuestro:** Modifica la entrada principal del archivo original. Cambia el letrero de "Entrada" que llevaba al programa hacia el código original natural, y lo redirige hacia nuestra cueva.
* **El Alta Médica:** Vuelca todas las modificaciones quirúrgicas que hemos estado operando sobre la Memoria RAM de nuevo dentro de un sólido disco duro, creando un nuevo ejecutable clonado final llamado `woody`, preservando idénticamente los permisos y accesos del paciente original.

¡Y el archivo queda empaquetado para siempre!
---

<div align="center">

[⬅️ Volver al README principal](./README.md)

</div>
