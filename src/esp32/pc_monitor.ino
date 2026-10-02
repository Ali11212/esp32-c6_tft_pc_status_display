#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define TFT_CS   14
#define TFT_DC   15
#define TFT_RST  18
#define TFT_BLK  8

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Titresimsiz cizim icin ekran disi tampon (128x160x2 = ~40KB)
GFXcanvas16 *canvas;

// Wi-Fi Ayarları
const char* ssid = "TurkTelekom_Mesh_TP0820";
const char* password = "ebEg3Ly4KWy3";

// PC IP ve port
const char* pcIP = "192.168.1.116";  // Buraya PC IP'sini yaz
const uint16_t port = 5001;
WiFiClient client;

// ---------- Renkler ----------
#define RGB(r,g,b) ((uint16_t)((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|((b)>>3)))
const uint16_t C_BG     = RGB(8, 10, 16);
const uint16_t C_CARD   = RGB(18, 22, 32);
const uint16_t C_TRACK  = RGB(36, 42, 58);
const uint16_t C_DIM    = RGB(120, 130, 155);
const uint16_t C_WHITE  = RGB(255, 255, 255);
const uint16_t C_GREEN  = RGB(0, 230, 130);
const uint16_t C_AMBER  = RGB(255, 190, 0);
const uint16_t C_RED    = RGB(255, 70, 80);
const uint16_t C_CYAN   = RGB(0, 210, 255);
const uint16_t C_BLUE   = RGB(70, 90, 255);
const uint16_t C_PINK   = RGB(255, 80, 200);
const uint16_t C_PURPLE = RGB(150, 80, 255);

// ---------- Ölçerler ----------
struct Meter { int x, y, radius; const char* label; int value; float disp; };
Meter meters[4] = {
  {32, 39, 23, "CPU", 0, 0}, {96, 39, 23, "GPU", 0, 0},
  {32, 92, 23, "RAM", 0, 0}, {96, 92, 23, "DSK", 0, 0}
};
int dl = 0, ul = 0;
float dlDisp = 0, ulDisp = 0;

// ---------- Yay çizimi için sabitler ----------
#define SEGS   180
#define THICK  4
float cosT[SEGS + 1], sinT[SEGS + 1];

uint32_t introStart = 0;
#define INTRO_MS 1300
#define FRAME_MS 33

// ---------- Yardımcılar ----------
uint16_t lerpC(uint16_t a, uint16_t b, float t) {
  if (t < 0) t = 0; if (t > 1) t = 1;
  int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
  int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  int r = ar + (br - ar) * t;
  int g = ag + (bg - ag) * t;
  int bl = ab + (bb - ab) * t;
  return (r << 11) | (g << 5) | bl;
}

// t: 0..1 -> yeşil -> sarı -> kırmızı
uint16_t gradColor(float t) {
  if (t < 0.5f) return lerpC(C_GREEN, C_AMBER, t * 2.0f);
  return lerpC(C_AMBER, C_RED, (t - 0.5f) * 2.0f);
}

void centerText(const char* s, int cx, int y) {
  int16_t tx, ty; uint16_t tw, th;
  canvas->getTextBounds(s, 0, 0, &tx, &ty, &tw, &th);
  canvas->setCursor(cx - tw / 2, y);
  canvas->print(s);
}

void flush() {
  tft.drawRGBBitmap(0, 0, canvas->getBuffer(), 128, 160);
}

// ---------- Çizim fonksiyonları ----------
void drawHeader() {
  canvas->setTextSize(1);
  canvas->setTextColor(C_DIM);
  canvas->setCursor(6, 3);
  canvas->print("PC MONITOR");

  if (client.connected()) {
    float p = (millis() % 1400) / 1400.0f;           // nabız halkası
    canvas->drawCircle(120, 6, 3 + (int)(p * 4), lerpC(C_GREEN, C_BG, p));
    canvas->fillCircle(120, 6, 3, C_GREEN);
  } else {
    bool on = (millis() / 500) % 2;                   // yanıp sönen kırmızı
    canvas->fillCircle(120, 6, 3, on ? C_RED : C_TRACK);
  }
  canvas->drawFastHLine(6, 12, 116, C_TRACK);
}

void drawGauge(Meter &m) {
  int cx = m.x, cy = m.y, r = m.radius;

  // kart
  canvas->fillRoundRect(cx - 31, cy - 25, 62, 51, 6, C_CARD);

  // iz (arka plan yayı)
  for (int s = 0; s <= SEGS; s++) {
    canvas->drawLine(cx + cosT[s] * (r - THICK) + 0.5f, cy + sinT[s] * (r - THICK) + 0.5f,
                     cx + cosT[s] * r + 0.5f,           cy + sinT[s] * r + 0.5f, C_TRACK);
  }

  // dolu kısım (gradyan)
  int n = (int)(m.disp * SEGS / 100.0f + 0.5f);
  for (int s = 0; s <= n; s++) {
    uint16_t c = gradColor((float)s / SEGS);
    canvas->drawLine(cx + cosT[s] * (r - THICK) + 0.5f, cy + sinT[s] * (r - THICK) + 0.5f,
                     cx + cosT[s] * r + 0.5f,           cy + sinT[s] * r + 0.5f, c);
  }
  // uçtaki parlak nokta
  if (n > 0) {
    int tx = cx + cosT[n] * (r - THICK / 2) + 0.5f;
    int ty = cy + sinT[n] * (r - THICK / 2) + 0.5f;
    canvas->fillCircle(tx, ty, 2, C_WHITE);
  }

  // değer
  int val = (int)(m.disp + 0.5f);
  char num[5]; sprintf(num, "%d", val);
  int tw = strlen(num) * 12;
  bool pct = val < 100;
  int total = tw + (pct ? 7 : 0);
  int sx = cx - total / 2;
  canvas->setTextColor(C_WHITE);
  canvas->setTextSize(2);
  canvas->setCursor(sx, cy - 9);
  canvas->print(num);
  if (pct) {
    canvas->setTextSize(1);
    canvas->setTextColor(C_DIM);
    canvas->setCursor(sx + tw + 1, cy - 1);
    canvas->print("%");
  }

  // etiket
  canvas->setTextSize(1);
  canvas->setTextColor(C_DIM);
  centerText(m.label, cx, cy + 14);
}

