# Esteganografia-por-Enmascaramiento-con-Eco
Proyecto 1 - Curso procesamiento digital de señales

## Integrantes
- Nasser Brown
- David Duarte
- Jose Eduardo Zuniga Ramirez

## Entrega

- **Fecha límite:** 22 de octubre de 2026, 23:59.
- Repositorio GitHub/GitLab con acceso para el profesor (usuario `michaelgruner`).
- Respaldo ZIP/TAR del repositorio en TEC Digital.
- El repositorio debe incluir: artículo final en PDF, todo el código fuente, este `README.md` con instrucciones de instalación/construcción/ejecución, archivos de prueba (o instrucciones para obtenerlos) y un `Makefile`.
- El historial de Git debe reflejar el desarrollo del equipo (R5 vale 2.5 %): hacer commits pequeños y frecuentes, de ambos integrantes.

## Resumen del sistema

Basado en Gruhl, Lu y Bender, *Echo Hiding* (`Echo_hiding.pdf`). Cada bit de metadatos se esconde en una ventana del audio añadiéndole un eco:

```
H_b(z) = 1 + a_b · z^(-t_b)        h_b(n) = δ(n) + a_b · δ(n - t_b)
```

**Codificador:** `WAV → x(n) → ventanas v_k(n) → (bit b) → v_k * h_b → solapamiento y suma → WAV`

**Decodificador:** `WAV → x(n) → ventanas v_k(n) → R_cc → clasificar {0, 1, sin eco} → bits → bytes → metadatos`

Autocorrelación del cepstro, evaluada en el dominio espectral (R45):

```
R_cc(n) = IFFT{ ( ln|FFT{v_k(n)}| )^2 }
```

Antes de la FFT, cada ventana se escala a un nivel fijo (valor eficaz 2^15) y se multiplica por una ventana de Hann. El clasificador compara `R_cc[t0]` y `R_cc[t1]` contra el nivel típico de `R_cc` en los retardos `[t_max/2 + 1, 3·t_max)`, usando un z-score robusto (mediana y mediana de las desviaciones absolutas). El bit es el retardo con mayor `z`, y si ninguno supera `θ = 3` la ventana se marca como "sin eco" (R47).

## Solución implementada

### Prototipo en Python (`python/echo_hiding_poc.ipynb`)
1. Lee y escribe WAV de 44.1 kHz, 16 bits, estéreo con `scipy.io.wavfile` y trabaja sobre un canal.
2. Codificador: enventanado, convolución de cada ventana con `h0` o `h1` y combinación por **solapamiento y suma** (R36): la cola de `N-1 = t_b` muestras se suma a la ventana siguiente. El cuaderno verifica que el resultado es idéntico a convolucionar la señal completa.
3. Decodificador: `R_cc` por FFT y clasificador con umbral, como se describe arriba.
4. Protocolo de datos (diseño libre, sección 4.3.3 del enunciado):
   - El mensaje es texto UTF-8 con una línea `clave=valor` por metadato, y cada byte se transmite empezando por el bit más significativo.
   - Las ventanas posteriores al mensaje se dejan sin eco. El fin del mensaje es la primera racha de `GAP = 8` ventanas consecutivas sin eco, por lo que deben quedar al menos 8 ventanas libres.
   - Las claves conocidas se imprimen en `stdout` con su categoría (R48, R49) y las desconocidas o corruptas en `stderr` con una advertencia (R50, R51).
5. La última celda evalúa todas las pistas de `audio/` en una sola ejecución y guarda las figuras del artículo en `Reporte/images/todas/`.

### Caracterización (Python)
- Prueba de extremo a extremo con los parámetros por defecto en 6 pistas de 30 s: sintética, rock, piano moderno, piano clásico, electrónica y voz (A32).
- Barrido de amplitud (5–50 %) y de tamaño de ventana (2–32 `t_max`, 20.9–334 bit/s), con 3 repeticiones por punto. Se mide:
  - **Tasa de recuperación** (bits correctos / bits codificados). Con los valores por defecto fue de 96.4 a 100 % (R46), con PESQ entre 4.29 y 4.52.
  - **PESQ** (paquete `pesq`, en `environment.yml`). Se remuestrea a 16 kHz y se usa el modo de banda ancha. PESQ está pensado para voz y no pudo evaluar la pista de rock, limitación que se discute en el artículo.
  - **Tasa de transferencia** (bit/s = `fs / L`) y **SNR** como análisis adicional.
