# Solución de problemas

Los tropiezos reales de esta adaptación, por síntoma. Están en el orden en que salieron.

## Al compilar y grabar

| Síntoma | Causa | Solución |
|---|---|---|
| PlatformIO: `CERTIFICATE_VERIFY_FAILED` al descargar | Un antivirus o proxy revisa el HTTPS y Python no confía en su certificado | `pip install pip-system-certs` en el `.venv`. Si falla el instalador `uv` de la plataforma pioarduino, define `UV_SYSTEM_CERTS=1`. |
| `Failed to install Python dependencies into penv` con la plataforma pioarduino | El instalador de esa plataforma falla en algunos equipos Windows sin dar el motivo | Por eso el entorno `s3-ili9341` usa la plataforma estándar `espressif32@6.9.0`. |
| `idf_tools.py ... MSys/Mingw is not supported` | Se compiló desde Git Bash | Usa PowerShell o cmd. |
| `avatar43.cpp: no match for 'operator='` | La plataforma estándar compila con C++11 y el avatar usa C++ más nuevo | Ya resuelto: el entorno compila con `-std=gnu++17`. |
| `esptool: invalid choice: 'usb-reset'` | Esa versión escribe la opción con guion bajo | Ya resuelto: `--before=usb_reset`. |
| `Failed to connect to ESP32-S3: No serial data received` | La placa no está en modo descarga o el puerto COM cambió | Mantén **BOOT**, pulsa **RST**, suelta BOOT, y vuelve a mirar el puerto COM. |
| La grabación se corta al 15 % con `PermissionError` o `No serial data received` tras cambiar la velocidad | El USB de algunos equipos no aguanta 460 800 baudios | Ya resuelto: `upload_speed = 115200`. |
| El número de COM cambia | El USB nativo del ESP32-S3 se vuelve a enumerar al reiniciar y en modo descarga | Mira siempre el puerto actual antes de grabar. |

## La placa arranca pero no funciona

| Síntoma | Causa | Solución |
|---|---|---|
| **Pantalla en blanco o gris con la luz encendida** y no dibuja nada | La placa arrancó en **modo descarga**. El monitor serie dice `boot:0x1 (DOWNLOAD(USB/UART0))` y `waiting for download`. El GPIO 0 está a tierra al reiniciar. | Suelta BOOT. Revisa que ningún cable esté en el pin `0`, ni en el `45` o `46` (el conector de `MOSI`, GPIO 35, se corre fácilmente hacia el 0). Desenchufa y vuelve a enchufar sin tocar nada. Debe decir `SPI_FAST_FLASH_BOOT`. |
| La luz de fondo está encendida pero no hay imagen, y el programa sí corre | El módulo no recibe comandos | Revisa `RST` (GPIO 42), `WR` (1) y `RS` (2), `CS` a tierra y `RD` a 3V3. |
| Colores raros, rayas o ruido | Un cable de datos cruzado, o el módulo está en 8 bits | Revisa los `DB0`–`DB15` uno a uno. Junto al texto `16Bit (DB0~DB15)` debe haber un puente de soldadura, y en `8Bit` no. |
| El táctil da coordenadas enormes (`x=3500`) | Falta calibrarlo | Escribe `cal` en el monitor serie y toca las cruces. |
| Los puntos del táctil salen corridos o invertidos | Calibración con toques flojos, o pantalla al revés | Repite `cal`. Si la imagen sale al revés, cambia `LCD_ROTATION` de 1 a 3 en `board.h`. |
| El táctil responde solo a veces | Un pin del conector soldado flojo (por ejemplo el `PEN`, GPIO 39) | Vuelve a soldar la unión. |
| La hora sale una hora corrida | `TZ_POSIX` no es tu zona | Cámbialo en `firmware/include/config.h` y regraba. |
| La barra de RAM muestra números raros | Era un fallo de una versión anterior (el servidor manda la RAM en %) | Actualiza el firmware. |

## Wi-Fi y servidor

| Síntoma | Causa | Solución |
|---|---|---|
| Tu red no aparece en el portal `Pana-Setup` | El ESP32 solo ve redes de **2,4 GHz** | Activa la banda de 2,4 GHz del router. |
| Wi-Fi en verde y **Servidor en amarillo**, «Buscando el servidor…» | El firewall de Windows bloquea a la placa porque tu red es **Pública** | Pasa la red a **Privada** y permite Python en el aviso del firewall. |
| Sigue en amarillo con la red Privada | IP del servidor o puerto mal puestos, o el servidor está apagado | En el monitor serie, `host <ip>` y reinicia; comprueba que `iniciar-pana.bat` está abierto. La IP del PC sale con `ipconfig`. |
| El servidor falla al arrancar con `ZoneInfoNotFoundError` | El `.env` leía mal los comentarios | Ya resuelto en `server/config.py`. |
| `CPU` y `RAM` salen a cero | Se usa un servidor sin `psutil` | `pip install psutil` (está en `requirements.txt`). |
| Los botones se iluminan pero no abren nada | `launcher.json` mal formado, o el servidor no lo ve | Mira la consola del servidor: si hay un error de JSON lo dice. Solo se abren `http` y `https`. |

## Comandos del monitor serie

`pio device monitor` a 115 200:

| Orden | Qué hace |
|---|---|
| `info` | Versión, memoria libre y tiempo encendido |
| `cal` | Repite la calibración del táctil |
| `host <ip>` · `port <n>` | Cambian el servidor (se aplican al reiniciar) |
| `wifi reset` | Borra el Wi-Fi guardado y abre el portal |
| `s` | Vuelca la pantalla por serie (para `tools/shot.py`) |
