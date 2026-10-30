// Composición apaisada de 320x240 (ESP32-S3 + ILI9341): el avatar con su cartel a la izquierda y, a la derecha,
// lo que haya que decir: confirmación, tarjeta, burbuja de Hermes o, en reposo, los datos de tu ordenador.
// Dos sprites en PSRAM: scr (la pantalla entera) y avs (el avatar, que se repinta solo donde cambia).
#ifdef BOARD_S3_ILI9341
#include <ArduinoJson.h>
#include "character/avatar.h"
#include "common.h"
#include "config.h"
#include "fonts/t43l.h"
#include "fonts/t43m.h"
#include "fonts/t43s.h"
#include "fonts/t43xs.h"
#include "model.h"
#include "net/net.h"
#include "ui.h"
#include <time.h>

static LGFX *lcd;
static LGFX_Sprite scr, avs;
static Avatar face;
static const uint32_t BG = 0x05080C, INK = 0xE8EEF2, MUTED = 0x8090A0, DIM = 0x3A4854, LINE = 0x1E2A34;
static const uint32_t AMBER = 0xFFB020, GREEN = 0x5CF08A, RED = 0xFF6A6A, TEAL = 0x40E0C0, SEG = 0x1A2630;

static const int AVW = 140;                       // el avatar ocupa 0..139 (132x161 con su cartel debajo)
static const int PX = 148, PW = SCR_W - PX - 6;   // el panel de la derecha
static const float LETTERS_PER_S = 40;

static const uint8_t *fontNow = nullptr;
static void useFont(const uint8_t *f) {
    if (fontNow == f) return;
    scr.unloadFont();
    scr.loadFont(f);
    fontNow = f;
}

struct Box { int x, y, w, h; };
static bool inside(Box b, int x, int y) { return x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h; }
static Box optBox(int i, int n) { return {PX, SCR_H - (n - i) * 38, PW, 34}; }   // botones apilados al pie
// botones de apps en reposo: de dos en dos al pie del panel (una fila de 40 px o dos de 30)
static Box appBox(int i, int n) {
    int rows = (n + 1) / 2, h = rows == 1 ? 40 : 30, w = (PW - 6) / 2;
    return {PX + (i % 2) * (w + 6), SCR_H - 2 - (rows - i / 2) * h - (rows - 1 - i / 2) * 4, w, h};
}
static int pressedApp = -1;                       // el botón que se acaba de tocar, resaltado un momento
static uint32_t pressedAt = 0;

static void button(Box b, const String &label, uint32_t bg, uint32_t fg) {
    scr.fillRoundRect(b.x, b.y, b.w, b.h, 6, bg);
    useFont(t43xs);
    scr.setTextColor(fg, bg);
    scr.setTextDatum(textdatum_t::middle_center);
    scr.drawString(ellipsize(scr, label, b.w - 10), b.x + b.w / 2, b.y + b.h / 2);
    scr.setTextDatum(textdatum_t::top_left);
}

// --- estado ----------------------------------------------------------------------------------
static uint32_t drawnVersion = UINT32_MAX, drawnDash = UINT32_MAX, lastPanel = 0, tBubble = 0;
static uint8_t drawnSig = 0xFF;
static int drawnClock = -2;
static int drawnRevealed = -1;
static String animBubble;
static float revealed = 0;
static bool spActive = false;
static int labelState = -1;
static bool forceAvatar = true;

// --- avatar ----------------------------------------------------------------------------------
static void drawLabel() {
    useFont(t43s);
    scr.fillRect(0, 178, AVW, SCR_H - 178, BG);
    scr.setTextColor(INK, BG);
    scr.setTextDatum(textdatum_t::middle_center);
    scr.drawString(ellipsize(scr, PACK_ANIMS[face.current()].label, AVW - 8), AVW / 2, 208);
    scr.setTextDatum(textdatum_t::top_left);
    labelState = face.current();
}

