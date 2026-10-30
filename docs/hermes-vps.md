# Hermes Agent en una VPS, con el servidor de Pana en tu PC

El servidor de Pana se queda en tu PC con Windows (lee tu CPU, tu RAM y tus ventanas, y abre tus apps). **Hermes Agent** corre aparte, en una VPS Linux pequeña (4 vCPU y 10 GB de RAM sobran), y tu PC habla con él por un túnel SSH. La API de Hermes no se expone a internet.

```
placa ──Wi-Fi──▶ servidor de Pana (tu PC) ──túnel SSH──▶ API de Hermes (VPS, solo 127.0.0.1:8642) ──▶ OpenRouter
```

## En la VPS (Ubuntu 24.04)

1. Entra solo con **llave SSH** y desactiva la contraseña (`PasswordAuthentication no` en un archivo de `/etc/ssh/sshd_config.d/`). Activa `ufw` dejando solo `OpenSSH`.
2. Crea un usuario normal para Hermes (`adduser --disabled-password hermes`), copia tu `authorized_keys` y activa `loginctl enable-linger hermes`.
3. Como ese usuario: `curl -fsSL https://hermes-agent.nousresearch.com/install.sh | bash -s -- --non-interactive --skip-browser`. Antes de instalar, instala `libatomic1` como administrador.
4. Elige proveedor y modelo con `hermes model` (con OpenRouter, pega tu clave ahí; no la pongas en chats ni en el repositorio).
5. Activa la API en `~/.hermes/.env`: `API_SERVER_ENABLED=true`, `API_SERVER_HOST=127.0.0.1` y un `API_SERVER_KEY` aleatorio (`openssl rand -hex 32`). Después, `hermes gateway install --if-missing`.

Si quieres que Hermes administre la VPS, añade ese usuario al grupo `sudo` **con contraseña**, y haz antes un snapshot.

## En tu PC

1. Genera la llave: `ssh-keygen -t ed25519 -f %USERPROFILE%\.ssh\pana_vps` y copia la `.pub` a la VPS.
2. Guarda la clave de la API de Hermes en `%USERPROFILE%\.config\pana\hermes-api-key` (la que pusiste en `API_SERVER_KEY`).
3. Crea `%USERPROFILE%\.config\pana\vps.txt` con una línea `usuario@ip-de-tu-vps`. Si este archivo existe, `iniciar-pana.bat` abre el túnel (`tunel-hermes.bat`) y arranca con `COMPANION_BRAIN=hermes`; si no, usa el modo local.
4. Doble clic en `iniciar-pana.bat`. Prueba la conversación sin voz:

```powershell
$b = [Text.Encoding]::UTF8.GetBytes('{"text":"Hola, ¿quién eres?"}')
Invoke-RestMethod -Uri http://localhost:8770/api/chat -Method Post -ContentType "application/json; charset=utf-8" -Body $b
```

También puedes escribirle desde el simulador en <http://localhost:8770>.

## Cosas a tener en cuenta

- `PYTHONUTF8=1` (ya está en el `.bat`): sin él, Windows falla al escribir los `«»` del registro.
- Lo que Hermes ejecute como herramienta se ejecuta **en la VPS**, no en tu PC.
- Aún no hay voz: faltan Whisper, Piper y el micrófono y el altavoz de la placa ([audio.md](audio.md)). Hoy se le escribe por texto.
- Tailscale es una alternativa al túnel SSH si prefieres una red privada permanente.
