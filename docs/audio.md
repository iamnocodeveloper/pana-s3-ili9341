# Micrófono y altavoz

> **Estado: planeado, no implementado.** El firmware de esta placa compila con `HAS_AUDIO 0`: no hay códec de audio y no suena ni escucha. El servidor lo sabe porque la placa se presenta con `mic=False, altavoz=False`.

El audio solo tiene sentido cuando haya un agente detrás y los servicios de voz del servidor (Whisper para escuchar y Piper para hablar, ver el [README de Erik](../README.erik.md)). Se pueden comprar las piezas ya y conectarlas después.

## Piezas

| Pieza | Qué comprar | Buscar como |
|---|---|---|
| Micrófono | Módulo MEMS con salida digital I2S, 3,3 V | `INMP441 I2S microphone module` |
| Amplificador | Amplificador I2S clase D de 3 W | `MAX98357A I2S amplifier module` |
| Altavoz | 4 Ω u 8 Ω, 3 W, de 28 a 40 mm | `speaker 4 ohm 3W 40mm` |

- Mejor con los pines **ya soldados**. Si no, hay que soldarlos para usar cables Dupont. El altavoz va soldado o atornillado al módulo del amplificador.
- Un altavoz Bluetooth normal **no sirve**: el ESP32-S3 solo trae BLE, no Bluetooth clásico.

## Pines

El micrófono y el amplificador comparten el reloj del bus I2S, así que solo hacen falta cuatro GPIO. Quedan libres tras la pantalla y el táctil:

| Señal | GPIO | Va a |
|---|---|---|
| BCLK (compartido) | 14 | `BCLK` del amplificador y `SCK` del micrófono |
| WS (compartido) | 44 | `LRC` del amplificador y `WS` del micrófono |
| DOUT, hacia el amplificador | 38 | `DIN` del amplificador |
| DIN, desde el micrófono | 43 | `SD` del micrófono |

| Alimentación | Va a |
|---|---|
| Amplificador `VIN` | `5V` de la placa |
| Micrófono `VDD` | `3V3` de la placa |
| `GND` de los dos | `GND` |
| Micrófono `L/R` | `GND` (canal izquierdo) |

- Las dos salidas `3V3` de la placa ya están usadas por la pantalla (`VDD` y `RD`). Para dejar una libre, haz un puente entre `RD` y el segundo `VDD` del módulo de pantalla, y usa ese `3V3` para el micrófono.
- Dos módulos en el mismo GPIO (14 y 44) no caben con cables hembra. Necesitas una **mini protoboard**, un cable en Y, o soldar dos cables al mismo pin.
- Los GPIO 43 y 44 son los del UART0, pero la consola de esta placa va por el USB nativo, así que están libres. Al arrancar la ROM del chip escribe por el 43 y el amplificador puede dar un chasquido.

## Lo que falta por hacer en el software

1. Un controlador I2S **sin códec** para esta placa. El que hay (`firmware/src/audio/audio.cpp`) maneja los chips ES8311 y ES7210 de las placas Waveshare. Formato esperado por el servidor: PCM de 16 bits, mono, a 16 kHz.
2. El **pulsar para hablar**: mantener la cara pulsada ya envía `audio_start` en las otras placas; hay que cablearlo en `ui320.cpp`.
3. Instalar Whisper, Piper y el agente en el servidor.

## Alternativa sin comprar nada

Usar el micrófono y los altavoces del PC desde el servidor, y que la placa solo muestre. Habría que añadir al servidor la captura y reproducción de audio en el PC.

## La caja

El altavoz de 40 mm necesita unos 40 × 40 × 12 mm y varios agujeros en el cartón; el micrófono, uno pequeño de 3 mm en el frente. Cuando tengas las piezas, mide y se añaden a [`caja/generar_caja.py`](../caja/generar_caja.py).