static void drawAvatar() {
    if (forceAvatar) { face.redrawAll(); forceAvatar = false; }
    if (!face.dirty() && labelState == face.current()) return;
    bool moved = face.dirty();
    if (moved) face.draw(avs, 0, SCR_H);
    avs.pushSprite(&scr, 0, 0);
    drawLabel();
    lcd->flush(scr, 0, 0, AVW, SCR_H);
}

// --- panel de la derecha -----------------------------------------------------------------------
static void drawStatus(Model &m) {
    useFont(t43xs);
    scr.setTextDatum(textdatum_t::top_left);
    scr.fillCircle(PX + 4, 9, 4, m.wifi ? GREEN : RED);
    scr.setTextColor(MUTED, BG);
    scr.drawString("Wi-Fi", PX + 12, 1);
    scr.fillCircle(PX + 70, 9, 4, m.server ? GREEN : (m.wifi ? AMBER : DIM));
    scr.drawString("Servidor", PX + 78, 1);
}

static uint32_t loadColor(float pct) { return pct < 60 ? GREEN : pct < 85 ? AMBER : RED; }

static void meter(const char *name, float pct, const String &right, int y) {
    useFont(t43xs);
    scr.setTextColor(INK, BG);
    scr.setTextDatum(textdatum_t::top_left);
    scr.drawString(name, PX, y);
    scr.setTextColor(MUTED, BG);
    scr.setTextDatum(textdatum_t::top_right);
    scr.drawString(right, PX + PW, y);
    scr.setTextDatum(textdatum_t::top_left);
    scr.fillRoundRect(PX, y + 22, PW, 8, 3, SEG);
    int w = (int)(PW * constrain(pct, 0.0f, 100.0f) / 100);
    if (w > 0) scr.fillRoundRect(PX, y + 22, max(w, 6), 8, 3, loadColor(pct));
}

// la hora llega del servidor con cada ping; antes de eso el reloj marca --:--
static bool clockNow(struct tm &tm) {
    time_t now = time(nullptr);
    if (now < 1700000000) return false;
    localtime_r(&now, &tm);
    return true;
}

static int clockKey() {                           // cambia cada minuto (y al llegar la primera hora)
    struct tm tm;
    return clockNow(tm) ? tm.tm_hour * 60 + tm.tm_min : -1;
}

static void drawClock() {
    static const char *dias[] = {"dom", "lun", "mar", "mié", "jue", "vie", "sáb"};
    static const char *meses[] = {"ene", "feb", "mar", "abr", "may", "jun", "jul", "ago", "sep", "oct", "nov", "dic"};
    struct tm tm;
    bool ok = clockNow(tm);
    char b[24];
    snprintf(b, sizeof(b), ok ? "%02d:%02d" : "--:--", tm.tm_hour, tm.tm_min);
    useFont(t43l);
    scr.setTextColor(INK, BG);
    scr.setTextDatum(textdatum_t::top_left);
    scr.drawString(b, PX, 22);
    if (!ok) return;
    useFont(t43xs);
    scr.setTextColor(MUTED, BG);
    scr.setTextDatum(textdatum_t::top_right);
    scr.drawString(dias[tm.tm_wday], PX + PW, 24);
    snprintf(b, sizeof(b), "%d %s", tm.tm_mday, meses[tm.tm_mon]);
    scr.drawString(b, PX + PW, 44);
    scr.setTextDatum(textdatum_t::top_left);
}