- Correlaciones de Pearson entre recuperación, tasa de transferencia y PESQ (A30, A31).

### Implementación en C++ (`cpp/`)
- `cpp/src/main.cpp` une el CLI, el WAV, el codificador y el decodificador, con el mismo protocolo que el prototipo.
- Lectura y escritura de WAV propias (`cpp/include/encoder/wav.hpp`). Se rechaza todo lo que no sea 44.1 kHz, PCM de 16 bits y estéreo (R25). El archivo de salida conserva el encabezado de 44 bytes (R37) y el canal no codificado queda intacto.
- Convolución explícita ventana × `h_b` y solapamiento y suma (`cpp/include/encoder/encoder.hpp`, R34, R36). Las ventanas se cruzan con rampas sin²/cos² de una ventana completa que suman 1, así que el cambio de núcleo es suave (R32). La salida se satura a int16.
- `R_cc` con una FFT propia (radix-2 y Bluestein para longitudes que no son potencia de 2, `cpp/include/deco/`). Una ventana en silencio digital devuelve `R_cc` nula para coincidir con el prototipo.
- CLI con `getopt_long` (`cpp/src/cli.cpp`): aborta ante opciones desconocidas (R7) y se queda con el último valor si una opción se repite (R9).
- Los mensajes de diagnóstico van a `stderr`, de modo que `stdout` contiene solo los metadatos.
- Validación contra el prototipo: en las 6 pistas, el WAV codificado por C++ es idéntico muestra por muestra al de Python, y la salida del decodificador (`stdout` y advertencias) es la misma.
- Pruebas en `cpp/tests/`: `encoder_test.cpp` (solapamiento y suma frente a convolución completa, y codificación con decodificación), `deco_test.cpp` y `cli_test.cpp`.

### Tamaño de ventana (R33, R42)
Codificador y decodificador usan la misma regla, `L = 10 · t_max` (660 muestras, 15 ms y 66.8 bit/s con los valores por defecto). El cepstro real es simétrico y periódico en `L` y el clasificador toma su referencia hasta `3·t_max`, lo que exige `L ≥ 6·t_max`. Dentro de ese límite, 10 es el menor múltiplo que cumple PESQ ≥ 4.0 (R32) y recuperación ≥ 95 % (R46) en todas las pistas de prueba; con 9 PESQ baja de 4.0 (barrido en el artículo).

### Interfaz (CLI)
```
cpp/build/echo_hiding [-h] [metadatos] [enmascaramiento] [--channel {left,right}] ENTRADA.wav [SALIDA.wav]

Metadatos (si hay al menos uno → modo codificación, si no → decodificación):
  --title TEXTO   --artist TEXTO   --author TEXTO   --album TEXTO
  --year TEXTO    --guest TEXTO    --producer TEXTO

Enmascaramiento (deben coincidir al codificar y decodificar):
  --delay0 US     retardo del eco para un bit 0 [µs] (default: 1000)
  --delay1 US     retardo del eco para un bit 1 [µs] (default: 1500)
  --att0 PCT      amplitud del eco para un bit 0 [% de la original] (default: 40)
  --att1 PCT      amplitud del eco para un bit 1 [% de la original] (default: 40)
  --channel left|right   canal a codificar/decodificar (default: left)
```

### Estructura del repositorio
```
audio/        pistas de prueba (MP3 originales y WAV de 30 s) e instrucciones de conversión
cpp/
  include/    cli/, deco/ (R_cc, clasificador, FFT) y encoder/ (codificador y WAV)
  src/        cli.cpp y main.cpp (aplicación)
  tests/      pruebas del codificador, del decodificador y del CLI
  build/      binario cpp/build/echo_hiding (generado por make)
python/       prototipo, barridos de parámetros, PESQ y gráficas
Reporte/      fuente LaTeX (IEEEtran), referencias, figuras y PDF del artículo
Makefile      objetivos all, env y clean
environment.yml
```

### Cronograma tentativo
| Fechas        | Actividad                                                        |
|---------------|------------------------------------------------------------------|
| 11–13 oct     | Prototipo Python: codificador, decodificador y protocolo         |
| 13–16 oct     | Barridos, PESQ, géneros musicales y elección de valores por defecto |
| 14–18 oct     | Implementación en C/C++, `Makefile` y CLI                         |
| 18–20 oct     | Validación de C++ contra Python y resultados finales             |
| 16–21 oct     | Redacción del artículo (en paralelo)                             |
| 22 oct        | Revisión final, PDF, ZIP a TEC Digital                            |

