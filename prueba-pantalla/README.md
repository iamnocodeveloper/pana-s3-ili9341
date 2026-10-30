# Prueba de pantalla y táctil

Un programa mínimo para comprobar el cableado de la pantalla ILI9341 y el táctil **antes** de usar el firmware completo. No usa Wi-Fi ni el servidor.

- Pasa por **rojo, verde, azul y blanco**, con el nombre del color.
- Después dibuja un punto amarillo donde tocas y muestra las coordenadas.
- Por el monitor serie (115 200) escribe lo que hace.

```powershell
cd prueba-pantalla
..\.venv\Scripts\pio run -t upload --upload-port COM4
```

Pines y cableado: [../docs/cableado-ili9341.md](../docs/cableado-ili9341.md).

## Qué indica cada resultado

| Se ve | Significa |
|---|---|
| Nada, luz apagada | `BL` (GPIO 41), `VDD` o `GND` |
| Blanco o gris con luz y nada más | O el programa no corre (la placa está en modo descarga, mira [solución de problemas](../docs/solucion-de-problemas.md)), o `RST`, `WR`, `RS` o `CS` |
| Colores equivocados o ruido | Un cable de datos, o el módulo en modo 8 bits |
| Colores bien y el táctil no escribe | `T_CS`, `CLK`, `MOSI`, `MISO` o `PEN` |

## La calibración

Los cuatro números de `cal[8]` en `src/main.cpp` son de **una** unidad concreta (se midieron tocando las esquinas). Con otra pantalla salen distintos: sustituye `tft.setTouchCalibrate(cal)` por `tft.calibrateTouch(cal, TFT_WHITE, TFT_BLACK, 20)`, imprime `cal` por serie tras tocar las cruces, y copia los valores. El firmware completo guarda su propia calibración y la pide la primera vez.