static void drawIdle(Model &m) {
    const Dash &d = m.dash;
    drawClock();
    if (!m.server || !d.have) {
        useFont(t43xs);
        scr.setTextColor(MUTED, BG);
        static String ls[4];
        int n = wrap(scr, !m.wifi ? "Sin Wi-Fi. Conéctate a la red Pana-Setup desde el móvil." :
                                    !m.server ? "Buscando el servidor…" : "Esperando datos de tu ordenador…", PW, ls, 4);
        for (int i = 0; i < n; i++) scr.drawString(ls[i], PX, 76 + i * 21);
        return;
    }
    char b[28];
    snprintf(b, sizeof(b), "%.0f %%", d.cpu);
    meter("CPU", d.cpu, b, 68);
    snprintf(b, sizeof(b), "%.1f / %.0f GB", d.ramGb, d.ramTotal);
    meter("RAM", d.ram, b, 108);
    int y = 148;
    const int limit = d.nApps ? appBox(0, d.nApps).y - 4 : SCR_H;    // las filas de datos acaban donde empiezan los botones
    useFont(t43xs);
    scr.setTextDatum(textdatum_t::top_left);
    auto row = [&](const String &s, uint32_t color) {
        if (y + 18 > limit) return;
        scr.setTextColor(color, BG);
        scr.drawString(s, PX, y);
        y += 21;
    };
    if (d.wins >= 0) {
        snprintf(b, sizeof(b), "Navegador: %d %s", d.wins, d.wins == 1 ? "ventana" : "ventanas");
        row(b, INK);
    }
    if (d.temp >= 0) {
        snprintf(b, sizeof(b), "Temp  %.0f °C", d.temp);
        row(b, MUTED);
    }
    if (d.gpu >= 0) {
        snprintf(b, sizeof(b), "GPU  %.0f %%", d.gpu);
        row(b, MUTED);
    }
    if (d.up.length()) row("Encendido " + d.up, MUTED);
    for (int i = 0; i < d.nApps; i++) {
        bool on = i == pressedApp;
        button(appBox(i, d.nApps), d.appLabel[i], on ? TEAL : SEG, on ? BG : INK);
    }
}

static void drawBubble(Model &m) {
    useFont(t43xs);
    scr.setTextColor(INK, BG);
    scr.setTextDatum(textdatum_t::top_left);
    static String ls[40];
    int n = wrap(scr, utf8Prefix(m.bubbleText, (int)revealed), PW, ls, 40);
    const int lh = scr.fontHeight(), fit = (SCR_H - 30) / lh;
    int from = max(0, n - fit);                               // al ir escribiendo, se queda con el final
    for (int i = from; i < n; i++) scr.drawString(ls[i], PX, 26 + (i - from) * lh);
}

static void drawCard(Model &m) {
    int btnTop = m.nActions ? optBox(0, m.nActions).y - 6 : SCR_H;
    useFont(t43s);
    scr.setTextColor(kindColor(m.cardKind), BG);
    scr.setTextDatum(textdatum_t::top_left);
    scr.drawString(ellipsize(scr, m.cardTitle, PW), PX, 26);
    useFont(t43xs);
    scr.setTextColor(INK, BG);
    const int lh = scr.fontHeight();
    int y = 54;
    for (int i = 0; i < m.nLines && y + lh <= btnTop; i++) {
        static String ls[3];
        int n = wrap(scr, m.cardLines[i], PW, ls, 3);
        for (int j = 0; j < n && y + lh <= btnTop; j++, y += lh) scr.drawString(ls[j], PX, y);
    }
    for (int i = 0; i < m.nActions; i++)
        button(optBox(i, m.nActions), m.cardActions[i].label, i == 0 ? TEAL : SEG, i == 0 ? BG : INK);
}

static void drawConfirm(Model &m, uint32_t now) {
    int n = max(1, m.nOptions);
    int btnTop = optBox(0, n).y - 8;
    useFont(t43xs);
    scr.setTextDatum(textdatum_t::top_left);
    const int lh = scr.fontHeight();
    int rows = (btnTop - 26) / lh;
    static String qs[6], ps[6];
    int nq = wrap(scr, m.question, PW, qs, 6);
    int np = m.preview.length() ? wrap(scr, m.preview, PW, ps, 6) : 0;
    int qShow = min(nq, max(1, rows - (np ? 1 : 0))), pShow = min(np, max(0, rows - qShow));
    scr.setTextColor(INK, BG);
    for (int i = 0; i < qShow; i++) scr.drawString(qs[i], PX, 26 + i * lh);
    scr.setTextColor(MUTED, BG);
    for (int i = 0; i < pShow; i++) scr.drawString(ps[i], PX, 26 + (qShow + i) * lh);
    // la cuenta atrás de la confirmación
    int left = m.confirmUntil > now ? m.confirmUntil - now : 0;
    scr.fillRect(PX, btnTop + 2, PW, 3, SEG);
    scr.fillRect(PX, btnTop + 2, (int)((uint64_t)PW * left / max<uint32_t>(m.confirmTotal, 1)), 3, AMBER);
    for (int i = 0; i < m.nOptions; i++) button(optBox(i, n), m.options[i], optionColor(m.options[i]), 0xFFFFFFu);
}

