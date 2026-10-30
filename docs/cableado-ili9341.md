# Cableado: ESP32-S3 N8R2 + módulo ILI9341 de 3,2" (MRB3205)

Pantalla de 240×320 con bus paralelo de **16 bits** y táctil resistivo **XPT2046** por SPI. Son 25 cables de señal más alimentación. Entorno de PlatformIO: `s3-ili9341` (`firmware/platformio.ini`); los pines están en `firmware/include/board.h`.

El módulo trae una tira de 2×17 pines con la serigrafía por columnas. Con la tira arriba y de izquierda a derecha:

| Columna | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Exterior | CLK | F_CS | MOSI | NC | GND | VDD | SDCS | DB14 | DB12 | DB10 | DB8 | DB6 | DB4 | DB2 | DB0 | RD | RS |
| Interior | T_CS | PEN | MISO | GND | VDD | BL | DB15 | DB13 | DB11 | DB9 | DB7 | DB5 | DB3 | DB1 | RST | WR | CS |

## Pines

| Módulo | GPIO | Módulo | GPIO |
|---|---|---|---|
| `DB0` | 4 | `DB8` | 8 |
| `DB1` | 5 | `DB9` | 9 |
| `DB2` | 6 | `DB10` | 10 |
| `DB3` | 7 | `DB11` | 11 |
| `DB4` | 15 | `DB12` | 12 |
| `DB5` | 16 | `DB13` | 13 |
| `DB6` | 17 | `DB14` | 21 |
| `DB7` | 18 | `DB15` | 47 |
| `WR` | 1 | `CLK` (táctil) | 36 |
| `RS` (DC) | 2 | `MOSI` (táctil) | 35 |
| `RST` | 42 | `MISO` (táctil) | 37 |
| `BL` | 41 | `T_CS` (táctil) | 40 |
| `RD` | 3V3 | `PEN` (táctil) | 39 |
| `CS` | GND | `VDD` / `GND` | 3V3 / GND |

Sin conectar: `F_CS`, `SDCS`, `NC` y el GPIO 38.

## Cosas que se aprendieron

- **Evita el GPIO 0, 3, 45 y 46** (arranque), el 19 y 20 (USB) y el 48 (LED RGB). Un cable en el GPIO 0 deja la placa en modo descarga y el programa no corre.
- **El módulo debe estar en 16 bits**: soldadura sobre `16Bit (DB0~DB15)`, y vacía la de `8Bit`.
- **Bus a 10 MHz**: con cables de puente largos, más no aguanta.
- **El ESP32 solo usa Wi-Fi de 2,4 GHz.**
- **Si no graba**: mantén BOOT, pulsa y suelta RST, suelta BOOT. El puerto COM cambia (en modo descarga y en ejecución es otro).
- **Calibrar el táctil**: la primera vez pide tocar cuatro cruces y la guarda. Para repetirla, escribe `cal` en el monitor serie.
