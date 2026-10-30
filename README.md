# Pana · ESP32-S3 + pantalla ILI9341

Un companion de escritorio con cara para tu ordenador, en una placa barata: un **ESP32-S3 N8R2** con una pantalla táctil **ILI9341 de 3,2"**. Hoy enseña un personaje animado, la hora, el estado de tu PC y botones para abrir tus apps. Más adelante, con un agente de IA detrás, conversa.

> **Este proyecto es una adaptación de [pana-companion](https://github.com/eriktaveras/pana-companion), de Erik Taveras** (licencia MIT). El diseño del sistema, el protocolo, el servidor, el avatar y el firmware original son suyos. Aquí se añade el soporte para otra placa y otra pantalla. Mira [Créditos](#créditos-y-licencia) y [CAMBIOS.md](CAMBIOS.md).

## Qué hace hoy

Probado en el hardware de abajo:

- **Avatar animado** de 16 estados (listo, pensando, hablando, sin conexión…) y saluda al tocarle la cara.
- **Reloj y fecha** en tu zona horaria.
- **Datos de tu PC con Windows:** CPU, RAM, tiempo encendido y cuántas ventanas de navegador tienes abiertas.
- **Botones para abrir apps** (YouTube y el navegador de serie). Solo abren lo que está en una lista cerrada, `server/launcher.json`.
- **Wi-Fi sin tocar código:** la placa abre una red `Pana-Setup` donde pones tu Wi-Fi y la IP del servidor.
- Tarjetas con botones, confirmaciones y burbujas de texto, para cuando conectes un agente.

## Qué falta

- **Conversación con un agente** (Hermes Agent o un LLM local). El servidor y el avatar ya lo soportan, solo falta instalarlo y conectarlo.
- **Voz:** micrófono y altavoz. La placa no los tiene; el plan está en [docs/audio.md](docs/audio.md).
- **La caja.** Hay un plano para cartón duro: [docs/caja.md](docs/caja.md).

## Hardware

| Pieza | Modelo |
|---|---|
| Placa | ESP32-S3 **N8R2** (flash de 8 MB, PSRAM de 2 MB), con dos USB-C |
| Pantalla | Módulo 3,2" **ILI9341**, 240×320, bus paralelo de **16 bits**, SKU MRB3205 |
| Táctil | Resistivo **XPT2046** por SPI (viene en el módulo) |
| Cables | Dupont hembra-hembra, unos 30 |

El cableado pin a pin está en [docs/cableado-ili9341.md](docs/cableado-ili9341.md).

## Inicio rápido en Windows

Guía completa y con los tropiezos habituales: [docs/instalacion-windows.md](docs/instalacion-windows.md).

```powershell
git clone <este repositorio> pana
cd pana
python -m venv .venv
.venv\Scripts\python -m pip install -r server\requirements.txt platformio pip-system-certs
copy .env.example .env             # y pon tu zona horaria en COMPANION_TZ
```

**Servidor** (el cerebro, en tu PC): doble clic a `iniciar-pana.bat`. Tu red de Windows debe ser **Privada** o el firewall bloquea a la placa.

**Firmware** (la placa):

```powershell
cd firmware
..\.venv\Scripts\pio run -e s3-ili9341 -t upload --upload-port COM4
```

La primera vez la pantalla te pide tocar cuatro cruces para calibrar el táctil. Luego conéctate desde el móvil a la red `Pana-Setup` y rellena tu Wi-Fi (de **2,4 GHz**), la IP del PC y el puerto `8770`.

## Estructura

```
firmware/           el cuerpo (PlatformIO, LovyanGFX)
  src/ui/ui320.cpp    interfaz apaisada de 320×240 (nueva)
  src/board_s3_ili9341.cpp  táctil y envío de imagen de esta pantalla (nuevo)
server/             el cerebro (FastAPI)
  brain/lanzador.py   botones para abrir apps (nuevo)
  launcher.json       lista cerrada de lo que pueden abrir
caja/               generador del plano de la caja en cartón (nuevo)
prueba-pantalla/    programa de prueba de pantalla y táctil (nuevo)
docs/               documentación
iniciar-pana.bat    arranca el servidor en Windows (nuevo)
README.erik.md      el README original de Erik Taveras
```

## Documentación

- [Instalación en Windows](docs/instalacion-windows.md)
- [Cableado de la pantalla](docs/cableado-ili9341.md)
- [Solución de problemas](docs/solucion-de-problemas.md)
- [La caja de cartón](docs/caja.md)
- [Micrófono y altavoz (plan)](docs/audio.md)
- [Lista de cambios sobre el original](CAMBIOS.md)
- Del proyecto original: [README de Erik](README.erik.md), [protocolo](protocol/PROTOCOL.md), [hardware](docs/hardware.md)

## Créditos y licencia

- **Proyecto original: [pana-companion](https://github.com/eriktaveras/pana-companion) de Erik Taveras.** Este repositorio conserva su historial de commits, su licencia y su documentación (`README.erik.md`).
- **Licencia MIT**, la misma que el original. El archivo [LICENSE](LICENSE) no se ha modificado.
- Dependencias de terceros en [THIRD_PARTY.md](THIRD_PARTY.md).
- Esta adaptación (la placa ESP32-S3 + ILI9341, la interfaz 320×240, el lanzador de apps, el soporte para Windows y la caja) la hizo [@iamnocodeveloper](https://github.com/iamnocodeveloper), con ayuda de Claude.