---

## Checklist

Entre paréntesis va el peso en la nota cuando el enunciado lo indica.

### Generales y entrega
- [x] R1 Prototipo en Python e implementación final en C/C++ con herramientas libres
- [x] R2 Todo el código fuente de autoría propia en el repositorio
- [x] R3 Funciona en GNU/Linux
- [x] R4 `Makefile` que construye la aplicación
- [x] R5 Historial de Git que demuestre el desarrollo (2.5)
- [x] Acceso al repositorio para `michaelgruner`
- [x] README con instrucciones de instalación, construcción y ejecución
- [x] Archivos de prueba o instrucciones para obtenerlos
- [x] Artículo final en PDF dentro del repositorio
- [x] ZIP/TAR subido a TEC Digital antes del 22/10/2026 23:59

### Parámetros
- [x] R6 Parámetros por línea de comandos
- [x] R7 Abortar ante parámetros desconocidos (0.5)

### Metadatos (12 %)
- [x] R8 Parámetros de metadatos opcionales (0.5)
- [x] R9 Ante duplicados, se usa el último (1)
- [x] R10 Título de la canción (1.5)
- [x] R11 Artista (1.5)
- [x] R12 Autor original (1.5)
- [x] R13 Álbum (1.5)
- [x] R14 Año de publicación (1.5)
- [x] R15 Artista invitado (1.5)
- [x] R16 Productor (1.5)

### Enmascaramiento (5 %)
- [x] R17 Valores por defecto si no se especifican
- [x] R18 Valores por defecto = los de mejor desempeño medido
- [x] R19 Retardos en microsegundos
- [x] R20 Parámetro de retardo para el bit 0
- [x] R21 Parámetro de retardo para el bit 1
- [x] R22 Atenuaciones en porcentaje
- [x] R23 Parámetro de atenuación para el bit 0
- [x] R24 Parámetro de atenuación para el bit 1

### Formato de audio (5 %)
- [x] R25 WAV de 44.1 kHz, 16 bits little endian, estéreo (4)
- [x] R26 Argumento para elegir el canal (0.5)
- [x] R27 Canal izquierdo por defecto (0.5)

### Codificador (20 %)
- [x] R28 Modo codificación si hay al menos un metadato (0.5)
- [x] R29 Primer argumento posicional = WAV original (0.5)
- [x] R30 Segundo argumento posicional = WAV destino (0.5)
- [x] R31 Termina cuando se codifica todo el archivo (0.5)
- [x] R32 Con los valores por defecto, la salida es perceptualmente indistinguible (2.5)
- [x] R33 Ventana de menor tamaño posible según el mayor retardo (0.5)
- [x] R34 Codificación por convolución con `h0`/`h1` (7.5)
- [x] R35 / R36 Combinación por solapamiento y almacenamiento **o** solapamiento y suma (solo una) (7.5)
- [x] R37 Encabezado WAV equivalente al original
- [x] R38 Se reproduce en reproductores estándar (VLC, MPlayer, etc.)

### Decodificador (20 %)
- [x] R39 Modo decodificación si no hay metadatos (0.5)
- [x] R40 Primer argumento posicional = WAV codificado (0.5)
- [x] R41 Termina cuando se decodifica todo el archivo (0.5)
- [x] R42 Misma regla de tamaño de ventana que el codificador (0.5)
- [x] R43 Clasifica cada ventana como 1 o 0 (4)
- [x] R44 Clasifica con la autocorrelación del cepstro (7.5)
- [x] R45 `R_cc` evaluada en el dominio espectral, no como autocorrelación temporal
- [x] R46 Tasa de recuperación ≥ 90 % con los valores por defecto
- [x] R47 Reconoce las ventanas no codificadas (3.5)
- [x] R48 Metadatos encontrados en `stdout` (1)
- [x] R49 Cada metadato con su categoría (0.5)
- [x] R50 Metadatos no reconocidos en `stderr` (1)
- [x] R51 Metadatos desconocidos con advertencia (0.5)

### Artículo científico (35 %)
- [x] A1 Escrito en LaTeX
- [x] A2 Formato IEEE Transactions (plantilla `IEEEtran`)
- [x] A3 Máximo 4 páginas

