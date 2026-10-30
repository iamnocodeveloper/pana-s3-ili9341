"""Plano 1:1 de la caja de cartón duro de Pana (ESP32-S3 + pantalla ILI9341 de 3,2").

Saca caja/caja-plano.html: abre ese archivo en el navegador e imprime en A4 horizontal al 100 % (sin «ajustar a
la página»). Cada hoja trae una regla de 10 cm para comprobar que salió a escala. Recorta las piezas de papel,
calca sobre el cartón y corta.

Para otras medidas:  python generar_caja.py --pcb-w 87 --pcb-h 56 --ventana-w 62 --ventana-h 46 --fondo 50
Las opciones van en milímetros; el plano muestra las medidas en centímetros (el dibujo es 1:1).
"""
import argparse
import math
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument("--pcb-w", type=float, default=86, help="largo de la placa azul de la pantalla")
ap.add_argument("--pcb-h", type=float, default=55, help="ancho de la placa azul de la pantalla")
ap.add_argument("--ventana-w", type=float, default=62, help="largo de la ventana en el frente (algo menos que la zona iluminada)")
ap.add_argument("--ventana-h", type=float, default=46, help="alto de la ventana")
ap.add_argument("--ventana-dx", type=float, default=0, help="desplazar la ventana a la derecha (+) o izquierda (-)")
ap.add_argument("--ventana-dy", type=float, default=0, help="desplazar la ventana hacia abajo (+) o arriba (-)")
ap.add_argument("--centrada", action="store_true",
                help="ventana centrada en el frente (la caja se ensancha y por dentro la placa queda descentrada)")
ap.add_argument("--margen-vidrio", type=float, default=1,
                help="con --centrada: distancia del vidrio al borde de la placa por el lado sin pines")
ap.add_argument("--grosor", type=float, default=3, help="grosor del cartón")
ap.add_argument("--holgura", type=float, default=1.5, help="aire alrededor de la placa")
ap.add_argument("--fondo", type=float, default=50, help="profundidad interior: pantalla + placa + cables")
ap.add_argument("--usb-w", type=float, default=32, help="ancho de la ranura de los USB-C")
ap.add_argument("--usb-h", type=float, default=9, help="alto de la ranura de los USB-C")
ap.add_argument("--usb-x", type=float, default=0, help="posición de la ranura desde el centro de la pieza ABAJO (+ derecha)")
ap.add_argument("--altavoz", action="store_true",
                help="agujeros del altavoz de 40 mm en el FONDO y del micrófono en ARRIBA (usa --fondo 55)")
ap.add_argument("--altavoz-x", type=float, default=0, help="centro del altavoz desde la izquierda del FONDO (0 = a un cuarto del ancho)")
ap.add_argument("--sin-pata", action="store_true", help="no dibujar la pata para que se apoye inclinada")
a = ap.parse_args()

t = a.grosor
wi, hi = a.pcb_w + 2 * a.holgura, a.pcb_h + 2 * a.holgura   # interior
if a.centrada:
    # el vidrio (la ventana) en el centro y la placa pegada al lado sin pines: la placa sobresale por el de los pines,
    # así que el interior tiene que ser bastante más ancho que la propia placa
    a.ventana_dx = 0
    wi = max(wi, 2 * a.pcb_w - a.ventana_w - 2 * a.margen_vidrio + 2 * a.holgura)
wo, ho = wi + 2 * t, hi + 2 * t                              # exterior
d = a.fondo
fondo_ad, arriba_ad = [], []
if a.altavoz:
    ax = a.altavoz_x or wo / 4                       # el altavoz de 40 mm (mirando hacia fuera) pegado por dentro
    fondo_ad = [("guia", ax, ho / 2, 20, "ALTAVOZ Ø40 · pegar por dentro")]
    fondo_ad += [("circ", ax, ho / 2, 1.6, "")] + [("circ", ax + 12 * math.cos(k * math.pi / 4), ho / 2 + 12 * math.sin(k * math.pi / 4), 1.6, "") for k in range(8)]
    arriba_ad = [("circ", wo / 2, 10, 1.6, "MIC Ø3 · micrófono pegado por debajo")]    # a 10 mm del frente

