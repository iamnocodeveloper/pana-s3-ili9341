// Composición apaisada de 320x240 (ESP32-S3 + ILI9341): el avatar con su cartel a la izquierda y, a la derecha,
// lo que haya que decir: confirmación, tarjeta, burbuja de Hermes o, en reposo, páginas que se cambian
// deslizando el dedo: principal, Apps, Mi PC, Hoy (agenda y redes) y Hermes.
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
enum Page : uint8_t { PG_MAIN, PG_APPS, PG_PC, PG_DAY, PG_HERMES, NPG };
static int page = PG_MAIN;
static uint32_t pageAt = 0;                       // último toque o cambio de página (a los 60 s vuelve a la principal)
static const int MAIN_APPS = 4;                   // botones de apps en la página principal
static float cpuHist[40];                         // la CPU de los últimos ~3 minutos (un dato cada 5 s)
static int nHist = 0;
static uint32_t histVersion = 0;
static String lastReply, lastTools[3];            // para la página de Hermes
static int nLastTools = 0;
// rejilla de la página de Apps: dos columnas, hasta cuatro filas
static Box gridBox(int i) {
    int w = (PW - 6) / 2, h = 40;
    return {PX + (i % 2) * (w + 6), 44 + (i / 2) * (h + 4), w, h};
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
static void drawStatus(Model &m, bool dots) {
    useFont(t43xs);
    scr.setTextDatum(textdatum_t::top_left);
    scr.fillCircle(PX + 4, 9, 4, m.wifi ? GREEN : RED);
    scr.setTextColor(MUTED, BG);
    scr.drawString("Wi-Fi", PX + 12, 1);
    scr.fillCircle(PX + 62, 9, 4, m.server ? GREEN : (m.wifi ? AMBER : DIM));
    scr.drawString("Servidor", PX + 70, 1);
    if (dots)                                     // en qué página estás
        for (int i = 0; i < NPG; i++) scr.fillCircle(PX + PW - NPG * 8 + i * 8 + 4, 9, i == page ? 3 : 2, i == page ? INK : DIM);
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
    const int na = min(d.nApps, MAIN_APPS);
    const int limit = na ? appBox(0, na).y - 4 : SCR_H;    // las filas de datos acaban donde empiezan los botones
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
    for (int i = 0; i < na; i++) {
        bool on = i == pressedApp;
        button(appBox(i, na), d.appLabel[i], on ? TEAL : SEG, on ? BG : INK);
    }
}

// --- las otras páginas -------------------------------------------------------------------------------
static void pageTitle(const String &t) {
    useFont(t43xs);
    scr.setTextColor(TEAL, BG);
    scr.setTextDatum(textdatum_t::top_left);
    scr.drawString(ellipsize(scr, t, PW - 50), PX, 22);
}

static void drawApps(Model &m) {
    const Dash &d = m.dash;
    pageTitle("Apps");
    useFont(t43xs);
    scr.setTextColor(MUTED, BG);
    if (!d.nApps) {
        static String ls[5];
        int n = wrap(scr, "Sin apps. Añádelas en server/launcher.json (hasta 8).", PW, ls, 5);
        for (int i = 0; i < n; i++) scr.drawString(ls[i], PX, 48 + i * 21);
        return;
    }
    for (int i = 0; i < d.nApps; i++) {
        bool on = i == pressedApp;
        button(gridBox(i), d.appLabel[i], on ? TEAL : SEG, on ? BG : INK);
    }
}

static void drawPC(Model &m) {
    const Dash &d = m.dash;
    pageTitle(d.host.length() ? "Mi PC · " + d.host : String("Mi PC"));
    if (!d.have) { useFont(t43xs); scr.setTextColor(MUTED, BG); scr.drawString("Esperando datos…", PX, 48); return; }
    char b[32];
    snprintf(b, sizeof(b), "%.0f %%", d.cpu);
    meter("CPU", d.cpu, b, 42);
    snprintf(b, sizeof(b), "%.1f / %.0f GB", d.ramGb, d.ramTotal);
    meter("RAM", d.ram, b, 78);
    const int gy = 114, gh = 40;                                       // gráfica de la CPU
    scr.fillRoundRect(PX, gy, PW, gh, 4, SEG);
    for (int i = 1; i < nHist; i++) {
        int x0 = PX + (i - 1) * (PW - 1) / 39, x1 = PX + i * (PW - 1) / 39;
        int y0 = gy + gh - 3 - (int)(cpuHist[i - 1] * (gh - 6) / 100), y1 = gy + gh - 3 - (int)(cpuHist[i] * (gh - 6) / 100);
        scr.drawLine(x0, y0, x1, y1, loadColor(cpuHist[i]));
    }
    useFont(t43xs);
    scr.setTextDatum(textdatum_t::top_left);
    int y = 160;
    auto row = [&](const String &s, uint32_t c) { if (y + 18 > SCR_H) return; scr.setTextColor(c, BG); scr.drawString(ellipsize(scr, s, PW), PX, y); y += 21; };
    if (d.wins >= 0) { snprintf(b, sizeof(b), "Navegador: %d %s", d.wins, d.wins == 1 ? "ventana" : "ventanas"); row(b, INK); }
    String t;
    if (d.temp >= 0) t += String("Temp ") + (int)d.temp + " °C  ";
    if (d.gpu >= 0) t += String("GPU ") + (int)d.gpu + " %";
    if (t.length()) row(t, MUTED);
    if (d.up.length()) row("Encendido " + d.up, MUTED);
}

static String miles(long v) {
    if (v >= 1000000) return String(v / 1000000.0, 1) + " M";
    if (v >= 10000) return String(v / 1000) + " mil";
    return String(v);
}

static void drawDay(Model &m) {
    const Dash &d = m.dash;
    pageTitle("Hoy");
    useFont(t43xs);
    scr.setTextDatum(textdatum_t::top_left);
    int y = 44;
    if (!d.agendaOk) {
        scr.setTextColor(MUTED, BG);
        static String ls[5];
        int n = wrap(scr, d.agendaWhy.length() ? d.agendaWhy : String("Sin agenda. Pon AGENDA_ICS_URL en el .env del servidor."), PW, ls, 5);
        for (int i = 0; i < n; i++) scr.drawString(ls[i], PX, y + i * 21);
    } else if (!d.nAgenda) {
        scr.setTextColor(MUTED, BG);
        scr.drawString("Nada en la agenda", PX, y);
    } else {
        for (int i = 0; i < min(d.nAgenda, 5); i++) {
            scr.setTextColor(d.agenda[i].now ? AMBER : MUTED, BG);
            scr.drawString(d.agenda[i].h, PX, y);
            scr.setTextColor(d.agenda[i].now ? INK : MUTED, BG);
            scr.drawString(ellipsize(scr, d.agenda[i].title, PW - 46), PX + 46, y);
            y += 21;
        }
        if (d.agendaLeft > 0) { scr.setTextColor(DIM, BG); scr.drawString(String("+ ") + d.agendaLeft + " más", PX, y); }
    }
    int by = SCR_H - 2 - 21 * ((d.yt ? 1 : 0) + (d.ig ? 1 : 0));
    if (d.yt) { scr.setTextColor(INK, BG); scr.drawString("YouTube " + miles(d.ytSubs), PX, by); by += 21; }
    if (d.ig) { scr.setTextColor(INK, BG); scr.drawString("Instagram " + miles(d.igFollowers), PX, by); }
}

static void drawHermes(Model &m) {
    const Dash &d = m.dash;
    pageTitle("Hermes");
    useFont(t43xs);
    scr.setTextDatum(textdatum_t::top_left);
    int y = 44;
    scr.setTextColor(INK, BG);
    scr.drawString("Cerebro: " + (d.brain.length() ? d.brain : String("?")), PX, y);
    y += 21;
    for (int i = 0; i < d.nSvc && y < 150; i++) {                       // Hermes, Whisper… vivos o no
        if (d.brain == "hermes" && (d.svcName[i] == "LLM" || d.svcName[i] == "Rápido")) continue;   // no se usan con Hermes
        scr.fillCircle(PX + 4, y + 9, 4, d.svcOk[i] ? GREEN : RED);
        scr.setTextColor(MUTED, BG);
        scr.drawString(ellipsize(scr, d.svcName[i], PW - 16), PX + 14, y);
        y += 21;
    }
    scr.setTextColor(TEAL, BG);
    scr.drawString(nLastTools ? "Última tarea" : "Última respuesta", PX, y + 2);
    y += 23;
    scr.setTextColor(INK, BG);
    static String ls[3];
    String txt = nLastTools ? lastTools[0] : (lastReply.length() ? lastReply : String("Aún nada. Escríbele en localhost:8770."));
    int n = wrap(scr, txt, PW, ls, 3);
    for (int i = 0; i < n && y + 18 <= SCR_H; i++, y += 19) scr.drawString(ls[i], PX, y);
}

static void drawPage(Model &m) {
    switch (page) {
    case PG_APPS: drawApps(m); break;
    case PG_PC: drawPC(m); break;
    case PG_DAY: drawDay(m); break;
    case PG_HERMES: drawHermes(m); break;
    default: drawIdle(m);
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
    bool idle = !m.confirm && !m.card && !m.bubble;
    drawStatus(m, idle);
    if (m.confirm) drawConfirm(m, now);
    else if (m.card) drawCard(m);
    else if (m.bubble) drawBubble(m);
    else drawPage(m);
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

static struct { bool down = false; int x = 0, y = 0, x0 = 0, y0 = 0; uint32_t t0 = 0; } tch;

static void setPage(int p, uint32_t now) {
    p = constrain(p, 0, NPG - 1);
    if (p != page) { page = p; drawnVersion = UINT32_MAX; }
    pageAt = now;
}

static void handleTouch(Model &m, uint32_t now) {
    lgfx::touch_point_t tp;
    if (lcd->getTouch(&tp)) {
        if (!tch.down) { tch.down = true; tch.t0 = now; tch.x0 = tp.x; tch.y0 = tp.y; }
        pageAt = now;
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
    int dx = x - tch.x0, dy = y - tch.y0;
    if (!m.card && !m.bubble && abs(dx) > 50 && abs(dy) < 60) {      // deslizar cambia de página
        setPage(page + (dx < 0 ? 1 : -1), now);
        return;
    }
    if (!m.bubble && (page == PG_MAIN || page == PG_APPS)) {         // en reposo, los botones de apps
        int n = page == PG_MAIN ? min(m.dash.nApps, MAIN_APPS) : m.dash.nApps;
        for (int i = 0; i < n; i++)
            if (inside(page == PG_MAIN ? appBox(i, n) : gridBox(i), x, y)) {
                pressedApp = i;
                pressedAt = now;
                drawnVersion = UINT32_MAX;               // repintar ya: el botón se ilumina
                String t = "app:" + m.dash.appId[i];
                sendTouch(t.c_str(), x, y, "tap");
                return;
            }
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

    if (m.dash.have && m.dash.version != histVersion) {                   // un punto más en la gráfica de la CPU
        histVersion = m.dash.version;
        if (nHist == 40) { memmove(cpuHist, cpuHist + 1, 39 * sizeof(float)); nHist = 39; }
        cpuHist[nHist++] = constrain(m.dash.cpu, 0.0f, 100.0f);
    }
    if (m.bubble && m.bubbleText.length()) { lastReply = m.bubbleText; nLastTools = 0; }
    if (m.card && m.cardKind == "tools" && m.nTools) {
        nLastTools = min(m.nTools, 3);
        for (int i = 0; i < nLastTools; i++) lastTools[i] = m.tools[i].name + ": " + m.tools[i].detail;
    }
    if ((m.confirm || m.card || m.bubble) && page != PG_MAIN) { page = PG_MAIN; pageAt = now; drawnVersion = UINT32_MAX; }
    if (page != PG_MAIN && !tch.down && now - pageAt > 60000) { page = PG_MAIN; drawnVersion = UINT32_MAX; }   // vuelve sola

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
