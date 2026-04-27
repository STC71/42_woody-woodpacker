# 🛠️ El Motor de Construcción: Makefile explicado

<div align="center">
<a href="https://youtu.be/mXNzvDLyC0g?si=iaIWjAJIAXLUt6WA">📺 Vídeo recomendado: Makefile, el motor de construcción</a>
</div>

<br>


Este documento desglosa y explica, paso a paso y con ejemplos, el funcionamiento del `Makefile` de Woody Woodpacker. Aquí aprenderás cómo se compila, ensambla y limpia el proyecto, así como los comandos útiles para testear y evaluar el empaquetador.

## 🧭 ¿Por qué un Makefile?
El Makefile es el corazón de la automatización en proyectos C/ASM. Permite compilar, limpiar, testear y reconstruir el proyecto con un solo comando, evitando errores manuales y ahorrando tiempo.

## 📦 ¿Qué es un Makefile?
Un `Makefile` es un archivo de instrucciones que automatiza la compilación y gestión de proyectos. Permite compilar, limpiar y ejecutar tests con simples comandos como `make all`, `make clean`, etc. Esencial para mantener el proyecto ordenado y reproducible.

## 🧩 Estructura y Variables Clave

- **Variables de configuración:**
   - `NAME`: Nombre del ejecutable final (`woody_woodpacker`).
   - `CC` y `CFLAGS`: Compilador y flags de compilación para C.
   - `NASM` y `NASMFLAGS`: Ensamblador y flags para el payload en ASM.
   - `SRC_DIR`, `OBJ_DIR`, `INC_DIR`, `ASM_DIR`: Rutas de carpetas fuente, objetos, includes y ensamblador.
   - `SRC`, `OBJ`, `STUB`: Listas de archivos fuente, objetos y el payload ensamblador generado.
- **Colores ANSI:**
   - Para mensajes de consola más claros y visuales (mejor feedback en terminal).

## 🔨 Reglas principales y su función

- `all`: Regla por defecto. Compila el payload en ensamblador y el binario principal en C, mostrando mensajes decorativos.
- `header`/`footer`: Imprimen banners visuales al inicio y fin de la compilación.
- `$(STUB)`: Ensambla el payload (`payload.s`) usando NASM y genera el binario parásito (`payload.bin`).
- `$(OBJ_DIR)/%.o`: Compila cada archivo fuente C a objeto, almacenando los .o en la carpeta obj/.
- `$(NAME)`: Enlaza todos los objetos y genera el ejecutable final (`woody_woodpacker`).
- `clean`: Elimina archivos temporales (.o) y el payload ensamblador.
- `fclean`: Ejecuta `clean` y además borra ejecutables y logs de test.
- `re`: Limpia todo y recompila desde cero.
- `test`: Compila y ejecuta la batería de tests automáticos (`test_auto.sh`).
- `eval`: Compila y ejecuta la evaluación interactiva (`test_eval.sh`).
- `help`: Muestra un menú de ayuda con todos los comandos disponibles y ejemplos de uso.

## 🚦 Ejemplo de flujo de trabajo

1. **Compilar todo el proyecto:**
  ```bash
  make
  ```
  Esto genera el payload, compila los .c y enlaza el ejecutable final.

2. **Limpiar archivos temporales:**
  ```bash
  make clean
  ```
  Elimina los archivos objeto y el payload ensamblador.

3. **Limpiar todo (incluyendo ejecutables):**
  ```bash
  make fclean
  ```
  Borra todo lo anterior más los ejecutables y logs.

4. **Recompilar desde cero:**
  ```bash
  make re
  ```
  Limpia y recompila todo el proyecto.

5. **Ejecutar tests automáticos:**
  ```bash
  make test
  ```
  Compila y lanza la batería de tests.

6. **Evaluación interactiva:**
  ```bash
  make eval
  ```
  Compila y ejecuta el script de evaluación guiada.

## 📚 Resumen de comandos útiles

- `make` o `make all`    → Compila y enlaza todo el proyecto.
- `make clean`           → Elimina archivos objeto y payload ensamblador.
- `make fclean`          → Elimina todo lo anterior + ejecutables y logs.
- `make re`              → Limpia y recompila desde cero.
- `make test`            → Ejecuta tests automáticos.
- `make eval`            → Evaluación guiada.
- `make help`            → Muestra menú de ayuda y ejemplos de uso.

## 📝 Consejos y buenas prácticas

- Antes de enviar tu proyecto, ejecuta `make fclean` para asegurarte de que todo se puede reconstruir desde cero.
- Usa `make help` o make a secas para recordar rápidamente los comandos disponibles.
- Si modificas archivos fuente o el payload, siempre ejecuta `make re` para evitar inconsistencias.
- Los mensajes de colores ayudan a identificar rápidamente errores, advertencias y pasos exitosos.

---

<br>

<div align="center">

[⬅️ Volver al README principal](./README.md)

</div>

