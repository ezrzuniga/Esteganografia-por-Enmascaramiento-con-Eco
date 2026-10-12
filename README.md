# Esteganografia-por-Enmascaramiento-con-Eco
Proyecto 1 - Curso procesamiento digital de señales

## Integrantes
- David Duarte
- Jose Eduardo Zuniga Ramirez

## Entrega

- **Fecha límite:** 22 de octubre de 2026, 23:59.
- Repositorio GitHub/GitLab con acceso para el profesor (usuario `michaelgruner`).
- Respaldo ZIP/TAR del repositorio en TEC Digital.
- El repositorio debe incluir: artículo final en PDF, todo el código fuente, este `README.md` con instrucciones de instalación/construcción/ejecución, archivos de prueba (o instrucciones para obtenerlos) y un `Makefile`.
- El historial de Git debe reflejar el desarrollo del equipo (R5 vale 2.5 %): hacer commits pequeños y frecuentes, de ambos integrantes.

## Resumen del sistema

Basado en Gruhl y Bender, *Echo Hiding* (`Echo_hiding.pdf`). Cada bit de metadatos se esconde en una ventana del audio añadiéndole un eco:

```
H_b(z) = 1 + a_b · z^(-t_b)        h_b(n) = δ(n) + a_b · δ(n - t_b)
```

**Codificador:** `WAV → x(n) → ventanas v_k(n) → (bit b) → v_k * h_b → overlap-add/save → WAV`

**Decodificador:** `WAV → x(n) → ventanas v_k(n) → R_cc → clasificar {0, 1, sin eco} → bits → bytes → metadatos`

Autocorrelación del cepstro, evaluada en el dominio espectral (R45):

```
R_cc(n) = IFFT{ ( ln|FFT{v_k(n)}| )^2 }
```

El clasificador compara `R_cc[t0]` contra `R_cc[t1]` y, si ninguno sobresale del piso de ruido, marca la ventana como "sin eco" (R47).

## Plan de solución

### Fase 1: Prototipo en Python
1. Leer y escribir WAV de 44.1 kHz, 16 bits, estéreo (`scipy.io.wavfile` o `soundfile`) y seleccionar el canal.
2. Codificador: enventanado, convolución de cada ventana con `h0` o `h1`, y combinación por **solapamiento y suma** (es la más natural cuando se convoluciona por ventanas: la cola de `N-1 = t_max` muestras se suma a la ventana siguiente). R35 y R36 son excluyentes: implementar solo una.
3. Decodificador: `R_cc` por FFT, clasificador con umbral para detectar ventanas sin eco.
4. Protocolo de datos (diseño libre, sección 4.3.3). Propuesta TLV:
   - `[tag: 1 byte][largo: 1 byte][payload UTF-8]` por cada metadato, y `tag = 0x00` como fin de mensaje.
   - Tags: 1 = título, 2 = artista, 3 = autor, 4 = álbum, 5 = año, 6 = artista invitado, 7 = productor.
   - Un tag desconocido o un payload corrupto se imprime en `stderr` con advertencia (R50, R51).
   - Opcional: un byte de sincronía o preámbulo y un checksum/CRC, que sirve como base para O2.
5. Las ventanas posteriores al fin del mensaje se dejan sin eco (o el mensaje se repite cíclicamente para ganar robustez; hay que decidirlo y justificarlo).

### Fase 2: Caracterización (Python)
- Barrido de `t0`, `t1` (µs) y `a0`, `a1` (%), midiendo:
  - **Tasa de recuperación** (bits correctos / bits codificados). La meta es ≥ 90 % con los valores por defecto (R46).
  - **PESQ** (paquete `pesq`). PESQ está pensado para voz a 8/16 kHz, así que se remuestrea a 16 kHz mono y esta limitación se discute en el artículo.
  - **Tasa de transferencia** (bits/s = fs / M).
- Probar varios géneros musicales (A32).
- Elegir los valores por defecto con el mejor compromiso (R17, R18) y verificar que el audio sea imperceptible (R32).

