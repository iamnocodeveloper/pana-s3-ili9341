"""Avatar a partir de UNA imagen de personaje (pixel art 1254x1254, de frente): quita el fondo y saca
los 16 estados. El cuerpo es la imagen; cada estado cambia ojos y boca y rehace los brazos: la taza
sube a la boca al beber, la mano vacía saluda, la laptop se abre en las manos para escribir, etc.
Son PNG de 120x160 para tools/make_avatar.py.

Uso: python -X utf8 tools/avatar_imagen.py avatar/fuente/avatar.webp avatar/manuel
     python -X utf8 tools/make_avatar.py avatar/manuel
"""
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
from demo_avatar import ACCENT, FRAMES, H, W, Escalado, accesorios  # noqa: E402

N = 1254
FEET = (615, 1214)                      # centro de los pies en la imagen
ESC = 0.100                             # imagen → cuadro de 120x160 (el personaje mide ~118 px de alto)
PIEL, PIEL_S, PIEL_L, PIEL_B = (222, 150, 100), (186, 112, 72), (242, 178, 130), (92, 50, 34)
ABRIGO, ABRIGO_B, ABRIGO_L = (46, 30, 32), (22, 13, 15), (74, 50, 50)
LABIO_OSC = (70, 22, 28)
OJOS = {"izq": (508, 322), "der": (690, 306)}      # centro de cada ojo en la imagen
BOCA = (604, 464)
HOMBRO = {"izq": (478, 655), "der": (884, 650)}
TAZA_MANO = (330, 730)                  # mano que sostiene la taza (centro)
# la mano con la taza y el vapor; la manga del abrigo (x 400-500, y 540-640) se queda en el cuerpo
CHUNK_IZQ = [(256, 640), (300, 640), (300, 500), (400, 500), (400, 640), (500, 640), (504, 700), (498, 832), (256, 832)]
CHUNK_DER = [(676, 650), (905, 570), (990, 620), (990, 965), (676, 965)]


def quitar_fondo(im):
    """El fondo es casi negro azulado: se rellena desde los bordes y se cierran los huecos del abrigo."""
    w = im.copy()
    for pt in [(2, 2), (1250, 2), (2, 1250), (1250, 1250), (640, 1240), (100, 640), (1150, 640), (640, 5),
               (300, 1100), (1000, 1100)]:
        ImageDraw.floodfill(w, pt, (255, 0, 255), thresh=16)
    a = np.asarray(w)
    bg = (a[..., 0] == 255) & (a[..., 1] == 0) & (a[..., 2] == 255)
    al = Image.fromarray(((~bg) * 255).astype(np.uint8))
    al = al.filter(ImageFilter.MaxFilter(25)).filter(ImageFilter.MinFilter(25)).filter(ImageFilter.MedianFilter(5))
    out = im.convert("RGBA")
    out.putalpha(al)
    return out


def afin(im, dx=0.0, dy=0.0, sh=0.0, sx=1.0, sy=1.0):
    """Inclinación (sh), escala (sx, sy) alrededor de los pies y desplazamiento."""
    fx, fy = FEET
    T0 = np.array([[1, 0, -fx], [0, 1, -fy], [0, 0, 1]], float)
    Sh = np.array([[1, -sh, 0], [0, 1, 0], [0, 0, 1]], float)
    Sc = np.array([[sx, 0, 0], [0, sy, 0], [0, 0, 1]], float)
    T1 = np.array([[1, 0, fx + dx], [0, 1, fy + dy], [0, 0, 1]], float)
    Mi = np.linalg.inv(T1 @ Sc @ Sh @ T0)
    return im.transform(im.size, Image.AFFINE, tuple(Mi[:2].flatten()), resample=Image.BILINEAR)


def liso(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3 - 2 * t)


# ---------- cara ----------
def _tapar(d, cx, cy, medio=False):
    h = 12 if medio else 31
    d.ellipse([cx - 47, cy - h + (14 if medio else 0), cx + 47, cy + 31], fill=PIEL)
    if medio:
        d.arc([cx - 46, cy - 6, cx + 46, cy + 30], 200, 340, fill=(40, 24, 24), width=8)


