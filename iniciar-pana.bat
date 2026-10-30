@echo off
rem Arranca el servidor de Pana (el cerebro) en este ordenador. Déjalo abierto mientras uses la placa.
rem Con Hermes en una VPS: si existe %USERPROFILE%\.config\pana\vps.txt (usuario@ip), abre el túnel y usa Hermes.
rem Sin eso: COMPANION_BRAIN=local da el reloj, los datos del PC y los botones.
cd /d "%~dp0server"
set PYTHONUTF8=1
set PYTHONIOENCODING=utf-8
set COMPANION_BRAIN=local
if exist "%USERPROFILE%\.config\pana\vps.txt" (
  set COMPANION_BRAIN=hermes
  start "Pana tunel" /min cmd /c "%~dp0tunel-hermes.bat"
  timeout /t 3 /nobreak >nul
)
title Pana · servidor
echo Servidor de Pana en http://localhost:8770 (cerebro: %COMPANION_BRAIN%)  (cierra esta ventana para apagarlo)
"%~dp0.venv\Scripts\python.exe" main.py
pause