**Forma (5 %)**
- [x] A4 Solo palabras en español
- [x] A5 Siglas definidas antes de usarse
- [x] A6 Tablas, figuras y ecuaciones introducidas antes de aparecer
- [x] A7 Nombres de tablas, figuras, etc. con mayúscula (por ejemplo, "Tabla 1")
- [x] A8 Título debajo en todo excepto las tablas
- [x] A9 Título de las tablas encima
- [x] A10 Tablas sin divisiones verticales
- [x] A11 Líneas horizontales solo al inicio, al final y bajo el encabezado (`booktabs`)
- [x] A12 Sin código textual: usar diagramas
- [x] A13 Palabras clave en orden alfabético
- [x] A14 Resumen (*abstract*) en inglés

**Contenido (16 %)**
- [x] A15 El resumen permite decidir si el artículo es útil (1)
- [x] A16 Palabras clave representativas (0.5)
- [x] A17 Introducción con contexto, trabajos relacionados y estado del arte (2)
- [x] A18 Introducción termina con la estructura del documento (0.5)
- [x] A19 Teoría necesaria: LTI, convolución, OLA/OLS, cepstro y `R_cc` (4)
- [x] A20 Cada módulo descrito con detalle suficiente para reproducirlo (4)
- [x] A21 Respuestas ante distintas configuraciones y escenarios
- [x] A22 Resultados relacionados con la teoría
- [x] A23 Resultados por cada componente del sistema
- [x] A24 Conclusiones solo sobre resultados presentados (0.5)
- [x] A25 Una conclusión por cada resultado analizado (2)
- [x] A26 Recomendaciones para trabajo futuro (0.5)
- [x] A27 Citas bibliográficas (1)

**Análisis (14 %)**
- [x] A28 PESQ ante diferentes configuraciones (3)
- [x] A29 Tasa de recuperación ante diferentes configuraciones (3)
- [x] A30 Correlación entre tasa de recuperación y calidad perceptual (2)
- [x] A31 Correlación entre tasa de transferencia y calidad perceptual (2)
- [x] A32 Pruebas con distintos géneros musicales (2)
- [x] A33 Análisis adicionales propios, por ejemplo canal derecho contra izquierdo, silencios o recorte (*clipping*) (2)

### Trabajo opcional (extra)
- [ ] O1 Codificar la imagen de portada del disco (5)
- [ ] O2 Sistema de corrección de errores, por ejemplo Hamming o repetición (5)
- [ ] O3 Ejecución en línea con parlante y micrófono (5)

---

## Instalación, construcción y ejecución

### Requisitos

- GNU/Linux con `make` y un compilador de C++17.
- Opcional: conda o mamba. Si está instalado, `make` crea el ambiente `pds-proyecto1` desde
  `environment.yml` y compila con su g++; si no, compila con el `g++` del sistema.
- Para el prototipo en Python: el mismo ambiente (`conda activate pds-proyecto1`), que incluye
  numpy, scipy, matplotlib, pesq y Jupyter.

### Construcción

```bash
make          # genera cpp/build/echo_hiding
make clean    # borra cpp/build/
```

### Ejecución

Con al menos un metadato el programa codifica; sin metadatos, decodifica. Los parámetros de
enmascaramiento y el canal deben ser los mismos al codificar y al decodificar.

```bash
# Codificar: ENTRADA.wav SALIDA.wav + metadatos
cpp/build/echo_hiding audio/rock_indie.wav codificado.wav --title "Mi canción" --artist "Banda" --year 2026

# Decodificar
cpp/build/echo_hiding codificado.wav

# Canal derecho y parámetros propios
cpp/build/echo_hiding audio/rock_indie.wav codificado.wav --channel right --delay0 1000 --delay1 1500 \
    --att0 40 --att1 40 --title "Mi canción"
cpp/build/echo_hiding codificado.wav --channel right --delay0 1000 --delay1 1500

# Ayuda con todas las opciones
cpp/build/echo_hiding -h
```

Al decodificar, la salida estándar contiene solo los metadatos reconocidos:

```
Artista: Banda
Título: Mi canción
Año: 2026
```

Los diagnósticos (modo, ventana, porcentaje de ventanas con eco) y las advertencias por metadatos
desconocidos van a la salida de error.

### Prototipo y figuras del artículo

```bash
cd python
jupyter nbconvert --to notebook --execute --inplace echo_hiding_poc.ipynb
```

El cuaderno corre el prototipo, la prueba de extremo a extremo y los barridos, y regenera las
figuras de `Reporte/images/todas/`. Los archivos de prueba están en `audio/` (ver
`audio/README.md`).