# pieza: nombre, cuántas, ancho, alto, adornos (se dibujan dentro de la pieza)
piezas = [
    ("FRENTE", 1, wo, ho, [("rect", (wo - a.ventana_w) / 2 + a.ventana_dx, (ho - a.ventana_h) / 2 + a.ventana_dy,
                            a.ventana_w, a.ventana_h, "VENTANA")]),
    ("FONDO", 1, wo, ho, fondo_ad),
    ("ARRIBA", 1, wo, d, arriba_ad),
    ("ABAJO", 1, wo, d, [("rect", (wo - a.usb_w) / 2 + a.usb_x, d - a.usb_h - 0.0, a.usb_w, a.usb_h, "USB")]),
    ("LADO", 2, ho - 2 * t, d, []),
    ("TOPE", 4, 12, 12, []),
]
if not a.sin_pata:
    piezas.append(("PATA", 2, 60, 70, [("tri",)]))

M, G = 8, 5                       # margen y separación en la hoja
HOJA_W, HOJA_H = 297, 210


def colocar():
    items = []
    for nombre, n, w, h, ad in piezas:
        for i in range(n):
            items.append((nombre, i + 1, n, w, h, ad))
    items.sort(key=lambda p: -p[4])
    hojas, hoja, x, y, fila_h = [], [], M, M, 0
    for it in items:
        w, h = it[3], it[4]
        if x + w > HOJA_W - M:
            x, y, fila_h = M, y + fila_h + G, 0
        if y + h > HOJA_H - M - 14:             # 14 mm abajo para la regla
            hojas.append(hoja)
            hoja, x, y, fila_h = [], M, M, 0
        hoja.append((it, x, y))
        x += w + G
        fila_h = max(fila_h, h)
    if hoja:
        hojas.append(hoja)
    return hojas


def svg_hoja(num, total, hoja):
    s = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{HOJA_W}mm" height="{HOJA_H}mm" '
         f'viewBox="0 0 {HOJA_W} {HOJA_H}" font-family="Arial, sans-serif">']
    for (nombre, i, n, w, h, ad), x, y in hoja:
        s.append(f'<g transform="translate({x:.2f},{y:.2f})">')
        if ad and ad[0][0] == "tri":                       # pata: triángulo rectángulo
            s.append(f'<polygon points="0,{h} {w},{h} 0,0" fill="none" stroke="#000" stroke-width="0.4"/>')
            tx, ty = w * 0.22, h * 0.72
        else:
            s.append(f'<rect width="{w:.2f}" height="{h:.2f}" fill="none" stroke="#000" stroke-width="0.4"/>')
            tx, ty = w / 2, 5
            for kind, *p in ad:
                if kind == "rect":
                    rx, ry, rw, rh, et = p
                    s.append(f'<rect x="{rx:.2f}" y="{ry:.2f}" width="{rw:.2f}" height="{rh:.2f}" fill="#fde2e2" '
                             f'stroke="#d00" stroke-width="0.4"/>')
                    s.append(f'<text x="{rx + rw / 2:.2f}" y="{ry + rh / 2 + 1:.2f}" font-size="3" text-anchor="middle" '
                             f'fill="#d00">CORTAR {et} {cm(rw)}×{cm(rh)} cm</text>')
                elif kind == "circ":
                    cx, cy, r, et = p
                    s.append(f'<circle cx="{cx:.2f}" cy="{cy:.2f}" r="{r}" fill="#fde2e2" stroke="#d00" stroke-width="0.4"/>')
                    if et:
                        s.append(f'<text x="{cx:.2f}" y="{cy + 6:.2f}" font-size="2.4" text-anchor="middle" fill="#d00">{et}</text>')
                elif kind == "guia":
                    cx, cy, r, et = p
                    s.append(f'<circle cx="{cx:.2f}" cy="{cy:.2f}" r="{r}" fill="none" stroke="#06c" stroke-width="0.3" stroke-dasharray="1.5,1"/>')
                    s.append(f'<text x="{cx:.2f}" y="{cy + r + 4:.2f}" font-size="2.6" text-anchor="middle" fill="#06c">{et}</text>')
        et = f"{nombre}" + (f" ({i}/{n})" if n > 1 else "")
        anchor = "middle" if not (ad and ad[0][0] == "tri") else "start"
        if w >= 20:
            s.append(f'<text x="{tx:.2f}" y="{ty:.2f}" font-size="3.4" font-weight="bold" text-anchor="{anchor}">{et}</text>')
            s.append(f'<text x="{tx:.2f}" y="{ty + 3.6:.2f}" font-size="2.8" text-anchor="{anchor}" fill="#555">{cm(w)} × {cm(h)} cm</text>')
        else:
            s.append(f'<text x="{w / 2:.2f}" y="{h / 2 + 1:.2f}" font-size="2.4" text-anchor="middle" fill="#555">T</text>')
        s.append('</g>')
    ry = HOJA_H - 9
    s.append(f'<line x1="{M}" y1="{ry}" x2="{M + 100}" y2="{ry}" stroke="#000" stroke-width="0.4"/>')
    for k in range(0, 101, 10):
        s.append(f'<line x1="{M + k}" y1="{ry - 1.5}" x2="{M + k}" y2="{ry + 1.5}" stroke="#000" stroke-width="0.3"/>')
    s.append(f'<text x="{M + 104}" y="{ry + 1}" font-size="3">← esto debe medir 10 cm con regla · hoja {num}/{total}</text>')
    s.append('</svg>')
    return "\n".join(s)