def ojos(im, st, k, cerrar=False):
    d = ImageDraw.Draw(im)
    parpadeo = st in ("idle", "listening", "talking", "wave") and k % 14 == 13
    for lado, (cx, cy) in OJOS.items():
        if st == "sleeping" or parpadeo or cerrar:
            _tapar(d, cx, cy)
            d.arc([cx - 46, cy - 22, cx + 46, cy + 22], 20, 160, fill=(40, 24, 24), width=9)
        elif st in ("success", "wave"):
            _tapar(d, cx, cy)
            d.arc([cx - 44, cy - 10, cx + 44, cy + 44], 205, 335, fill=(40, 24, 24), width=10)
        elif st == "error":
            _tapar(d, cx, cy)
            d.line([(cx - 28, cy - 24), (cx + 28, cy + 24)], fill=(235, 70, 70), width=11)
            d.line([(cx + 28, cy - 24), (cx - 28, cy + 24)], fill=(235, 70, 70), width=11)
        elif st == "confused" and lado == "der":
            _tapar(d, cx, cy, medio=True)


def boca(im, st, k):
    d = ImageDraw.Draw(im)
    cx, cy = BOCA
    if st == "talking":
        d.ellipse([cx - 78, cy - 24, cx + 78, cy + 30], fill=PIEL)
        a = [8, 26, 16, 32, 22, 8, 26, 16][k % 8]
        d.ellipse([cx - 36, cy - a * 0.4, cx + 36, cy + a], fill=LABIO_OSC)
        d.rectangle([cx - 26, cy - a * 0.4, cx + 26, cy - a * 0.4 + 8], fill=(244, 238, 228))
    elif st in ("success", "wave"):
        d.ellipse([cx - 80, cy - 24, cx + 80, cy + 34], fill=PIEL)
        d.chord([cx - 62, cy - 26, cx + 62, cy + 40], 0, 180, fill=LABIO_OSC)
        d.rectangle([cx - 50, cy - 8, cx + 50, cy + 2], fill=(244, 238, 228))
    elif st in ("error", "confused"):
        d.ellipse([cx - 78, cy - 24, cx + 78, cy + 30], fill=PIEL)
        d.line([(cx - 44, cy + 12), (cx - 20, cy), (cx + 8, cy + 14), (cx + 44, cy + 2)], fill=(150, 62, 60), width=9)
    elif st == "sleeping":
        d.ellipse([cx - 78, cy - 24, cx + 78, cy + 30], fill=PIEL)
        d.line([(cx - 30, cy + 6), (cx + 30, cy + 6)], fill=(150, 62, 60), width=9)


# ---------- brazos, manos y objetos ----------
def _dibujar_con_borde(layer, mask, relleno, borde, grosor=7, sombra=None, luz=None):
    n = grosor * 2 + 1
    layer.paste(Image.new("RGBA", layer.size, borde + (255,)), (0, 0), mask.filter(ImageFilter.MaxFilter(n)))
    layer.paste(Image.new("RGBA", layer.size, relleno + (255,)), (0, 0), mask)
    if sombra:
        reg = ImageChops.subtract(mask, ImageChops.offset(mask, -14, -14)).filter(ImageFilter.GaussianBlur(5))
        layer.paste(Image.new("RGBA", layer.size, sombra + (255,)), (0, 0),
                    ImageChops.multiply(reg.point(lambda v: min(255, v * 2)), mask))
    if luz:
        reg = ImageChops.subtract(mask, ImageChops.offset(mask, 14, 14)).filter(ImageFilter.GaussianBlur(5))
        layer.paste(Image.new("RGBA", layer.size, luz + (255,)), (0, 0),
                    ImageChops.multiply(reg.point(lambda v: min(255, int(v * 1.3))), mask))


_MANOS = {}


def mano(tipo):
    """Mano de 260x300 con la muñeca abajo en el centro (130, 285), mirando hacia arriba."""
    if tipo in _MANOS:
        return _MANOS[tipo]
    L = Image.new("RGBA", (260, 300), (0, 0, 0, 0))
    m = Image.new("L", L.size, 0)
    d = ImageDraw.Draw(m)
    if tipo == "abierta":
        d.ellipse([66, 130, 194, 270], fill=255)
        for x, y0 in ((72, 56), (102, 32), (132, 26), (162, 50)):
            d.rounded_rectangle([x - 15, y0, x + 15, 170], 15, fill=255)
        d.ellipse([30, 150, 92, 232], fill=255)
    elif tipo == "puno":
        d.ellipse([62, 110, 198, 262], fill=255)
        d.ellipse([34, 120, 100, 200], fill=255)
    elif tipo == "pulgar":
        d.ellipse([62, 130, 198, 270], fill=255)
        d.rounded_rectangle([96, 36, 140, 180], 22, fill=255)
    else:                                                    # "agarra": mano cerrada pequeña
        d.ellipse([70, 160, 190, 274], fill=255)
    _dibujar_con_borde(L, m, PIEL, PIEL_B, 6, PIEL_S, PIEL_L)
    dd = ImageDraw.Draw(L)
    if tipo == "abierta":
        for x in (88, 118, 148):
            dd.line([(x, 120), (x, 168)], fill=PIEL_S, width=4)
    elif tipo in ("puno", "pulgar"):
        for y in (176, 204, 232):
            dd.line([(120, y), (176, y)], fill=PIEL_S, width=4)
    _MANOS[tipo] = L
    return L


