# 🧠 El Cerebro Inyectado: Entendiendo nuestro Payload en Ensamblador

¡Bienvenido al corazón del proyecto! Este documento explica de forma sencilla qué hace el código escrito en lenguaje **Ensamblador (ASM)** dentro de la carpeta `asm/`.

---

## 🦠 ¿Qué es el "Payload"?
Imagina que el programa original es un tren de pasajeros. Nuestro "Payload" (carga útil) es un pasajero polizón que introducimos a escondidas en uno de los vagones vacíos. Cuando el tren empieza a moverse, este polizón toma el control temporalmente, hace su trabajo, y luego devuelve el control al conductor original para que nadie sospeche nada.

Como este polizón se inyecta literalmente en las venas del programa víctima, debe estar escrito en el lenguaje más básico y cercano al procesador posible: **Ensamblador**.

---

## 🛠️ Fases de Actuación del Polizón

Nuestro archivo `payload.s` ejecuta una coreografía estrictamente calculada:

### 1. Guardar el Estado (El modo Sigilo)
Antes de tocar nada, el polizón guarda exactamente cómo estaban la memoria y los registros del procesador de la víctima. 
* **¿Por qué?** Si movemos los mandos de la cabina y no los dejamos como estaban, el programa original se dará cuenta y se estrellará (generando el famoso *Segmentation Fault*). Usamos comandos como `push` para hacer una "copia de seguridad" de todo.

### 2. Dejar nuestra Firma
El programa imprime el clásico mensaje en pantalla: `"....WOODY...."`. Para ello, invoca directamente a las funciones internas del sistema operativo de Linux (llamadas *Syscalls*).

### 3. Encontrarse a sí mismo en el espacio (PIE)
Los programas modernos no cargan siempre en la misma dirección de memoria por seguridad. Nuestro polizón es ciego; no sabe dónde ha sido inyectado. Por tanto, usa un truco técnico para calcular **su propia posición relativa**. ¡Así evita perderse en la inmensidad de la memoria RAM!

### 4. Recibir el "Golpe Maestro" del inyector en C
El archivo en ensamblador tiene preparadas unas "Variables Huecas" o una **Firma Mágica**. Cuando el empaquetador en C inserta a este pasajero en el tren, sustituye esos huecos rellenándolos con:
* La dirección exacta de la cápsula encriptada.
* El tamaño de dicha cápsula.
* La **llave secreta** que necesita para abrirla.
* Las coordenadas de la cabina de control original.

### 5. Desencriptar la lógica original (Misión Principal)
El código de la aplicación original está protegido por un complejo candado criptográfico (RC4). Nuestro polizón extrae su llave y, a la velocidad del rayo, empieza a descifrar las instrucciones originales del programa directamente sobre la memoria RAM (este proceso se conoce como PRGA, *Pseudo-Random Generation Algorithm*).

### 6. Borrar sus huellas
Una vez descifrado el programa original, el polizón restaura toda la "copia de seguridad" de los mandos de la cabina que hizo en el Paso 1 (`pop` de los registros).

### 7. El Salto Incondicional (Retorno a la normalidad)
El último paso de nuestro código es un salto comando `jmp` (Jump) dictándole a la computadora: *"Ve al Punto de Entrada Original (OEP) del programa"*. El tren de pasajeros continúa su recorrido de forma natural, sin que los usuarios noten absolutamente nada anormal, salvo el mensaje en la consola.