def cm(v):
    """Milímetros → texto en centímetros, con coma decimal."""
    return f"{v / 10:.2f}".rstrip("0").rstrip(".").replace(".", ",")


hojas = colocar()
paginas = "\n".join(f'<section class="hoja">{svg_hoja(i + 1, len(hojas), h)}</section>' for i, h in enumerate(hojas))
instrucciones = f"""
<section class="info"><h1>Caja de Pana · cartón duro de {cm(t)} cm</h1>
<p><b>Imprime estas hojas en A4 horizontal al 100 %</b> (en el diálogo de impresión, «Escala: 100 %», no «Ajustar»).
Comprueba con una regla que la barra de cada hoja mide 10 cm.</p>
<h2>Medidas</h2>
<ul><li>Interior: {cm(wi)} × {cm(hi)} × {cm(d)} cm (placa {cm(a.pcb_w)} × {cm(a.pcb_h)} + {cm(a.holgura)} cm de aire por lado)</li>
<li>Exterior: {cm(wo)} × {cm(ho)} × {cm(d + 2 * t)} cm</li>
<li>Ventana: {cm(a.ventana_w)} × {cm(a.ventana_h)} cm ({'centrada' if a.centrada else 'desplazada ' + cm(a.ventana_dx) + ' cm'}; bordes izq/der {cm((wo - a.ventana_w) / 2 + a.ventana_dx)} / {cm((wo - a.ventana_w) / 2 - a.ventana_dx)} cm, arriba/abajo {cm((ho - a.ventana_h) / 2 + a.ventana_dy)} / {cm((ho - a.ventana_h) / 2 - a.ventana_dy)} cm)</li></ul>
<h2>Armado</h2>
<ol><li>Calca y corta todas las piezas. Las zonas rojas (VENTANA, USB y agujeros) también se cortan; el círculo azul solo guía dónde pegar el altavoz.</li>
<li>Pega la pantalla por detrás del <b>FRENTE</b> con la zona iluminada centrada en la ventana. Fíjala con 4 TOPES en las esquinas (cinta de doble cara o pegamento caliente).</li>
<li>Con todos los cables ya conectados y probados, sujeta el ESP32 al <b>FONDO</b> con pegamento caliente, con los USB-C hacia la ranura de <b>ABAJO</b>.</li>
<li>Pega <b>ARRIBA</b>, <b>ABAJO</b> y los dos <b>LADOS</b> entre el frente y el fondo (las uniones a tope se refuerzan con cinta por dentro).</li>
<li>Deja el <b>FONDO</b> pegado solo con cinta para poder abrir la caja. Las <b>PATAS</b> (opcional) van pegadas por detrás para que se apoye inclinada.</li></ol>
<p><b>Antes de cortar:</b> mide tu placa de pantalla y tu ESP32 y confirma o cambia las medidas con el script.</p>
</section>"""

html = f"""<!doctype html><html lang="es"><head><meta charset="utf-8"><title>Plano de la caja de Pana</title>
<style>
@page {{ size: 297mm 210mm; margin: 0 }}
body {{ margin: 0; background: #ddd; font-family: Arial, sans-serif }}
.hoja {{ width: 297mm; height: 210mm; margin: 6mm auto; background: #fff; page-break-after: always; overflow: hidden }}
.info {{ width: 297mm; min-height: 210mm; margin: 6mm auto; background: #fff; padding: 12mm; box-sizing: border-box; page-break-after: always }}
svg {{ display: block }}
@media print {{ body {{ background: #fff }} .hoja, .info {{ margin: 0 }} }}
</style></head><body>
{instrucciones}
{paginas}
</body></html>"""

out = Path(__file__).with_name("caja-plano-audio.html" if a.altavoz else "caja-plano.html")
out.write_text(html, encoding="utf-8")
print(f"{out} · {len(hojas)} hoja(s) · interior {wi:.1f}x{hi:.1f}x{d:g} · exterior {wo:.1f}x{ho:.1f}x{d + 2 * t:g}")
