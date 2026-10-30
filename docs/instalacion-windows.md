# Instalación en Windows

Probado en Windows 11 con Python 3.13, un ESP32-S3 N8R2 y la pantalla ILI9341 de 3,2". Para el cableado, mira antes [cableado-ili9341.md](cableado-ili9341.md).

## Requisitos

- Windows 10 u 11, **Python 3.11 o más** y **Git**.
- La placa y la pantalla ya cableadas.
- Un cable USB-C **de datos** (algunos solo cargan).
- Tu PC y la placa en la misma red Wi-Fi, y esa red debe ser de **2,4 GHz** (el ESP32 no ve las de 5 GHz).

## 1. El proyecto y el entorno de Python

```powershell
git clone <este repositorio> pana
cd pana
python -m venv .venv
.venv\Scripts\python -m pip install -r server\requirements.txt platformio pip-system-certs
```

`pip-system-certs` hace que Python use los certificados de Windows. Sin él, PlatformIO puede fallar con `CERTIFICATE_VERIFY_FAILED` cuando hay un antivirus o un proxy que revisa el HTTPS.

## 2. Configuración

```powershell
copy .env.example .env
```

Abre `.env` y cambia al menos tu zona horaria, por ejemplo `COMPANION_TZ=America/Bogota`. Los comentarios al final de la línea son válidos.

La hora de la placa sale del servidor, pero su **zona horaria** va dentro del firmware: está en `firmware/include/config.h` (`TZ_POSIX`). `COT5` es UTC-5 sin horario de verano; para República Dominicana usa `AST4`, y para México central `CST6`.

## 3. El servidor

Doble clic a `iniciar-pana.bat`, o a mano:

```powershell
cd server
$env:COMPANION_BRAIN = "local"
..\.venv\Scripts\python main.py
```

Abre el simulador en `http://localhost:8770`. Verás un aviso de que no hay voz instalada; es normal si todavía no tienes Piper.

### El firewall

Si tu red está marcada como **Pública**, Windows bloquea las conexiones de la placa al servidor. Pásala a **Privada**:

*Configuración → Red e Internet → Wi-Fi → propiedades de tu red → Tipo de perfil de red → Privada.*

Al arrancar el servidor Windows puede preguntar si permite a Python comunicarse en redes privadas; pulsa **Permitir**.

## 4. El firmware

Usa **PowerShell o cmd**, no Git Bash.

```powershell
cd firmware
..\.venv\Scripts\pio run -e s3-ili9341
```

La primera compilación tarda varios minutos: descarga la plataforma y compila el núcleo de Arduino. Después:

```powershell
..\.venv\Scripts\pio run -e s3-ili9341 -t upload --upload-port COM4
```

Cambia `COM4` por tu puerto (Administrador de dispositivos → Puertos).

### Si no conecta al grabar

Pon la placa en modo descarga: mantén **BOOT**, pulsa y suelta **RST**, y suelta BOOT. En ese modo el puerto COM suele ser otro; míralo de nuevo. Más casos en [solucion-de-problemas.md](solucion-de-problemas.md).

## 5. Primer arranque

1. La pantalla dice **«Toca las cruces»**. Toca el centro de cada cruz con la punta de un lápiz, con firmeza: el táctil es resistivo. Se guarda; para repetirlo, escribe `cal` en el monitor serie (`pio device monitor`).
2. Aparece un aviso de Wi-Fi. Desde el móvil conéctate a la red abierta **`Pana-Setup`**. Si no se abre la página sola, ve a `192.168.4.1` → *Configure WiFi*.
3. Elige tu Wi-Fi de 2,4 GHz y su clave. En **Servidor** pon la IP de tu PC (`ipconfig`) y en **Puerto** `8770`. Guarda.
4. La placa se reinicia. Los dos puntos de arriba a la derecha, **Wi-Fi** y **Servidor**, se ponen en verde, y salen la hora y los datos del PC.

Para cambiar la IP del servidor más adelante, escribe en el monitor serie `host <ip>` y reinicia, o `wifi reset` para volver al portal.

## Cambiar los botones de apps

Edita `server/launcher.json` (máximo 4). Se lee cada 5 segundos, no hace falta reiniciar.

```json
[
  {"id": "youtube",   "label": "YouTube",   "url":  "https://www.youtube.com"},
  {"id": "navegador", "label": "Navegador", "url":  "https://www.google.com"},
  {"id": "codigo",    "label": "VS Code",   "ruta": "C:\\Users\\tu-usuario\\AppData\\Local\\Programs\\Microsoft VS Code\\Code.exe"}
]
```

`url` abre una página web (solo `http` o `https`); `ruta` abre un programa o una carpeta como con doble clic. La placa solo manda el `id`, el servidor busca en la lista y no ejecuta nada más. Aun así, cualquier dispositivo de tu red puede pulsar un botón de esa lista, así que no pongas nada que no quieras que se abra.

## Probar solo la pantalla

Si algo falla y no sabes si es el cableado, graba antes [`prueba-pantalla`](../prueba-pantalla/README.md): pinta colores y dibuja donde tocas, sin depender de Wi-Fi ni del servidor.
