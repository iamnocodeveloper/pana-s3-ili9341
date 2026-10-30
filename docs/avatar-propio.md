# Avatar propio a partir de una imagen

El avatar de esta copia no es el robot de Erik: sale de **una imagen de personaje** (pixel art, de frente, 1254×1254) y `tools/avatar_imagen.py` saca de ella los 16 estados.

```powershell
python -X utf8 tools/avatar_imagen.py avatar/fuente/avatar.webp avatar/manuel   # 278 cuadros PNG de 120x160
python -X utf8 tools/make_avatar.py avatar/manuel                               # escribe sprites43.h y sprites_data43.cpp
cd firmware; ..\.venv\Scripts\pio run -e s3-ili9341 -t upload --upload-port COM4
```

`-X utf8` hace falta en Windows: sin él `make_avatar.py` falla al escribir el `→` de un comentario y deja el archivo a medias.

## Cómo se hacen los estados

- **Fondo:** se rellena desde los bordes (el fondo es casi negro azulado) y se cierran los huecos del abrigo.
- **Cuerpo:** es la imagen. Cada estado la desplaza, la inclina o la estira (saltos, balanceo) alrededor de los pies.
- **Cara:** ojos cerrados, felices, en X o medio cerrados, y boca abierta, sonriente o torcida, pintados encima.
- **Brazos:** se borra el trozo del brazo de la taza y/o el de la laptop y se dibujan de nuevo la manga, la mano y el objeto en la pose del estado (`parametros()` define cuál).
- **Accesorios:** los signos de pregunta, los puntos de pensar, el engranaje, los «zzz» y demás son los de `tools/demo_avatar.py`.

| Estado | Pose |
|---|---|
| idle | taza en la mano; cada pocos segundos sube a la boca y bebe |
| talking | boca animada, taza en la mano |
| wave | suelta la taza y saluda con la mano vacía |
| listening | mano junto a la oreja |
| thinking | puño en la barbilla |
| confused | se rasca la cabeza, un ojo medio cerrado |
| approval | pulgar arriba |
| success | salta con las dos manos arriba |
| error | manos junto a la cara, ojos en X |
| typing · executing · working · reading · searching | laptop abierta en las dos manos; la pantalla muestra código, terminal, progreso, texto o buscador |
| reconnecting · sleeping | taza en la mano; dormido, además, inclinado y con los ojos cerrados |

## Cambiar el personaje

Cambia la imagen de `avatar/fuente/` por otra de las mismas proporciones y ajusta en `avatar_imagen.py` los puntos que dependen de ella: `OJOS`, `BOCA`, `HOMBRO`, `TAZA_MANO`, `CHUNK_IZQ`, `CHUNK_DER`, `FEET` y `ESC`. La escala en pantalla es `-DAV_NUM=5 -DAV_DEN=4` en `platformio.ini`.

## Qué trae este repositorio

El repositorio público lleva el **robot de Erik** compilado en `firmware/src/character/sprites*43.h/.cpp`, para no publicar la cara de nadie. `tools/avatar_imagen.py` sí está, pero la imagen de partida (`avatar/fuente/`) y los PNG (`avatar/`) están en `.gitignore`.

Para usar un avatar propio: pon tu imagen en `avatar/fuente/`, genera los cuadros, ejecuta `make_avatar.py` y, si el personaje es más ancho que el robot, usa `-DAV_NUM=5 -DAV_DEN=4` en el entorno `s3-ili9341` de `platformio.ini` (el robot usa 4/3). Antes de hacer `git commit`, mira `git status`: los dos archivos `sprites*43` llevarían tu avatar.