def manga(im, p0, p1, lado, ancho=104):
    """Brazo de abrigo de p0 (hombro) a p1 (muñeca), con un codo ligeramente doblado."""
    d = ImageDraw.Draw(im)
    mx, my = (p0[0] + p1[0]) / 2, (p0[1] + p1[1]) / 2
    sal = -1 if lado == "izq" else 1
    codo = (mx + sal * 34, my + 60)
    pts = [p0, codo, p1]
    for col, w in ((ABRIGO_B, ancho + 16), (ABRIGO, ancho)):
        d.line(pts, fill=col, width=w, joint="curve")
        for x, y in (p0, p1):
            d.ellipse([x - w / 2, y - w / 2, x + w / 2, y + w / 2], fill=col)
    d.line([(p0[0] - 26, p0[1] + 4), (codo[0] - 26, codo[1]), (p1[0] - 22, p1[1])], fill=ABRIGO_L, width=18, joint="curve")


def poner_mano(im, tipo, muneca, rot=0.0, espejo=False):
    h = mano(tipo)
    if espejo:
        h = h.transpose(Image.FLIP_LEFT_RIGHT)
    h = h.rotate(rot, center=(130, 285), resample=Image.BILINEAR)
    im.alpha_composite(h, (round(muneca[0] - 130), round(muneca[1] - 285)))


def borrar(im, poly):
    mk = Image.new("L", im.size, 0)
    ImageDraw.Draw(mk).polygon(poly, fill=255)
    mk = ImageChops.multiply(mk, im.getchannel("A"))
    trozo = Image.new("RGBA", im.size, (0, 0, 0, 0))
    trozo.paste(im, (0, 0), mk)
    im.paste(Image.new("RGBA", im.size, (0, 0, 0, 0)), (0, 0), mk)
    return trozo


def brazo_con_mano(im, lado, tipo, muneca, rot=0.0):
    manga(im, HOMBRO[lado], (muneca[0], muneca[1] + 20), lado)
    poner_mano(im, tipo, muneca, rot, espejo=(lado == "der"))


def taza_en_mano(im, trozo, dx, dy, rot):
    """Vuelve a poner la mano con la taza (la de la imagen) desplazada y girada; la manga la une al hombro."""
    cx, cy = TAZA_MANO[0] + dx, TAZA_MANO[1] + dy
    manga(im, HOMBRO["izq"], (cx + 30, cy - 10), "izq", ancho=84 if dy < -100 else 104)
    t = ImageChops.offset(trozo, round(dx), round(dy))
    t = t.rotate(rot, center=(round(cx + 50), round(cy)), resample=Image.BILINEAR)
    im.alpha_composite(t)


_FUENTE = {}


def fuente(tam):
    if tam not in _FUENTE:
        try:
            _FUENTE[tam] = ImageFont.truetype("consola.ttf", tam)
        except OSError:
            _FUENTE[tam] = ImageFont.load_default(tam)
    return _FUENTE[tam]


