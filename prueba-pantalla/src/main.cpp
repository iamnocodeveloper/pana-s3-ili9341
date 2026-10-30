// Prueba de pantalla ILI9341 240x320 por bus paralelo de 16 bits + táctil XPT2046 en ESP32-S3.
// Primero recorre colores, luego dibuja donde tocas y muestra las coordenadas.
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9341 _panel;
    lgfx::Bus_Parallel16 _bus;
    lgfx::Light_PWM _light;
    lgfx::Touch_XPT2046 _touch;

public:
    LGFX() {
        {
            auto b = _bus.config();
            b.freq_write = 10000000;    // bajo a propósito: los cables de puente largos no aguantan más
            b.pin_wr = 1;
            b.pin_rd = -1;      // RD va fijo a 3V3: no se lee la pantalla
            b.pin_rs = 2;       // RS = DC
            b.pin_d0 = 4;  b.pin_d1 = 5;  b.pin_d2 = 6;  b.pin_d3 = 7;
            b.pin_d4 = 15; b.pin_d5 = 16; b.pin_d6 = 17; b.pin_d7 = 18;
            b.pin_d8 = 8;  b.pin_d9 = 9;  b.pin_d10 = 10; b.pin_d11 = 11;
            b.pin_d12 = 12; b.pin_d13 = 13; b.pin_d14 = 21; b.pin_d15 = 47;
            _bus.config(b);
            _panel.setBus(&_bus);
        }
        {
            auto p = _panel.config();
            p.pin_cs = -1;      // CS va fijo a GND
            p.pin_rst = 42;
            p.pin_busy = -1;
            p.panel_width = 240;
            p.panel_height = 320;
            p.offset_x = 0;
            p.offset_y = 0;
            p.readable = false;
            p.invert = false;
            p.rgb_order = false;
            p.dlen_16bit = true;    // 16 bits por transferencia
            p.bus_shared = false;
            _panel.config(p);
        }
        {
            auto l = _light.config();
            l.pin_bl = 41;
            l.invert = false;
            l.freq = 5000;
            l.pwm_channel = 7;
            _light.config(l);
            _panel.setLight(&_light);
        }
        {
            auto t = _touch.config();
            t.x_min = 0;   t.x_max = 239;
            t.y_min = 0;   t.y_max = 319;
            t.pin_int = 39;         // PEN
            t.bus_shared = false;
            t.offset_rotation = 0;
            t.spi_host = SPI3_HOST;
            t.freq = 1000000;
            t.pin_sclk = 36;
            t.pin_mosi = 35;
            t.pin_miso = 37;
            t.pin_cs = 40;
            _touch.config(t);
            _panel.setTouch(&_touch);
        }
        setPanel(&_panel);
    }
};

LGFX tft;

void pantallaDeColor(uint32_t color, const char *nombre) {
    tft.fillScreen(color);
    tft.setTextColor(TFT_BLACK, color);
    tft.setTextDatum(middle_center);
    tft.setTextSize(3);
    tft.drawString(nombre, tft.width() / 2, tft.height() / 2);
    Serial.printf("color: %s\n", nombre);
    delay(900);
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\nPrueba de pantalla ILI9341 + XPT2046");

    tft.init();
    tft.setRotation(0);
    tft.setBrightness(255);
    Serial.printf("pantalla %dx%d\n", tft.width(), tft.height());

    pantallaDeColor(TFT_RED, "ROJO");
    pantallaDeColor(TFT_GREEN, "VERDE");
    pantallaDeColor(TFT_BLUE, "AZUL");
    pantallaDeColor(TFT_WHITE, "BLANCO");

    // Calibración del XPT2046 de esta pantalla, medida tocando las cuatro esquinas.
    // Para repetirla: tft.calibrateTouch(cal, TFT_WHITE, TFT_BLACK, 20) e imprimir cal.
    static uint16_t cal[8] = {306, 433, 432, 3756, 3695, 509, 3693, 3850};
    tft.setTouchCalibrate(cal);

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(top_left);
    tft.setTextSize(2);
    tft.drawString("Toca la pantalla", 10, 10);
    tft.drawRect(0, 0, tft.width(), tft.height(), TFT_CYAN);
}

void loop() {
    static uint32_t ultimo = 0;
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
        tft.fillCircle(x, y, 3, TFT_YELLOW);
        if (millis() - ultimo > 150) {
            ultimo = millis();
            tft.fillRect(10, 40, 220, 20, TFT_BLACK);
            tft.setCursor(10, 40);
            tft.printf("x=%u y=%u", x, y);
            Serial.printf("toque x=%u y=%u\n", x, y);
        }
    }
    delay(5);
}
