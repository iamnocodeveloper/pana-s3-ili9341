@echo off
rem Túnel SSH a Hermes: el puerto 8642 de este PC llega a la API de Hermes en la VPS (que no está expuesta a internet).
rem Lee usuario@ip de %USERPROFILE%\.config\pana\vps.txt y usa la llave %USERPROFILE%\.ssh\pana_vps. Se reconecta solo.
title Pana · túnel a Hermes
set /p VPS=<"%USERPROFILE%\.config\pana\vps.txt"
:otra
ssh -N -L 127.0.0.1:8642:127.0.0.1:8642 -i "%USERPROFILE%\.ssh\pana_vps" -o BatchMode=yes -o ExitOnForwardFailure=yes -o ServerAliveInterval=30 %VPS%
timeout /t 5 /nobreak >nul
goto otra