def portatil_abierta(im, st, k, n):
    """Laptop abierta delante del pecho, sostenida con las dos manos, con la pantalla de cada estado."""
    d = ImageDraw.Draw(im)
    t = k / n
    base = [(448, 868), (782, 868), (816, 926), (414, 926)]
    d.polygon(base, fill=(174, 176, 186), outline=(30, 32, 40), width=7)
    d.polygon([(448, 868), (782, 868), (790, 884), (440, 884)], fill=(210, 212, 220))
    d.rounded_rectangle([474, 650, 756, 868], 14, fill=(52, 54, 62), outline=(24, 26, 32), width=7)
    d.rectangle([494, 670, 736, 846], fill=(14, 22, 32))
    x0, y0 = 506, 684
    if st == "typing":
        cols = [(92, 240, 138), (122, 168, 255), (255, 176, 32), (230, 230, 240)]
        for i in range(7):
            ln = 40 + ((i + int(t * 7)) * 53) % 130
            ind = 22 * ((i + int(t * 7)) % 3 == 1)
            d.rounded_rectangle([x0 + ind, y0 + i * 22, x0 + ind + ln, y0 + i * 22 + 11], 4,
                                fill=cols[(i + int(t * 7)) % 4])
        if (k // 2) % 2:
            d.rectangle([x0 + 150, y0 + 6 * 22, x0 + 162, y0 + 6 * 22 + 14], fill=(240, 240, 250))
    elif st == "executing":
        d.text((x0, y0), ">_ run", font=fuente(34), fill=(92, 240, 138))
        for i in range(1, 6):
            ln = 60 + ((i * 47 + int(t * 6) * 31) % 120)
            d.rounded_rectangle([x0, y0 + 20 + i * 24, x0 + ln, y0 + 31 + i * 24], 4, fill=(60, 140, 100))
        if (k // 2) % 2:
            d.rectangle([x0 + 190, y0 + 4, x0 + 206, y0 + 36], fill=(92, 240, 138))
    elif st == "working":
        cx, cy = 615, 750
        a0 = t * 360
        d.arc([cx - 52, cy - 52, cx + 52, cy + 52], a0, a0 + 250, fill=(255, 176, 32), width=14)
        d.rounded_rectangle([508, 818, 722, 832], 6, fill=(40, 52, 66))
        d.rounded_rectangle([508, 818, 508 + round(214 * t), 832], 6, fill=(255, 176, 32))
    elif st == "reading":
        for i in range(7):
            ln = 210 if i % 3 else 150
            col = (240, 236, 220) if i != int(t * 7) else (255, 215, 128)
            d.rounded_rectangle([x0, y0 + i * 22, x0 + ln, y0 + i * 22 + 11], 4, fill=col)
    elif st == "searching":
        d.rounded_rectangle([x0, y0, 724, y0 + 34], 10, fill=(50, 62, 80))
        d.ellipse([x0 + 8, y0 + 5, x0 + 32, y0 + 29], outline=(255, 176, 32), width=5)
        d.line([(x0 + 28, y0 + 26), (x0 + 40, y0 + 36)], fill=(255, 176, 32), width=5)
        for i in range(1, 5):
            ln = 120 + ((i * 61) % 90)
            d.rounded_rectangle([x0, y0 + 40 + i * 28, x0 + ln, y0 + 51 + i * 28], 4,
                                fill=(255, 215, 128) if i == 1 + int(t * 4) % 4 else (122, 168, 255))
    # manos sujetando la base; en "typing" los dedos suben y bajan
    ba = 8 * math.sin(t * 2 * math.pi * 3) if st == "typing" else 0
    for x, y, sg in ((428, 902 + ba, -1), (802, 902 - ba, 1)):
        d.ellipse([x - 58, y - 48, x + 58, y + 48], fill=PIEL, outline=PIEL_B, width=7)
        d.ellipse([x - 40 + sg * 6, y - 34, x + 6 + sg * 6, y - 4], fill=PIEL_L)


# ---------- poses ----------
ABIERTA_ESTADOS = ("typing", "executing", "working", "reading", "searching")


def parametros(st, k, n):
    """Movimiento del cuerpo entero en este cuadro y pose de cada brazo."""
    t = k / n
    s = math.sin(t * 2 * math.pi)
    p = dict(dx=0.0, dy=0.0, sh=0.0, sx=1.0, sy=1.0, izq=("taza", 0, 0, 0), der="laptop", cerrar=False)
    if st == "idle":
        p["dy"] = 4 * s
        sorbo = liso((k - 6) / 5) - liso((k - 14) / 5)          # sube la taza a la boca y vuelve a bajar
        if sorbo > 0.02:
            p["izq"] = ("taza", 150 * sorbo, -240 * sorbo, -16 * sorbo)
            p["cerrar"] = sorbo > 0.8
    elif st == "listening":
        p.update(dy=3 * s, sh=0.018 + 0.006 * s, izq=("mano", "abierta", (310, 650), 4 + 4 * s))
    elif st == "thinking":
        p.update(sh=0.045 + 0.01 * s, dy=2 * s, izq=("mano", "puno", (470 + 6 * s, 690), -38))
    elif st == "talking":
        p.update(dy=-4 * abs(math.sin(t * 4 * math.pi)), izq=("taza", 0, 18 * math.sin(t * 4 * math.pi), 0))
    elif st == "success":
        h = abs(math.sin(t * 2 * math.pi))
        p.update(dy=-30 * h, sy=1 + 0.02 * h - 0.03 * (1 - h), sx=1 - 0.01 * h,
                 izq=("mano", "abierta", (330, 560 - 20 * h), -16), der=("mano", "abierta", (902, 556 - 20 * h), -16))
    elif st == "wave":
        p.update(dy=3 * s, izq=("mano", "abierta", (292, 570), 10 + 20 * math.sin(t * 4 * math.pi)))
    elif st == "confused":
        p.update(sh=-0.04 * (0.6 + 0.4 * s), dy=2 * s, izq=("mano", "abierta", (350, 470), -40 + 8 * math.sin(t * 6 * math.pi)))
    elif st in ABIERTA_ESTADOS:
        p.update(dy=2 * s, izq=None, der=None, sh=0.012 if st == "searching" else 0.0)
    elif st == "approval":
        p.update(dy=2 * s, izq=("mano", "pulgar", (330, 640 + 10 * s), -6))
    elif st == "error":
        p.update(dx=10 * math.sin(t * 10 * math.pi) * (1 - t), izq=("mano", "abierta", (350, 620), -30),
                 der=("mano", "abierta", (880, 620), -30))
    elif st == "reconnecting":
        p.update(dy=3 * s)
    elif st == "sleeping":
        p.update(sh=0.07 + 0.008 * s, dy=6 + 6 * s)
    return p


def frame(base, st, k, n):
    p = parametros(st, k, n)
    im = base.copy()
    izq, der = p["izq"], p["der"]
    taza = None
    if izq is None or izq[0] != "taza" or izq[1:] != (0, 0, 0):
        taza = borrar(im, CHUNK_IZQ)
    if der != "laptop":
        borrar(im, CHUNK_DER)
    ojos(im, st, k, p["cerrar"])
    if not (izq and izq[0] == "taza" and izq[2] < -120):
        boca(im, st, k)
    # brazo izquierdo (el de la taza)
    if izq and izq[0] == "taza" and taza is not None:
        taza_en_mano(im, taza, izq[1], izq[2], izq[3])
    elif izq and izq[0] == "mano":
        brazo_con_mano(im, "izq", izq[1], izq[2], izq[3])
    elif izq is None and st not in ABIERTA_ESTADOS:
        brazo_con_mano(im, "izq", "agarra", (330, 830), 0)
    # brazo derecho
    if isinstance(der, tuple):
        brazo_con_mano(im, "der", der[1], der[2], der[3])
    elif der is None and st not in ABIERTA_ESTADOS:
        brazo_con_mano(im, "der", "agarra", (900, 830), 0)
    if st in ABIERTA_ESTADOS:
        manga(im, HOMBRO["izq"], (428, 884), "izq")
        manga(im, HOMBRO["der"], (802, 884), "der")
        portatil_abierta(im, st, k, n)
    im = afin(im, p["dx"], p["dy"], p["sh"], p["sx"], p["sy"])
    lado = round(N * ESC)
    peq = im.resize((lado, lado), Image.LANCZOS)
    out = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    out.alpha_composite(peq, (round(60 - FEET[0] * ESC), round(150 - FEET[1] * ESC)))
    if st not in ("reading", "typing", "executing", "working", "searching"):
        ac = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        accesorios(Escalado(ImageDraw.Draw(ac), 0.88), st, k, n, k / n, round(p["dy"] * ESC), ACCENT[st])
        out.alpha_composite(ac)
    elif st in ("working", "searching"):
        ac = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        accesorios(Escalado(ImageDraw.Draw(ac), 0.88), st, k, n, k / n, round(p["dy"] * ESC), ACCENT[st])
        if st == "searching":
            ac = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        out.alpha_composite(ac)
    return out


def main():
    origen = Path(sys.argv[1])
    out = Path(sys.argv[2] if len(sys.argv) > 2 else "avatar/manuel")
    out.mkdir(parents=True, exist_ok=True)
    base = quitar_fondo(Image.open(origen).convert("RGB"))
    for st, n in FRAMES.items():
        for k in range(n):
            frame(base, st, k, n).save(out / f"{st}-{k:03d}.png")
    print(f"{sum(FRAMES.values())} cuadros en {out}")


if __name__ == "__main__":
    main()