### Fase 3: Implementación en C/C++
- Lectura/escritura de WAV propia (parsear el encabezado RIFF y copiarlo tal cual, R37) o con `libsndfile`.
- FFT con `FFTW3` (libre) para `R_cc`.
- Convolución explícita ventana × `h_b` (R34).
- Saturar a int16 después de sumar el eco, para evitar desbordes.
- CLI con `getopt_long`: abortar ante opciones desconocidas (R7) y quedarse con el último valor si una opción se repite (R9).
- `Makefile` con objetivos `all`, `clean` y `test`.
- Validar que la salida en C++ coincida con el prototipo en Python.

### Decisión clave: tamaño de ventana (R33, R42)
La ventana debe ser "la menor posible según el mayor retardo". El cepstro real es simétrico y periódico en `M`, así que para resolver el pico en `t_max` se necesita `t_max < M/2`. Hay que definir la regla exacta (por ejemplo `M = 2·t_max + margen`, o la siguiente potencia de 2 para la FFT), justificarla con ecuaciones en el artículo y usar **la misma regla** en el codificador y el decodificador.

### Interfaz propuesta (CLI)
```
echohide [opciones] <entrada.wav> [salida.wav]

Metadatos (si hay al menos uno → modo codificación, si no → decodificación):
  --title <txt>   --artist <txt>   --author <txt>   --album <txt>
  --year <n>      --featured <txt> --producer <txt>

Enmascaramiento (con valores por defecto):
  --d0 <µs>  --d1 <µs>      retardos para bit 0 / bit 1
  --a0 <%>   --a1 <%>       atenuaciones para bit 0 / bit 1
  --channel left|right      canal a codificar/decodificar (por defecto: left)
```

