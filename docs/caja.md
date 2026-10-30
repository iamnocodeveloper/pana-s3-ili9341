# La caja de cartón duro

`caja/generar_caja.py` crea un plano **a escala 1:1** de una caja hecha de cartón duro (de unos 3 mm). Saca `caja/caja-plano.html`, de tres hojas A4 horizontales: las instrucciones y dos de piezas.

## Cómo usarlo

```powershell
.venv\Scripts\python caja\generar_caja.py --pcb-w 90 --pcb-h 57 --ventana-w 78 --ventana-h 55 --centrada
```

Abre `caja\caja-plano.html` en el navegador e imprime en A4 horizontal al **100 %** (sin «ajustar a la página»). Cada hoja tiene una barra de 100 mm: comprueba con una regla que mide eso. Después recorta las piezas de papel, calca sobre el cartón, corta, y pega.

## Medidas de este montaje

| Pieza | Medida |
|---|---|
| Placa de la pantalla, con bordes | 90 × 57 mm |
| Vidrio de la pantalla | 78 × 55 mm |
| Interior de la caja | 103 × 60 × 50 mm |
| **Tapa frontal** | **109 × 66 mm**, con una ventana de 78 × 55 mm centrada |
| Bordes de la tapa frontal | 15,5 mm a izquierda y derecha, 5,5 mm arriba y abajo |
| Exterior | 109 × 66 × 56 mm |

Con `--centrada` la ventana queda en el centro del frente. Como la placa de 90 mm lleva la tira de pines en un extremo, el vidrio no está centrado sobre ella: queda más cerca de un borde. Para que la ventana quede centrada, la placa se desplaza dentro de la caja y el interior se ensancha unos 10 mm. Por dentro queda desigual, y ese hueco sirve para los cables. Sin `--centrada`, la caja es más estrecha (99 mm) y la ventana queda descentrada.

## Piezas

`FRENTE` (con la ventana), `FONDO`, `ARRIBA`, `ABAJO` (con la ranura de los USB-C), dos `LADO`, 4 `TOPE` pequeños para sujetar la pantalla y, opcional, dos `PATA` para que se apoye inclinada (`--sin-pata` las quita).

## Opciones del script

| Opción | Qué hace | Por defecto |
|---|---|---|
| `--pcb-w`, `--pcb-h` | Largo y ancho de la placa azul | 86 × 55 |
| `--ventana-w`, `--ventana-h` | Tamaño de la ventana | 62 × 46 |
| `--centrada` | Ventana centrada en el frente | no |
| `--margen-vidrio` | Con `--centrada`: distancia del vidrio al borde de la placa, sin pines | 1 |
| `--ventana-dx`, `--ventana-dy` | Desplaza la ventana | 0 |
| `--grosor` | Grosor del cartón | 3 |
| `--holgura` | Aire alrededor de la placa | 1,5 |
| `--fondo` | Profundidad interior | 50 |
| `--usb-w`, `--usb-h`, `--usb-x` | Ranura de los USB-C | 32 × 9, centrada |
| `--sin-pata` | No dibuja las patas | |

## Pendiente de verificar

- **La ventana de 78 × 55 mm enseña el vidrio entero**, con su borde negro. Los píxeles que se iluminan ocupan solo unos 65 × 49 mm en el centro. Si prefieres que el cartón tape ese borde, usa `--ventana-w 63 --ventana-h 47`.
- **El margen del vidrio de 1 mm** es un supuesto. Haz una prueba con papel: imprime el frente, recorta la ventana y colócalo sobre la pantalla encendida. Los bordes del vidrio deben quedar cubiertos por igual.
- **La posición de la ranura de los USB-C** depende de dónde pongas el ESP32. Mide tu placa y la distancia entre sus dos USB-C, y ajusta `--usb-w` y `--usb-x`.
- **La profundidad de 50 mm** deja sitio para los cables hembra. Si más adelante añades altavoz, hará falta más.
