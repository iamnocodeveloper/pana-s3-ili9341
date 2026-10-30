@echo off
rem Arranca el servidor de Pana (el cerebro) en este ordenador. Déjalo abierto mientras uses la placa.
rem Sin Hermes ni LLM: COMPANION_BRAIN=local da el reloj, los datos del PC y los botones.
cd /d "%~dp0server"
set COMPANION_BRAIN=local
title Pana · servidor
echo Servidor de Pana en http://localhost:8770  (cierra esta ventana para apagarlo)
"%~dp0.venv\Scripts\python.exe" main.py
pause
