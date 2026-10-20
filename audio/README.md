# Pistas de audio de prueba

Todas las pistas se obtuvieron de [Pixabay](https://pixabay.com/) y se usan bajo la
[Licencia de contenido de Pixabay](https://pixabay.com/service/license-summary/).

## Resumen

| Pista | Género / tipo | Duración | fs original | Canales | Media / pico | Silencios |
|---|---|---|---|---|---|---|
| `Chopin - Twelve Etudes Op.25 - 9. No.9 in G flat - Allegro vivace.mp3` | Clásica, piano solo | 0:56 | 44.1 kHz | Mono | -17.7 / -0.3 dB | 2 pausas, 3.4 s |
| `marmixer-see-you-later-203103.mp3` | Clásica moderna, piano | 1:16 | 44.1 kHz | Estéreo real | -15.7 / 0.0 dB | 2 pausas, 3.9 s |
| `aurec-acid-house-590194.mp3` | Acid house, electrónica | 3:22 | 48 kHz | Estéreo | -13.9 / -2.0 dB | Ninguno |
| `indie-rock.mp3` | Indie rock, banda | 1:36 | 44.1 kHz | Estéreo real | -13.2 / 0.0 dB | 2 (inicio y final), 2.6 s |
| `narration_2.mp3` | Voz hablada (narración) | 0:36 | 24 kHz | Estéreo desbalanceado | -29.5 / -1.5 dB | 15 pausas, 9.2 s |

Los niveles están en dBFS. Un pico de 0 dB es el máximo representable en 16 bits.

## Detalle por pista

### Chopin, Estudio Op. 25 n.º 9

- **Archivo:** mono de un solo canal (`channels=1`). Al convertirlo a estéreo, ambos
  canales quedan idénticos, por lo que **no sirve para comparar canales**.
- **Nivel:** media de -17.7 dB y pico de -0.3 dB en la pista completa. El fragmento usado
  (0 a 20 s) es más suave: media de -23.3 dB y pico de -4.9 dB.
- **Uso previsto:** referencia de música clásica acústica, con dinámica amplia.

### marmixer, *See You Later*

- **Estéreo:** canal izquierdo con media de -16.3 dB y derecho con -15.2 dB. La diferencia
  L − R tiene una media de -15.2 dB (correlación entre canales ≈ 0.44), así que es una
  mezcla estéreo abierta.
- **Nivel:** el pico llega a 0 dBFS (masterizada al máximo). Cualquier eco con a > 0
  saturará en los pasajes fuertes.
- **Uso previsto:** comparar el canal izquierdo con el derecho, y analizar la saturación.
  Junto con Chopin forma un par con el mismo instrumento (piano) y producción distinta.

### aurec, *Acid House*

- **Estéreo:** canales casi iguales en nivel (-14.0 y -13.8 dB). La diferencia L − R tiene
  una media de -23.3 dB (correlación ≈ 0.94), así que es un estéreo moderado.
- **Nivel:** el más alto en promedio (-13.9 dB), con 2 dB de margen en el pico. El factor
  de cresta es de unos 12 dB: está muy comprimida.
- **Sin silencios:** es música continua y densa, el caso más favorable para el
  decodificador según Gruhl et al.
- **fs original de 48 kHz:** se remuestrea a 44.1 kHz.

### Indie rock (`indie-rock.mp3`)

- **Estéreo:** canales con el mismo nivel (-13.1 y -13.2 dB). La diferencia L − R tiene
  una media de -16.3 dB (correlación ≈ 0.76), así que es un estéreo real con
  instrumentos distribuidos entre ambos canales.
- **Nivel:** la pista más fuerte en promedio (-13.2 dB), con el pico en 0 dBFS y un factor
  de cresta de unos 13 dB. Es la que más va a saturar al agregar el eco.
- **Silencios:** solo al inicio (0 a 1.0 s) y al final (94.5 a 96.0 s). El cuerpo de la
  canción es continuo.
- **Uso previsto:** representante de música con banda (guitarras, batería y voz), con un
  espectro denso de banda completa.

### Narración (`narration_2.mp3`)

- **Estéreo desbalanceado:** canal izquierdo con media de -32.1 dB y derecho con -27.9 dB
  (el derecho ≈ 4 dB más fuerte). La diferencia L − R tiene una media de -29.0 dB
  (correlación ≈ 0.49).
- **Silencios:** 15 pausas que suman 9.2 s (≈ 25 % del audio). Es el caso difícil para
  detectar ventanas sin eco (R47).
- **fs original de 24 kHz:** solo tiene contenido hasta 12 kHz. Tras remuestrear a
  44.1 kHz, la banda de 12 a 22.05 kHz queda vacía, lo que afecta a `ln|X|` en el cepstro.

## Conversión al formato del proyecto

El sistema trabaja con WAV de 44.1 kHz, 16 bits *little endian* y estéreo (R25):

```bash
O="-ar 44100 -ac 2 -sample_fmt s16 -map_metadata -1 -fflags +bitexact -flags:a +bitexact"
ffmpeg -i "Chopin - Twelve Etudes Op.25 - 9. No.9 in G flat - Allegro vivace.mp3" -t 20 $O clasica_chopin.wav
ffmpeg -i marmixer-see-you-later-203103.mp3 -ss 30 -t 20 $O clasica_moderna_marmixer.wav
ffmpeg -i aurec-acid-house-590194.mp3       -ss 60 -t 20 $O house_aurec.wav
ffmpeg -i indie-rock.mp3                    -ss 30 -t 20 $O rock_indie.wav
ffmpeg -i narration_2.mp3                               $O voz_narracion.wav
```

- `-ss` y `-t` recortan 20 s de cada canción, saltando la introducción. La narración se
  conserva completa porque es corta y sus pausas son parte de la prueba.
- `-map_metadata -1` y las opciones `bitexact` evitan que ffmpeg agregue un bloque `LIST`
  con metadatos. Así cada archivo tiene el encabezado RIFF canónico de 44 bytes
  (`RIFF`, `fmt ` de 16 bytes y `data`), y la reconversión es reproducible.
- Las pistas mono se duplican a ambos canales con `-ac 2`.

### Archivos WAV generados

Todos son PCM de 16 bits *little endian*, 44.1 kHz y estéreo, con encabezado de 44 bytes.

| Archivo | Origen | Duración | Media / pico |
|---|---|---|---|
| `clasica_chopin.wav` | Chopin, 0 a 20 s | 20 s | -23.3 / -4.9 dB |
| `clasica_moderna_marmixer.wav` | marmixer, 30 a 50 s | 20 s | -14.1 / 0.0 dB |
| `house_aurec.wav` | aurec, 60 a 80 s | 20 s | -15.7 / -2.7 dB |
| `rock_indie.wav` | indie rock, 30 a 50 s | 20 s | -12.4 / 0.0 dB |
| `voz_narracion.wav` | narración completa | 36.4 s | -29.5 / -1.4 dB |

Los niveles de los fragmentos difieren de los de la pista completa. El fragmento de Chopin,
por ejemplo, tiene 4.9 dB de margen aunque la pista completa llegue a -0.3 dB.

### Consideraciones sobre la narración

La narración es la única pista cuya conversión puede comprometer los resultados. Hay que
tenerlo en cuenta al analizarla y al compararla con las demás:

1. **Ancho de banda limitado.** El original está a 24 kHz, así que solo tiene contenido
   hasta 12 kHz (Nyquist). Remuestrear a 44.1 kHz no recupera la banda de 12 a 22.05 kHz:
   la energía por encima de 12.5 kHz es de -66.7 dB, frente a -37.2 dB en
   `rock_indie.wav` (unos 30 dB menos).
2. **Efecto sobre el cepstro.** En esa banda vacía, `ln|X(k)|` toma valores muy negativos
   que dependen casi solo del ruido de cuantización y de la constante ε usada para evitar
   `ln(0)`. Eso puede agregar ruido a `R_cc` y bajar la tasa de recuperación. Conviene
   documentar el valor de ε y, si es posible, comparar el resultado con y sin limitar el
   análisis a la banda útil.
3. **Silencios.** El 25 % del audio son pausas. En esas ventanas no hay señal que lleve el
   eco, así que es esperable que se pierdan bits o se clasifiquen como "sin eco" (R47).
   La tasa de recuperación de esta pista debe reportarse aparte o separando ventanas con
   voz y ventanas en silencio.
4. **Doble compresión.** El MP3 original ya perdió información. Esto no afecta la
   comparación con PESQ, porque la referencia es el WAV convertido y no el MP3, pero debe
   mencionarse al describir los datos.
5. **PESQ.** Como PESQ trabaja a 16 kHz (banda ancha, hasta 8 kHz), la falta de contenido
   por encima de 12 kHz no afecta la medición de calidad perceptual de esta pista.

## Cómo se midieron las características

```bash
# Duración, frecuencia de muestreo y canales
ffprobe -v error -show_entries format=duration:stream=channels,sample_rate -of default=nw=1 pista.mp3

# Nivel medio y pico
ffmpeg -i pista.mp3 -af volumedetect -f null - 2>&1 | grep -E "mean_volume|max_volume"

# Nivel de cada canal (c0 = izquierdo, c1 = derecho)
ffmpeg -i pista.mp3 -af "pan=mono|c0=c0,volumedetect" -f null - 2>&1 | grep mean_volume

# Diferencia L − R (≈ -91 dB o menos indica canales idénticos)
ffmpeg -i pista.mp3 -af "pan=mono|c0=c0-c1,volumedetect" -f null - 2>&1 | grep mean_volume

# Silencios (por debajo de -40 dB durante al menos 0.3 s)
ffmpeg -i pista.mp3 -af silencedetect=n=-40dB:d=0.3 -f null - 2>&1 | grep silence_duration
```

La correlación entre canales se estima a partir de las potencias medias:
ρ = (P_L + P_R − P_{L−R}) / (2·√(P_L·P_R)), con P = 10^(dB/10).