static void drawPanel(Model &m, uint32_t now) {
    scr.fillRect(PX - 4, 0, SCR_W - PX + 4, SCR_H, BG);
    scr.drawFastVLine(PX - 5, 6, SCR_H - 12, LINE);
    drawStatus(m);
    if (m.confirm) drawConfirm(m, now);
    else if (m.card) drawCard(m);
    else if (m.bubble) drawBubble(m);
    else drawIdle(m);
    lcd->flush(scr, PX - 5, 0, SCR_W - PX + 5, SCR_H);
    lastPanel = now;
}

// --- aviso a pantalla (Wi-Fi, OTA) ---------------------------------------------------------------
void uiSplash(const char *l1, const char *l2) {
    face.disconnected = true;
    face.step(33, millis());
    drawAvatar();
    scr.fillRect(PX - 4, 0, SCR_W - PX + 4, SCR_H, BG);
    scr.drawFastVLine(PX - 5, 6, SCR_H - 12, LINE);
    useFont(t43s);
    scr.setTextColor(TEAL, BG);
    scr.setTextDatum(textdatum_t::top_left);
    scr.drawString(ellipsize(scr, l1, PW), PX, 26);
    useFont(t43xs);
    scr.setTextColor(MUTED, BG);
    static String ls[8];
    int n = wrap(scr, l2, PW, ls, 8);
    for (int i = 0; i < n; i++) scr.drawString(ls[i], PX, 58 + i * scr.fontHeight());
    lcd->flush(scr, PX - 5, 0, SCR_W - PX + 5, SCR_H);
    spActive = true;
    drawnVersion = UINT32_MAX;
}

// --- toques ---------------------------------------------------------------------------------
static void send(JsonDocument &d) {
    String s;
    serializeJson(d, s);
    netSend(s);
}

static void sendTouch(const char *target, int x, int y, const char *gesture) {
    JsonDocument d;
    d["type"] = "touch"; d["target"] = target; d["x"] = x; d["y"] = y; d["gesture"] = gesture;
    send(d);
}

static struct { bool down = false; int x = 0, y = 0; uint32_t t0 = 0; } tch;

static void handleTouch(Model &m, uint32_t now) {
    lgfx::touch_point_t tp;
    if (lcd->getTouch(&tp)) {
        if (!tch.down) { tch.down = true; tch.t0 = now; }
        tch.x = tp.x; tch.y = tp.y;
        return;
    }
    if (!tch.down) return;
    tch.down = false;
    int x = tch.x, y = tch.y;
    const char *gesture = now - tch.t0 > 600 ? "long_press" : "tap";
    if (m.confirm) {
        for (int i = 0; i < m.nOptions; i++)
            if (inside(optBox(i, m.nOptions), x, y)) {
                JsonDocument d;
                d["type"] = "confirm_reply"; d["id"] = m.confirmId; d["choice"] = m.options[i];
                send(d);
                return;
            }
        return;
    }
    if (m.card)
        for (int i = 0; i < m.nActions; i++)
            if (inside(optBox(i, m.nActions), x, y)) {
                String t = "card:" + m.cardId + ":" + m.cardActions[i].id;
                sendTouch(t.c_str(), x, y, "tap");
                return;
            }
    if (!m.bubble)                                   // en reposo, los botones de apps
        for (int i = 0; i < m.dash.nApps; i++)
            if (inside(appBox(i, m.dash.nApps), x, y)) {
                pressedApp = i;
                pressedAt = now;
                drawnVersion = UINT32_MAX;               // repintar ya: el botón se ilumina
                String t = "app:" + m.dash.appId[i];
                sendTouch(t.c_str(), x, y, "tap");
                return;
            }
    if (x < AVW) {                                   // la cara saluda
        face.play(S_WAVE, 1);
        sendTouch("face", x, y, gesture);
    } else if (m.bubble) {                           // tocar la burbuja la cierra
        m.bubble = false; m.version++;
        sendTouch("bubble", x, y, gesture);
    }
}

