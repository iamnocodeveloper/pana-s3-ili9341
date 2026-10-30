@echo off
rem Arranca el servidor de Pana (el cerebro) en este ordenador. Déjalo abierto mientras uses la placa.
rem El cerebro (hermes o local) lo decide COMPANION_BRAIN en el archivo .env de la raíz.
cd /d "%~dp0server"
set PYTHONUTF8=1
set PYTHONIOENCODING=utf-8
title Pana · servidor
echo Servidor de Pana en http://localhost:8770  (cierra esta ventana para apagarlo)
"%~dp0.venv\Scripts\python.exe" main.py
pause
