# Cambios sobre pana-companion

Esta adaptación parte del commit `bc5aa10` de [eriktaveras/pana-companion](https://github.com/eriktaveras/pana-companion). Aquí está todo lo que se añadió o se tocó. Las otras placas del original (`amoled175` y `p4-4c`) siguen igual: los cambios en archivos compartidos son aditivos o van detrás de `#ifdef`.

## Firmware

| Archivo | Cambio |
|---|---|
| `firmware/platformio.ini` | Entorno nuevo `s3-ili9341`: flash de 8 MB, PSRAM quad, C++17, plataforma estándar `espressif32@6.9.0`, grabación a 115 200 baudios. |
| `firmware/include/board.h` | Bloque `BOARD_S3_ILI9341`: ILI9341 por bus paralelo de 16 bits (LovyanGFX), táctil XPT2046, `HAS_AUDIO 0`, rotación apaisada. Pines en [docs/cableado-ili9341.md](docs/cableado-ili9341.md). |
| `firmware/include/config.h` | `DEVICE_ID`, tamaño 320×240, `LCD_ROTATION` y `TZ_POSIX` (zona horaria; `COT5` en esta placa, `AST4` como antes en las demás). |
| `firmware/src/board_s3_ili9341.cpp` | **Nuevo.** Calibración del táctil (guardada en NVS) y envío de imagen a la pantalla. |
| `firmware/src/ui/ui320.cpp` | **Nuevo.** Interfaz de 320×240: avatar a la izquierda, a la derecha reloj, CPU, RAM, ventanas del navegador y botones de apps; también burbujas, tarjetas y confirmaciones. |
| `firmware/src/character/avatar.h` | La escala del avatar es configurable con `AV_NUM`/`AV_DEN` (por defecto ×2,5, como antes). Esta placa usa ×4/3. |
| `firmware/src/main.cpp` | Rotación por placa, orden serie `cal` para recalibrar el táctil, y los comandos de I2C/I2S solo si hay audio. |
| `firmware/src/net/net.cpp` | Zona horaria desde `config.h`; lee `ventanas` y `apps` del mensaje `dash`. |
| `firmware/src/ui/model.h` | Campos `wins` y `apps` en `Dash`. |
| `firmware/partitions8.csv` | **Nuevo.** Tabla de particiones de 8 MB con dos apps de 3,6 MB (OTA). |

## Servidor

| Archivo | Cambio |
|---|---|
| `server/config.py` | El `.env` admite comentarios al final de la línea y se lee en UTF-8. En Windows el `.env.example` original no se podía cargar. Con prueba. |
| `server/brain/tablero.py` | CPU, RAM y tiempo encendido también en Windows y macOS, con `psutil` (antes solo leía `/proc`). Cuenta las ventanas de navegador (solo Windows) y manda la lista de botones. |
| `server/brain/lanzador.py` | **Nuevo.** Abre apps y páginas web desde una lista cerrada (`launcher.json`); rechaza ids desconocidos y direcciones que no sean http o https. Con pruebas. |
| `server/launcher.json` | **Nuevo.** Los botones: YouTube y Navegador. |
| `server/brain/core.py` | El toque `app:<id>` abre el botón correspondiente. |
| `server/requirements.txt` | Se añade `psutil`. |
| `server/tests/test_basico.py` | Dos pruebas nuevas (30 en total). |

## Herramientas y documentación

- `iniciar-pana.bat`: arranca el servidor en Windows con doble clic.
- `prueba-pantalla/`: programa de prueba de pantalla y táctil, para validar el cableado antes de usar el firmware completo.
- `caja/generar_caja.py`: genera el plano 1:1 de una caja de cartón duro.
- `docs/`: cableado, instalación en Windows, solución de problemas, caja y plan de audio.
- `README.md` es el de esta adaptación. El de Erik está en `README.erik.md`.

## Lo que se aprendió

Los detalles están en [docs/solucion-de-problemas.md](docs/solucion-de-problemas.md). Los dos que más tiempo costaron:

1. Un cable en el GPIO 0 deja la placa en **modo descarga** al arrancar. El programa no corre y la pantalla se queda en blanco con la luz encendida.
2. El firewall de Windows bloquea a la placa si la red está marcada como **Pública**.
