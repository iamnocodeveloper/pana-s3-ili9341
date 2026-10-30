# Hardware

Todo lo que el firmware sabe de cada placa está en [`firmware/include/board.h`](../firmware/include/board.h) (pines, pantalla, táctil) y en un archivo `board*.cpp` (arranque, táctil y cómo se manda la imagen a la pantalla). La interfaz se elige por tamaño: `ui175.cpp` para 466×466, `ui720.cpp` para 720×720 y `ui320.cpp` para 320×240.

## Waveshare ESP32-S3-Touch-AMOLED-1.75 / 1.75C · `amoled175`

| Parte | Chip | Conexión |
|---|---|---|
| Pantalla | CO5300, AMOLED 466×466 (memoria de 480 columnas, 6 de margen) | QSPI: D0–D3 = GPIO 4–7, SCLK 38, CS 12, RST 39 |
| Táctil | CST9217 (0x5A) | I2C SDA 15 / SCL 14, INT 11, RST 40 |
| Altavoz | ES8311 (0x18) + amplificador | I2S: MCLK 42, BCLK 9, WS 45, DOUT 8 · PA en GPIO 46 (alto = suena) |
| Micrófonos | ES7210 (0x40), MIC1 + MIC2 | DIN 10 · **MCLK también por GPIO 16** |
| Energía | AXP2101 (0x34) | ALDO1 + BLDO1 encendidos al arrancar |
| Movimiento | QMI8658 (0x6B) | despierta la pantalla al levantarla |

- El CO5300 solo acepta ventanas que empiezan y acaban en coordenada par: `LGFX::flush()` lo redondea.
- La imagen va por franjas de 16 filas copiadas a SRAM con DMA.
- Si el táctil no responde (no hay ACK en 0x5A), suele estar colgado: quita la corriente del todo.

## Waveshare ESP32-P4-WIFI6-Touch-LCD-4C · `p4-4c`

| Parte | Chip | Conexión |
|---|---|---|
| Pantalla | JD9365, IPS 720×720 | MIPI-DSI 2 carriles a 1500 Mbps, DPI 80 MHz · RST 27 · retroiluminación PWM en GPIO 26 (invertida) |
| Táctil | GT911 (0x5D) | I2C SDA 7 / SCL 8 |
| Audio | ES8311 + ES7210 | I2S: MCLK 13, BCLK 12, WS 10, DOUT 9, DIN 11 · PA en GPIO 53 |
| Wi-Fi 6 / BLE | ESP32-C6 por SDIO (esp_hosted) | CLK 18, CMD 19, D0–D3 14–17, reset 54 (los pines por defecto del core) |

- **Grabar:** el esptool de PlatformIO se cuelga con el P4 rev 3; usa `tools/flash_p4.sh` (esptool de ESP-IDF). Grabar el `.factory.bin` completo borra la NVS: la placa olvida la Wi-Fi guardada por el portal.
- **CABC:** el panel trae el brillo adaptativo al contenido; con el personaje animándose, el fondo «palpita». El arranque manda `0x55 = 0x00` para apagarlo.

### P4: memoria y pantalla

El DPI lee la imagen continuamente de la PSRAM, la misma en la que se dibuja. Copiar el cuadro entero (1 MB) en cada fotograma produce rayas verticales y parpadeo. Por eso `ui720.cpp` pinta por capas:

- **En cada cuadro:** solo el anillo del borde, el rectángulo del personaje si cambió y el panel si se está animando.
- **Copia a pantalla:** `LGFX::flush()` copia solo esos rectángulos con el PPA (que de paso cambia el orden de bytes).
- **Repintado completo:** solo al cambiar de página o de contenido, y por franjas de 48 filas con una pausa entre ellas.

Si añades algo que se mueve, repíntalo y cópialo por su cuenta en vez de forzar un repintado completo.

## ESP32-S3 N8R2 + módulo ILI9341 de 3,2" (MRB3205) · `s3-ili9341`

*Añadida por la adaptación de este repositorio; no es una placa Waveshare.*

| Parte | Chip | Conexión |
|---|---|---|
| Placa | ESP32-S3 N8R2: flash de 8 MB, PSRAM quad de 2 MB, dos USB-C | USB nativo para grabar y para la consola |
| Pantalla | ILI9341, 240×320 (se usa apaisada) | Bus paralelo de **16 bits**: `DB0`–`DB15` en los GPIO 4, 5, 6, 7, 15, 16, 17, 18, 8, 9, 10, 11, 12, 13, 21, 47 · `WR` 1 · `RS` 2 · `RST` 42 · `BL` 41 · `RD` a 3V3 · `CS` a GND |
| Táctil | XPT2046, resistivo | SPI: `CLK` 36, `MOSI` 35, `MISO` 37, `T_CS` 40, `PEN` 39 |
| Audio | ninguno (`HAS_AUDIO 0`) | Plan en [audio.md](audio.md) |

- **Cableado completo y fotos del módulo:** [cableado-ili9341.md](cableado-ili9341.md).
- **Imagen:** `LGFX::flush()` copia el trozo que cambió del sprite de pantalla completa (en PSRAM). El bus va a 10 MHz: con cables de puente largos no aguanta más.
- **Táctil:** los valores del XPT2046 son en bruto; la calibración se hace la primera vez y se guarda en NVS (`touchSetup()` en `board_s3_ili9341.cpp`). La orden serie `cal` la repite.
- **Memoria:** la tabla `partitions8.csv` da dos apps de 3,6 MB para OTA. Con las 16 animaciones del avatar el firmware ocupa unos 1,9 MB.
- **Evita el GPIO 0, 3, 45, 46** (arranque), el 19 y 20 (USB) y el 48 (LED RGB). Un cable en el GPIO 0 deja la placa en modo descarga.
- **Compilador:** usa la plataforma estándar `espressif32@6.9.0` (Arduino-ESP32 2.x) con `-std=gnu++17`, no la de pioarduino.

## Añadir una placa

1. **`board.h`:** un bloque `#elif defined(BOARD_X)` con sus pines y una clase `LGFX` con `init()`, `getTouch()` y `flush()` (o `present()`). Mira la 1.75, con un panel QSPI, o la P4, con DSI. Los pines y las secuencias de arranque están en el BSP oficial de Waveshare de cada placa ([registro de componentes de Espressif](https://components.espressif.com/components?q=waveshare)).
2. **`config.h`:** `DEVICE_ID`, `SCR_W` y `SCR_H`.
3. **`platformio.ini`:** un entorno con su `board`, su tipo de memoria y `-DBOARD_X -DPACK_AVATAR` (y `-DROUND_UI` si usa la interfaz de 466).
4. **Interfaz:** si la pantalla no es de 466 ni de 720, copia la más parecida y ajusta las medidas.
5. **Audio:** si usa ES8311/ES7210 como estas, basta con los pines (y `HAS_ES7210`).

Comprueba cada paso con el monitor serie: la placa imprime si la pantalla, el táctil y el audio arrancan. Usa `s` para sacar una captura de lo que pinta.