### Estructura de repositorio propuesta
```
python/      prototipo, barridos de parámetros y scripts de PESQ y gráficas
src/         implementación en C/C++
include/
tests/       audios de prueba o script para descargarlos
articulo/    fuente LaTeX (IEEEtran) y PDF final
Makefile
requirements.txt
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
- [ ] R1 Prototipo en Python e implementación final en C/C++ con herramientas libres
- [ ] R2 Todo el código fuente de autoría propia en el repositorio
- [ ] R3 Funciona en GNU/Linux
- [ ] R4 `Makefile` que construye la aplicación
- [ ] R5 Historial de Git que demuestre el desarrollo (2.5)
- [ ] Acceso al repositorio para `michaelgruner`
- [ ] README con instrucciones de instalación, construcción y ejecución
- [ ] Archivos de prueba o instrucciones para obtenerlos
- [ ] Artículo final en PDF dentro del repositorio
- [ ] ZIP/TAR subido a TEC Digital antes del 22/10/2026 23:59

### Parámetros
- [ ] R6 Parámetros por línea de comandos
- [ ] R7 Abortar ante parámetros desconocidos (0.5)

### Metadatos (12 %)
- [ ] R8 Parámetros de metadatos opcionales (0.5)
- [ ] R9 Ante duplicados, se usa el último (1)
- [ ] R10 Título de la canción (1.5)
- [ ] R11 Artista (1.5)
- [ ] R12 Autor original (1.5)
- [ ] R13 Álbum (1.5)
- [ ] R14 Año de publicación (1.5)
- [ ] R15 Artista invitado (1.5)
- [ ] R16 Productor (1.5)

### Enmascaramiento (5 %)
- [ ] R17 Valores por defecto si no se especifican
- [ ] R18 Valores por defecto = los de mejor desempeño medido
- [ ] R19 Retardos en microsegundos
- [ ] R20 Parámetro de retardo para el bit 0
- [ ] R21 Parámetro de retardo para el bit 1
- [ ] R22 Atenuaciones en porcentaje
- [ ] R23 Parámetro de atenuación para el bit 0
- [ ] R24 Parámetro de atenuación para el bit 1

### Formato de audio (5 %)
- [ ] R25 WAV de 44.1 kHz, 16 bits little endian, estéreo (4)
- [ ] R26 Argumento para elegir el canal (0.5)
- [ ] R27 Canal izquierdo por defecto (0.5)

### Codificador (20 %)
- [ ] R28 Modo codificación si hay al menos un metadato (0.5)
- [ ] R29 Primer argumento posicional = WAV original (0.5)
- [ ] R30 Segundo argumento posicional = WAV destino (0.5)
- [ ] R31 Termina cuando se codifica todo el archivo (0.5)
- [ ] R32 Con los valores por defecto, la salida es perceptualmente indistinguible (2.5)
- [ ] R33 Ventana de menor tamaño posible según el mayor retardo (0.5)
- [ ] R34 Codificación por convolución con `h0`/`h1` (7.5)
- [ ] R35 / R36 Combinación por solapamiento y almacenamiento **o** solapamiento y suma (solo una) (7.5)
- [ ] R37 Encabezado WAV equivalente al original
- [ ] R38 Se reproduce en reproductores estándar (VLC, MPlayer, etc.)

### Decodificador (20 %)
- [ ] R39 Modo decodificación si no hay metadatos (0.5)
- [ ] R40 Primer argumento posicional = WAV codificado (0.5)
- [ ] R41 Termina cuando se decodifica todo el archivo (0.5)
- [ ] R42 Misma regla de tamaño de ventana que el codificador (0.5)
- [ ] R43 Clasifica cada ventana como 1 o 0 (4)
- [ ] R44 Clasifica con la autocorrelación del cepstro (7.5)
- [ ] R45 `R_cc` evaluada en el dominio espectral, no como autocorrelación temporal
- [ ] R46 Tasa de recuperación ≥ 90 % con los valores por defecto
- [ ] R47 Reconoce las ventanas no codificadas (3.5)
- [ ] R48 Metadatos encontrados en `stdout` (1)
- [ ] R49 Cada metadato con su categoría (0.5)
- [ ] R50 Metadatos no reconocidos en `stderr` (1)
- [ ] R51 Metadatos desconocidos con advertencia (0.5)

### Artículo científico (35 %)
- [ ] A1 Escrito en LaTeX
- [ ] A2 Formato IEEE Transactions (plantilla `IEEEtran`)
- [ ] A3 Máximo 4 páginas

**Forma (5 %)**
- [ ] A4 Solo palabras en español
- [ ] A5 Siglas definidas antes de usarse
- [ ] A6 Tablas, figuras y ecuaciones introducidas antes de aparecer
- [ ] A7 Nombres de tablas, figuras, etc. con mayúscula (por ejemplo, "Tabla 1")
- [ ] A8 Título debajo en todo excepto las tablas
- [ ] A9 Título de las tablas encima
- [ ] A10 Tablas sin divisiones verticales
- [ ] A11 Líneas horizontales solo al inicio, al final y bajo el encabezado (`booktabs`)
- [ ] A12 Sin código textual: usar diagramas
- [ ] A13 Palabras clave en orden alfabético
- [ ] A14 Resumen (*abstract*) en inglés

**Contenido (16 %)**
- [ ] A15 El resumen permite decidir si el artículo es útil (1)
- [ ] A16 Palabras clave representativas (0.5)
- [ ] A17 Introducción con contexto, trabajos relacionados y estado del arte (2)
- [ ] A18 Introducción termina con la estructura del documento (0.5)
- [ ] A19 Teoría necesaria: LTI, convolución, OLA/OLS, cepstro y `R_cc` (4)
- [ ] A20 Cada módulo descrito con detalle suficiente para reproducirlo (4)
- [ ] A21 Respuestas ante distintas configuraciones y escenarios
- [ ] A22 Resultados relacionados con la teoría
- [ ] A23 Resultados por cada componente del sistema
- [ ] A24 Conclusiones solo sobre resultados presentados (0.5)
- [ ] A25 Una conclusión por cada resultado analizado (2)
- [ ] A26 Recomendaciones para trabajo futuro (0.5)
- [ ] A27 Citas bibliográficas (1)

**Análisis (14 %)**
- [ ] A28 PESQ ante diferentes configuraciones (3)
- [ ] A29 Tasa de recuperación ante diferentes configuraciones (3)
- [ ] A30 Correlación entre tasa de recuperación y calidad perceptual (2)
- [ ] A31 Correlación entre tasa de transferencia y calidad perceptual (2)
- [ ] A32 Pruebas con distintos géneros musicales (2)
- [ ] A33 Análisis adicionales propios, por ejemplo canal derecho contra izquierdo, silencios o recorte (*clipping*) (2)

### Trabajo opcional (extra)
- [ ] O1 Codificar la imagen de portada del disco (5)
- [ ] O2 Sistema de corrección de errores, por ejemplo Hamming o repetición (5)
- [ ] O3 Ejecución en línea con parlante y micrófono (5)

---

## Instalación, construcción y ejecución

_Pendiente: completar cuando la implementación esté lista._
