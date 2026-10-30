// Pantalla y táctil del ESP32-S3 con el módulo ILI9341 de 3,2" (ver board.h).
#ifdef BOARD_S3_ILI9341
#include <Arduino.h>
#include <Preferences.h>
#include "board.h"

void LGFX::touchSetup(bool force) {
    Preferences p;
    p.begin("touch", false);
    uint16_t cal[8];
    if (!force && p.getBytesLength("cal") == sizeof(cal) && p.getBytes("cal", cal, sizeof(cal)) == sizeof(cal)) {
        setTouchCalibrate(cal);
        p.end();
        return;
    }
    // el XPT2046 da valores en bruto: se calibra tocando las cuatro esquinas
    fillScreen(TFT_BLACK);
    setTextColor(TFT_WHITE, TFT_BLACK);
    setTextDatum(lgfx::textdatum_t::middle_center);
    setTextSize(2);
    drawString("Toca las cruces", width() / 2, height() / 2);
    calibrateTouch(cal, TFT_WHITE, TFT_BLACK, 20);
    p.putBytes("cal", cal, sizeof(cal));
    p.end();
    Serial.print("calibración táctil guardada:");
    for (int i = 0; i < 8; i++) Serial.printf(" %u", cal[i]);
    Serial.println();
}

void LGFX::flush(LGFX_Sprite &s, int x, int y, int w, int h) {
    int x1 = min(width(), x + w), y1 = min(height(), y + h);
    x = max(0, x);
    y = max(0, y);
    if (x1 <= x || y1 <= y) return;
    const uint16_t *src = (const uint16_t *)s.getBuffer();   // el sprite guarda big-endian
    const int sw = s.width();
    startWrite();
    if (x == 0 && x1 == sw) {
        pushImage(0, y, sw, y1 - y, (const lgfx::swap565_t *)(src + y * sw));
    } else {
        for (int yy = y; yy < y1; yy++) pushImage(x, yy, x1 - x, 1, (const lgfx::swap565_t *)(src + yy * sw + x));
    }
    endWrite();
}
#endif
