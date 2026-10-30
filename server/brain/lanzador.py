"""Botones de la placa para abrir apps en este ordenador.

Solo se abre lo que está en launcher.json (hasta 4 entradas): cualquier dispositivo de la red puede hablar con el
servidor, así que la placa nunca manda una ruta ni una orden, solo el id de un botón de esa lista. Cada entrada
lleva «id», «label» y una de estas dos:
  · "url":  una página web (http o https), se abre en el navegador por defecto
  · "ruta": un programa, documento o carpeta de este ordenador, se abre como con doble clic
"""
import json
import logging
import os
import subprocess
import webbrowser
from pathlib import Path

log = logging.getLogger("lanzador")
ARCHIVO = Path(__file__).resolve().parent.parent / "launcher.json"
MAX_BOTONES = 4


def _leer() -> list[dict]:
    try:
        datos = json.loads(ARCHIVO.read_text(encoding="utf-8"))
    except (OSError, ValueError) as e:
        log.warning("launcher.json: %s", e)
        return []
    if not isinstance(datos, list):
        return []
    ok = [d for d in datos if isinstance(d, dict) and isinstance(d.get("id"), str) and isinstance(d.get("label"), str)
          and (isinstance(d.get("url"), str) or isinstance(d.get("ruta"), str))]
    return ok[:MAX_BOTONES]


def lista() -> list[list[str]]:
    """Lo que ve la placa: [[id, etiqueta], …]"""
    return [[d["id"], d["label"]] for d in _leer()]


def abrir(id: str) -> bool:
    for d in _leer():
        if d["id"] != id:
            continue
        if d.get("url"):
            if not d["url"].startswith(("http://", "https://")):
                log.warning("%s: solo se abren direcciones http o https", id)
                return False
            webbrowser.open(d["url"])
        elif hasattr(os, "startfile"):
            os.startfile(d["ruta"])                    # Windows
        else:
            subprocess.Popen(["xdg-open", d["ruta"]])  # Linux, sin shell
        log.info("abrí «%s»", d["label"])
        return True
    log.warning("botón desconocido: %r", id)
    return False