// --- bucle ------------------------------------------------------------------------------------
void uiBegin(LGFX &l) {
    lcd = &l;
    scr.setPsram(true);
    scr.setColorDepth(16);
    avs.setPsram(true);
    avs.setColorDepth(16);
    if (!scr.createSprite(SCR_W, SCR_H) || !avs.createSprite(AVW, SCR_H)) Serial.println("sin memoria para los sprites");
    scr.fillScreen(BG);
    avs.fillScreen(BG);
    lcd->touchSetup(false);
    lcd->fillScreen(BG);
}

void uiFrame(uint32_t now) {
    static uint32_t last = now;
    static bool wasServer = false;
    uint32_t dt = now - last;
    modelLock();
    Model &m = model;
    handleTouch(m, now);
    face.listening = false;
    face.confirming = m.confirm;
    face.set(m.emotion.c_str(), m.animation.c_str());
    m.lookNew = false;
    m.animation = "";
    face.disconnected = !m.server;
    if (m.server && !wasServer) face.play(S_WAVE, 2);
    wasServer = m.server;
    face.step(dt, now);
    last = now;

    if (m.splash1.length()) {
        String a = m.splash1, b = m.splash2;
        modelUnlock();
        uiSplash(a.c_str(), b.c_str());
        return;
    }
    if (spActive) { spActive = false; drawnVersion = UINT32_MAX; forceAvatar = true; }

    if (m.bubble && m.bubbleId != animBubble) { animBubble = m.bubbleId; revealed = 0; tBubble = now; }
    if (m.bubble) revealed += dt / 1000.0f * LETTERS_PER_S;
    if (m.bubble && now - tBubble > 30000) { m.bubble = false; m.version++; }     // se quita sola
    if (m.confirm && now > m.confirmUntil) { m.confirm = false; m.version++; }

    if (pressedApp >= 0 && now - pressedAt > 400) { pressedApp = -1; drawnVersion = UINT32_MAX; }

    drawAvatar();

    uint8_t sig = (m.wifi ? 1 : 0) | (m.server ? 2 : 0);
    int shown = m.bubble ? min((int)revealed, utf8Len(m.bubbleText)) : -1;
    bool idle = !m.confirm && !m.card && !m.bubble;
    int clk = clockKey();
    bool due = m.version != drawnVersion || m.dash.version != drawnDash || sig != drawnSig ||
               (m.bubble && shown != drawnRevealed) || (m.confirm && now - lastPanel > 250) ||
               (idle && clk != drawnClock);
    if (due) {
        drawnVersion = m.version;
        drawnDash = m.dash.version;
        drawnSig = sig;
        drawnRevealed = shown;
        drawnClock = clk;
        drawPanel(m, now);
    }
    modelUnlock();
}

void uiKey(char c) {
    if (c == 'R') { forceAvatar = true; drawnVersion = UINT32_MAX; lcd->fillScreen(BG); }   // repintar todo (tras calibrar)
}

void uiDump() {
    // el sprite guarda big-endian, como espera tools/shot.py
    Serial.printf("SHOT %d %d\n", SCR_W, SCR_H);
    const uint8_t *p = (const uint8_t *)scr.getBuffer();
    for (int y = 0; y < SCR_H; y++) Serial.write(p + y * SCR_W * 2, SCR_W * 2);
    Serial.flush();
}
#endif