void drawBar(int x, int y, int w, int h, float v, const char* label, uint16_t c1, uint16_t c2) {
  canvas->setTextSize(1);
  canvas->setTextColor(C_DIM);
  canvas->setCursor(x, y - 9);
  canvas->print(label);

  char buf[6]; sprintf(buf, "%d%%", (int)(v + 0.5f));
  int16_t tx, ty; uint16_t tw, th;
  canvas->getTextBounds(buf, 0, 0, &tx, &ty, &tw, &th);
  canvas->setTextColor(C_WHITE);
  canvas->setCursor(x + w - tw, y - 9);
  canvas->print(buf);

  canvas->fillRoundRect(x, y, w, h, h / 2, C_TRACK);
  int fw = (int)(w * v / 100.0f);
  for (int i = 0; i < fw; i++) {
    int inset = (i == 0 || i == fw - 1) ? 1 : 0;   // yuvarlak uçlar
    canvas->drawFastVLine(x + i, y + inset, h - inset * 2, lerpC(c1, c2, (float)i / w));
  }
}

void drawSplash(const char* txt, int f) {
  canvas->fillScreen(C_BG);
  int phase = f % 8;
  for (int i = 0; i < 8; i++) {
    int age = (phase - i + 8) % 8;
    float a = i * PI / 4.0f;
    canvas->fillCircle(64 + cos(a) * 16, 70 + sin(a) * 16, 3, lerpC(C_CYAN, C_BG, age / 8.0f));
  }
  canvas->setTextSize(1);
  canvas->setTextColor(C_DIM);
  centerText(txt, 64, 105);
  flush();
}

void ease(float &cur, float target) {
  cur += (target - cur) * 0.2f;
  if (fabs(target - cur) < 0.3f) cur = target;
}

void render() {
  uint32_t el = millis() - introStart;
  bool intro = el < INTRO_MS;
  float introVal = 0;
  if (intro) introVal = 100.0f * sin(PI * (float)el / INTRO_MS);

  // yumuşak geçiş
  for (int i = 0; i < 4; i++) {
    if (intro) meters[i].disp = introVal;
    else ease(meters[i].disp, meters[i].value);
  }
  if (intro) { dlDisp = introVal; ulDisp = introVal; }
  else { ease(dlDisp, dl); ease(ulDisp, ul); }

  canvas->fillScreen(C_BG);
  drawHeader();
  for (int i = 0; i < 4; i++) drawGauge(meters[i]);

  canvas->fillRoundRect(1, 121, 126, 38, 6, C_CARD);
  drawBar(8, 133, 112, 5, dlDisp, "DL", C_CYAN, C_BLUE);
  drawBar(8, 152, 112, 5, ulDisp, "UL", C_PINK, C_PURPLE);

  flush();
}

// ---------- Veri ayrıştırma (aynı mantık) ----------
void parseLine(String line) {
  int values[6]; int idx = 0;
  int start = 0;
  for (int i = 0; i <= line.length(); i++) {
    if (i == line.length() || line[i] == ',') {
      String part = line.substring(start, i);
      int sep = part.indexOf(':');
      if (sep > 0 && idx < 6) { values[idx++] = part.substring(sep + 1).toInt(); }
      start = i + 1;
    }
  }
  if (idx == 6) {
    meters[0].value = values[0]; meters[1].value = values[1];
    meters[2].value = values[2]; meters[3].value = values[3];
    dl = values[4]; ul = values[5];
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BLK, OUTPUT); digitalWrite(TFT_BLK, HIGH);
  SPI.begin(20, -1, 19);
  tft.initR(INITR_BLACKTAB);
  tft.setSPISpeed(16000000);   // artefakt olursa 8000000'e düşür
  tft.fillScreen(ST77XX_BLACK);

  canvas = new GFXcanvas16(128, 160);
  canvas->setTextWrap(false);

  // sin/cos tablosu (135° -> 405°, 270° süpürme)
  for (int s = 0; s <= SEGS; s++) {
    float a = (135.0f + 270.0f * s / SEGS) * PI / 180.0f;
    cosT[s] = cos(a); sinT[s] = sin(a);
  }

  WiFi.begin(ssid, password);
  int f = 0;
  while (WiFi.status() != WL_CONNECTED) {
    drawSplash("CONNECTING", f++);
    delay(60);
  }
  Serial.print("ESP IP: "); Serial.println(WiFi.localIP());

  client.setTimeout(20);
  introStart = millis();
}

void loop() {
  static uint32_t lastTry = 0, lastFrame = 0;

  if (!client.connected() && millis() - lastTry > 2000) {
    lastTry = millis();
    if (client.connect(pcIP, port, 300)) {
      Serial.println("PC'ye bağlandı");
    }
  }

  while (client.connected() && client.available()) {
    String line = client.readStringUntil('\n');
    parseLine(line);
  }

  if (millis() - lastFrame >= FRAME_MS) {
    lastFrame = millis();
    render();
  }
}
