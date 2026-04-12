# 🧩 El Plano Arquitectónico: Desglosando `woody.h`

Este documento explica la utilidad de nuestro archivo de cabecera (Header), el `woody.h`. 

Para entenderlo fácilmente: Si el resto del código fueran los obreros y los ladrillos construyendo una casa, el archivo `.h` es el **Plano del Arquitecto**.

---

## 📜 1. Librerías Externas
Las primeras líneas del archivo son un menú de herramientas (los `#include`). Le estamos diciendo al compilador qué cajas de herramientas estándar vamos a necesitar de Linux:
* **Entrada y Salida:** Herramientas para imprimir cosas en pantalla (`stdio.h`).
* **Manipulación de Archivos:** Herramientas de POSIX para leer, abrir y tocar archivos en el disco duro (`fcntl.h`, `sys/stat.h`).
* **Magia con Memoria:** Herramientas extremadamente avanzadas para proyectar archivos enteros dentro de la Memoria RAM y modificarlos en el aire de forma virtual (`sys/mman.h`).
* **Estructuras ELF:** Herramientas para que el programa entienda las "piezas" que forman un archivo ejecutable en Linux (`elf.h`).

---

## 🚨 2. Respuestas a Errores
Verás una gran bloque con cosas como `#define ERR_OPEN "Error: No se pudo abrir el archivo"`. 
Esto es el protocolo de emergencias. En lugar de escribir a mano qué decir cada vez que ocurre un fallo, dejamos estandarizadas aquí todas las respuestas posibles. Si el usuario intenta hackear a un archivo que es un texto y no un ejecutable o si no tiene permisos, tiraremos de este manual.

---

## 🧬 3. La Estructura Maestra: `t_woody`
Esta es la parte más importante. En el lenguaje C, una `struct` es como la ficha médica o el expediente de un paciente. A este expediente lo llamamos `t_woody`, y viajará a través de todas las fases del código llevando la información del archivo que estamos infectando.

¿Qué datos anota el empaquetador en esta ficha?

| Campo / Variable | Explicación para humanos |
| :--- | :--- |
| `*addr` y `size` | ¿Dónde está colocado temporalmente el archivo en el quirófano (memoria RAM) y cuánto espacio ocupa? |
| `file_mode` | Guardamos celosamente qué permisos tenía el archivo para devolvérselos idénticos cuando termine el proceso. |
| `*ehdr` | Una radiografía general de qué tipo de archivo es. |
| `target_segment` | Localizamos en qué parte física del archivo vive la lógica real del programa para poder ir a "dormirla". |
| `text_section` | La zona milimétrica del código que será atacada y encriptada por el candado RC4. |
| `cave_offset` / `size`| *"La Cueva"*. Un espacio vacío que el sistema operativo deja entre módulos del programa sin utilizar. Aquí es donde esconderemos a nuestro polizón (el Payload de Ensamblador). |
| `key` y `key_len` | La contraseña de 128-bits aleatoria (o introducida por el usuario) empleada para encerrar el código. |

<br>
Sabiendo esto, cada vez que una función del código C pida el expediente `t_woody`, ¡sabrás que le estamos entregando absolutamente todas las llaves necesarias para operar al paciente!