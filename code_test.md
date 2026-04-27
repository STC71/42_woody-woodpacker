# 🧪 Motor de Tests: `test_auto.sh` y `test_eval.sh` explicado

<div align="center">
<a href="https://youtu.be/EkYJKyOAkek?si=JQTPl2yf1dy6xLXf">📺 Vídeo recomendado: Motores de Tests</a>
</div>

<br>

Este documento explica el funcionamiento interno y el uso práctico de los scripts de evaluación automática incluidos en el proyecto: `test_auto.sh` y `test_eval.sh`.

## 🧭 Objetivo

Describir qué hace cada script, cómo ejecutarlos, qué resultados producirán, y cómo interpretar sus salidas. Está orientado tanto a evaluadores (correción automática) como a desarrolladores que quieran ejecutar pruebas locales reproducibles.

## 📋 Requisitos previos

- Sistema Linux x86_64.
- Herramientas: `bash`, `make`, `valgrind`, `awk`, `grep`, `xxd` y utilidades estándar de GNU.
- Compilar el proyecto con `make` para disponer del ejecutable `woody_woodpacker` y los binarios de prueba.

## 🔍 Visión general de los scripts

- `test_auto.sh`: Batería de tests automáticos y no interactivos que valida comportamiento funcional, robustez y ausencia de fugas de memoria.
- `test_eval.sh`: Evaluador interactivo / semiautomático pensado para guía y debug, que muestra resultados detallados del test actual y permite pasos manuales.

---

## 1) `test_auto.sh` — Ejecución automática de la batería de pruebas

### ¿Qué hace?

`test_auto.sh` automatiza una secuencia extensa (casi treinta) de pruebas diseñadas para poner a prueba límites, errores y condiciones edge del empaquetador. Realiza:

- Compilación previa (si procede).
- Ejecución de casos con entradas válidas e inválidas.
- Tests de stress y límites (tamaños, corrupciones de secciones ELF, manipulación de headers).
- Verificaciones de salida funcional (comparación byte a byte con `diff`/hashes cuando aplica).
- Comprobación de fugas de memoria con `valgrind` y captura de errores de memoria.
- Búsqueda y extracción de líneas relevantes de código (con `awk`/`grep`) para mostrar al usuario la porción de código implicada.

### Uso

Ejecutar la batería completa:

```bash
./test_auto.sh
```

Opciones típicas (varían según versión del script):

- `-v` o `--verbose`: muestra salidas extendidas.
- `-k` o `--keep`: preserva binarios y logs temporales.
- `-h` o `--help`: muestra ayuda integrada.

### Salida esperada

- Un resumen final con el número de tests pasados/fallados.
- Logs individuales por test en `logs/` o `reports/` (dependiendo de la configuración del script).
- Informes de `valgrind` para los tests que lo requieren.

### Criterios de evaluación automáticos

- Código correcto — la funcionalidad solicitada cumple con el spec del enunciado.
- Robustez — el binario no debe crashear con inputs malformados.
- Memoria — cero fugas (o fugas aceptadas por la rúbrica) según `valgrind`.
- Output exacto cuando se especifica (salidas textuales o binarios esperados).

### Errores comunes y cómo interpretarlos

- `Segmentation fault`: fallo de acceso a memoria — revisar logs y el output de `gdb` si está habilitado.
- `valgrind` reports with DEFINITE LEAKS: hay fugas que deben corregirse.
- Fallos de comparación: la salida funcional no coincide — verificar cadenas, offsets y endianness.

### Recomendaciones para desarrolladores

- Ejecuta `./test_auto.sh -v` localmente antes de push.
- Si un test falla, abre el log correspondiente y reproduce solo ese caso con la línea de comando exacta que aparece en el log.

---

## 2) `test_eval.sh` — Evaluación guiada / interactiva

### ¿Qué hace?

`test_eval.sh` está pensado para evaluación manual o semiautomática: reproduce tests individuales, muestra la línea de código implicada y ofrece pasos guiados para inspección con herramientas (`valgrind`, `xxd`, `readelf`, etc.). Es ideal para debugging y para que un corrector pueda ver exactamente qué parte del código se está evaluando.

### Flujo típico

1. Ejecutar `./test_eval.sh`.
2. Seleccionar el test a inspeccionar (por índice o nombre).
3. El script compila si es necesario, ejecuta el caso elegido y presenta:
   - Salida estándar y errores.
   - Informe `valgrind` en bruto (si aplica).
   - Fragmento de código fuente y/o ensamblador relacionado (extraído con `awk`/`sed`).
4. Opciones para re-ejecutar con flags distintos o para conservar artefactos.

### Uso

```bash
./test_eval.sh
```

O, para evaluar un test concreto directamente:

```bash
./test_eval.sh <nombre_o_indice_del_test>
```

### Salida y artefactos

- Resultado del test en pantalla.
- Logs de ejecución y `valgrind` guardados en `eval_logs/`.
- Indicación clara de la porción de código evaluada con línea y fichero.

### Buenas prácticas para uso interactivo

- Si necesitas inspeccionar memoria, usa la opción de `valgrind --leak-check=full --show-leak-kinds=all`.
- Para comparar binarios usa `xxd -p` o `sha256sum`.
- Conserva los logs cuando pidas revisión externa.

---

## ✅ Checklist rápido antes de ejecutar los tests

- [ ] Compilar con `make` y comprobar que `woody_woodpacker` existe.
- [ ] Tener instaladas las utilidades (`valgrind`, `xxd`, `readelf`).
- [ ] Ejecutar `./test_auto.sh` en un entorno limpio para reproducibilidad.

---

## 🧾 Ejemplo práctico (mini-caso)

1. Compila:

```bash
make fclean && make
```

2. Ejecuta un test automático:

```bash
./test_auto.sh -v
```

3. Evalúa interactivamente el test 07:

```bash
./test_eval.sh 07
```

---

---

## 📊 Resultados de la ejecución y valoración final

### Resumen de métricas y valoración

> **¡Felicidades! Todos los criterios de la hoja de evaluación obligatorios y bonus se han cumplido al 125%.**

#### `test_auto.sh`

- Todos los tests automáticos se ejecutaron correctamente, cubriendo:
   - Compilación limpia y sin errores.
   - Infección y ejecución de binarios reales y edge cases.
   - Comparativas funcionales y verificación de integridad.
   - Pruebas de robustez ante archivos corruptos, permisos y límites del sistema.
   - Análisis de fugas de memoria con `valgrind` (sin fugas detectadas).
- **No se detectaron errores críticos ni fallos de seguridad.**

#### `test_eval.sh`

- La evaluación interactiva confirmó:
   - Cumplimiento estricto de la hoja de corrección oficial.
   - Soporte de cifrado RC4 fuerte, llaves aleatorias y paramétricas.
   - Generación y validación de binarios infectados sin diferencias funcionales.
   - Mensajes de éxito y logs claros en cada paso.
- **Valoración final:**
   - El sistema de tests y evaluación es robusto, reproducible y cumple todos los requisitos, incluyendo los bonus.

---
<br>

<div align="center">

[⬅️ Volver al README principal](./README.md)

</div>