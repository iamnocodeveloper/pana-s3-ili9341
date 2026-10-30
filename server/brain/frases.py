"""La frase y el versículo de la home de la placa.

Hermes escribe cada pocas horas una frase corta (programación, constancia, ánimo) y un versículo de la Biblia en la
versión NVI con su cita. Se guardan en data/frases.json para que sobrevivan a un reinicio. Si Hermes no está
disponible (COMPANION_BRAIN=local, sin conexión…) sale una frase de la lista de abajo y ningún versículo: el texto
bíblico solo lo escribe Hermes y siempre va con su cita para que se pueda comprobar.
"""
import asyncio
import json
import logging
import random
import re
import time
from pathlib import Path

from config import cfg

log = logging.getLogger("frases")
ARCHIVO = Path(__file__).resolve().parent.parent / "data" / "frases.json"
CADA_S = 6 * 3600                  # una frase nueva cada 6 horas
MAX_FRASE, MAX_VERSO = 90, 110     # caben unas 6 líneas de 18 letras en la pantalla

_RESPALDO = [
    "Un error es solo un dato más: léelo, corrígelo y sigue.",
    "Lo simple que funciona gana a lo genial que no termina.",
    "Hoy, un commit pequeño vale más que un plan perfecto.",
    "Si el código te confunde hoy, mañana te lo agradecerá quien lo lea.",
    "Primero hazlo funcionar, luego bonito, luego rápido.",
    "La constancia compila lo que el talento solo escribe.",
    "Descansa un poco: los mejores bugs se resuelven caminando.",
    "Cada línea que escribes hoy es práctica para la de mañana.",
    "Pide ayuda a tiempo: ahorra horas y se nota en el resultado.",
    "No necesitas ver toda la escalera, solo el siguiente escalón.",
]

_estado = {"t": "", "v": "", "c": "", "ts": 0.0, "previas": []}
_tarea: asyncio.Task | None = None


def _cargar():
    try:
        d = json.loads(ARCHIVO.read_text(encoding="utf-8"))
        _estado.update({k: d.get(k, _estado[k]) for k in ("t", "v", "c", "ts", "previas")})
    except Exception:
        pass


def _guardar():
    try:
        ARCHIVO.parent.mkdir(parents=True, exist_ok=True)
        ARCHIVO.write_text(json.dumps(_estado, ensure_ascii=False), encoding="utf-8")
    except Exception as e:
        log.warning("no se pudo guardar: %s", e)


_cargar()


def actual() -> dict:
    """Lo que ve la placa: {t: frase, v: versículo, c: cita}."""
    if not _estado["t"]:
        _estado["t"] = random.choice(_RESPALDO)
    return {"t": _estado["t"], "v": _estado["v"], "c": _estado["c"]}


def _prompt() -> str:
    previas = "; ".join(_estado["previas"][-6:]) or "ninguna"
    return (
        "Responde SOLO con un objeto JSON, sin markdown ni explicaciones, con exactamente estas claves:\n"
        f'"frase": una frase motivadora nueva y original sobre programar, la constancia o aprender, en español, '
        f"de {MAX_FRASE} caracteres como máximo, sin emojis ni comillas dentro;\n"
        f'"versiculo": un versículo de la Biblia en la versión NVI (Nueva Versión Internacional), copiado palabra por '
        f"palabra como aparece en la NVI, de {MAX_VERSO} caracteres como máximo. Elige uno que conozcas con certeza; "
        f"si no estás seguro del texto exacto de la NVI, elige otro;\n"
        f'"cita": el libro, capítulo y versículo, por ejemplo "Josué 1:9".\n'
        f"No repitas estas frases ya usadas: {previas}."
    )


async def _preguntar() -> dict | None:
    from brain import hermes
    cuerpo = {"model": "hermes-agent", "messages": [{"role": "user", "content": _prompt()}]}
    r = await hermes._http().post(f"{cfg.hermes_url}/chat/completions", json=cuerpo,
                                  headers={"X-Hermes-Session-Id": "pana-frases"})
    r.raise_for_status()
    texto = r.json()["choices"][0]["message"]["content"]
    m = re.search(r"\{.*\}", texto, re.S)
    if not m:
        return None
    d = json.loads(m.group(0))
    frase = str(d.get("frase", "")).strip().strip('"')
    verso = str(d.get("versiculo", "")).strip().strip('"“”«»')
    cita = str(d.get("cita", "")).strip()
    if not frase or len(frase) > MAX_FRASE + 15:
        return None
    if not verso or not cita or len(verso) > MAX_VERSO + 20:          # sin cita no se muestra versículo
        verso, cita = "", ""
    return {"t": frase, "v": verso, "c": cita}


async def _refrescar():
    try:
        nuevo = await _preguntar() if cfg.brain == "hermes" else None
    except Exception as e:
        log.warning("Hermes no pudo escribir la frase: %s", e)
        nuevo = None
    if nuevo is None:
        nuevo = {"t": random.choice([f for f in _RESPALDO if f != _estado["t"]] or _RESPALDO), "v": "", "c": ""}
    _estado.update(nuevo)
    _estado["ts"] = time.time()
    _estado["previas"] = (_estado["previas"] + [nuevo["t"]])[-12:]
    _guardar()
    log.info("frase: «%s» · %s", nuevo["t"], nuevo["c"] or "sin versículo")


def tick():
    """Llamar a menudo: si toca una frase nueva y no hay otra petición en curso, la pide en segundo plano."""
    global _tarea
    if (_tarea and not _tarea.done()) or time.time() - _estado["ts"] < CADA_S:
        return
    _tarea = asyncio.create_task(_refrescar())
