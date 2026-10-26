/*
  DIY L-Desk Controller
  ESP32 + 3x BTS7960 H-bridges + 3 lifting columns with Hall encoders.

  Keypad:  ▲ / ▼ hold to move · 1 2 3 go to preset · M then 1/2/3 saves a preset
           any key stops an automatic move · hold M + ▼ for 3 s = reset (home)
           hold M for 3 s while showing "Err" = clear fault
  Safety:  soft start/stop, per-leg sync, skew cut-out, stall + speed-drop
           anti-collision with back-off, deadman for app moves, soft limits,
           power-loss detection (forces a reset if power died mid-move).
  App:     http://desk.local (served from this board) · OTA firmware updates.

  Board: "ESP32 Dev Module" — Arduino core "esp32 by Espressif" 3.x (2.x also OK)
  Library: "TM1637Display" by Avishay Orpaz (Library Manager)
*/
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <TM1637Display.h>
#include <esp_arduino_version.h>
#include <math.h>
#include "config.h"
#include "app_html.h"

// ------------------------------------------------------------------ PWM
static void pwmInit(int pin, int ch) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(pin, PWM_FREQ, PWM_BITS);
#else
  ledcSetup(ch, PWM_FREQ, PWM_BITS);
  ledcAttachPin(pin, ch);
#endif
}
static void pwmWrite(int pin, int ch, int duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)ch; ledcWrite(pin, duty);
#else
  (void)pin; ledcWrite(ch, duty);
#endif
}

// ------------------------------------------------------------- encoders
volatile int32_t cnt[3] = {0, 0, 0};
volatile int8_t encDir = 1;   // sign of the last commanded direction
void IRAM_ATTR hall0() { cnt[0] += encDir; }
void IRAM_ATTR hall1() { cnt[1] += encDir; }
void IRAM_ATTR hall2() { cnt[2] += encDir; }

// ---------------------------------------------------------------- state
enum Mode : uint8_t { IDLE, MANUAL, GOTO, HOMING, FAULT };
const char *MODE_NAME[] = {"idle", "manual", "goto", "homing", "fault"};

struct Settings { float cpm; int minH; int maxH; int preset[3]; } cfg;
Preferences prefs;
WebServer server(80);
TM1637Display disp(PIN_DISP_CLK, PIN_DISP_DIO);

Mode mode = IDLE;
int dir = 0;                 // +1 up, -1 down
bool stopping = false;
bool homed = false, dirty = false;
float targetMM = 0;
uint32_t deadmanUntil = 0, moveStartMs = 0, cruiseSince = 0;
int baseDuty = 0, legDuty[3] = {0, 0, 0};
int32_t lastCnt[3];
uint32_t lastChangeMs[3];
bool legDone[3];
float speedEma = 0;
int dropTicks = 0;
float spdRing[5] = {0}; uint8_t spdIdx = 0;
uint8_t faultCode = 0;        // 1 = legs out of sync, 2 = leg stalled during reset
float pendingTarget = NAN; uint32_t pendingAt = 0;
uint32_t saveDueAt = 0;
bool saveArmed = false; uint32_t saveArmedUntil = 0;
uint8_t msgSeg[4]; uint32_t msgUntil = 0;
float calMM1 = NAN, calC1 = 0;
bool netReady = false;

// --------------------------------------------------------------- buzzer
uint8_t beepsLeft = 0; uint16_t beepLen = 50; uint32_t beepNext = 0; bool beepOn = false;
void beep(uint8_t n, uint16_t len = 50) { beepsLeft = n; beepLen = len; beepNext = 0; }
void beepTick() {
  if (!beepsLeft && !beepOn) return;
  uint32_t now = millis();
  if (now < beepNext) return;
  if (beepOn) { digitalWrite(PIN_BUZZER, LOW); beepOn = false; beepsLeft--; beepNext = now + 70; }
  else if (beepsLeft) { digitalWrite(PIN_BUZZER, HIGH); beepOn = true; beepNext = now + beepLen; }
}

// -------------------------------------------------------------- display
const uint8_t CH_E = SEG_A | SEG_D | SEG_E | SEG_F | SEG_G;
const uint8_t CH_r = SEG_E | SEG_G;
const uint8_t CH_S = SEG_A | SEG_C | SEG_D | SEG_F | SEG_G;
const uint8_t CH_t = SEG_D | SEG_E | SEG_F | SEG_G;
const uint8_t CH_O = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F;
const uint8_t CH_P = SEG_A | SEG_B | SEG_E | SEG_F | SEG_G;
const uint8_t CH_DASH = SEG_G;
void showMsg(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint32_t ms) {
  msgSeg[0] = a; msgSeg[1] = b; msgSeg[2] = c; msgSeg[3] = d; msgUntil = millis() + ms;
}

// ------------------------------------------------------------- geometry
float legMM(int i) { return cfg.minH + cnt[i] / cfg.cpm; }
float heightMM() { return (legMM(0) + legMM(1) + legMM(2)) / 3.0f; }
float avgCounts() { return (cnt[0] + cnt[1] + cnt[2]) / 3.0f; }

// ----------------------------------------------------------- persistence
void loadSettings() {
  prefs.begin("desk", false);
  cfg.cpm  = prefs.getFloat("cpm", DEFAULT_COUNTS_PER_MM);
  cfg.minH = prefs.getInt("minH", DEFAULT_MIN_HEIGHT_MM);
  cfg.maxH = prefs.getInt("maxH", DEFAULT_MAX_HEIGHT_MM);
  int def[3] = DEFAULT_PRESETS;
  for (int i = 0; i < 3; i++) {
    char k[4] = {'p', char('0' + i), 0};
    cfg.preset[i] = prefs.getInt(k, def[i]);
    k[0] = 'c';
    cnt[i] = prefs.getInt(k, 0);
  }
  homed = prefs.getBool("homed", false);
  dirty = prefs.getBool("dirty", false);
  if (dirty) homed = false;   // power died while moving: position unknown
}
void saveSettings() {
  prefs.putFloat("cpm", cfg.cpm);
  prefs.putInt("minH", cfg.minH);
  prefs.putInt("maxH", cfg.maxH);
  for (int i = 0; i < 3; i++) { char k[4] = {'p', char('0' + i), 0}; prefs.putInt(k, cfg.preset[i]); }
}
void setDirty(bool v) { if (dirty == v) return; dirty = v; prefs.putBool("dirty", v); }
void saveCounts() {
  for (int i = 0; i < 3; i++) { char k[4] = {'c', char('0' + i), 0}; prefs.putInt(k, cnt[i]); }
  prefs.putBool("homed", homed);
  setDirty(false);
}

// ---------------------------------------------------------------- motors
void driveLeg(int i, int duty, int d) {
  duty = constrain(duty, 0, DUTY_MAX);
  if (LEG_INVERT[i]) d = -d;
  if (d > 0)      { pwmWrite(PIN_RPWM[i], i * 2, duty); pwmWrite(PIN_LPWM[i], i * 2 + 1, 0); }
  else if (d < 0) { pwmWrite(PIN_RPWM[i], i * 2, 0);    pwmWrite(PIN_LPWM[i], i * 2 + 1, duty); }
  else            { pwmWrite(PIN_RPWM[i], i * 2, 0);    pwmWrite(PIN_LPWM[i], i * 2 + 1, 0); }
}
void allOff() { for (int i = 0; i < 3; i++) { driveLeg(i, 0, 0); legDuty[i] = 0; } }

bool startMove(int d, Mode m, float target = 0) {
  if (mode == FAULT) return false;
  if (mode != IDLE) {
    if (dir == d) { mode = m; targetMM = target; stopping = false; return true; }  // same way: retarget
    stopping = true; return false;                                                 // reverse: stop first
  }
  dir = d; encDir = d; mode = m; targetMM = target; stopping = false;
  baseDuty = 0; speedEma = 0; dropTicks = 0; cruiseSince = 0;
  uint32_t now = millis(); moveStartMs = now;
  for (int i = 0; i < 3; i++) { lastCnt[i] = cnt[i]; lastChangeMs[i] = now; legDone[i] = false; spdRing[i] = 0; }
  spdRing[3] = spdRing[4] = 0;
  digitalWrite(PIN_EN, HIGH);
  digitalWrite(PIN_LED, HIGH);
  setDirty(true);
  return true;
}
void stopMotion() { if (mode != IDLE && mode != FAULT) stopping = true; }
void endMotion() {
  allOff(); baseDuty = 0; stopping = false; mode = IDLE;
  digitalWrite(PIN_EN, LOW);
  digitalWrite(PIN_LED, LOW);
  saveDueAt = millis() + 300;     // let the columns coast, then store position
}
void fault(uint8_t code) {
  allOff(); baseDuty = 0; stopping = false;
  digitalWrite(PIN_EN, LOW);
  mode = FAULT; faultCode = code; homed = false;
  saveCounts(); beep(1, 600);
}
void clearFault() { if (mode == FAULT) { mode = IDLE; faultCode = 0; beep(1); } }

bool gotoMM(float mm) {
  if (!homed || mode == FAULT || mode == HOMING) { beep(1, 300); return false; }
  mm = constrain(mm, (float)cfg.minH, (float)cfg.maxH);
  float h = heightMM();
  if (fabsf(mm - h) < 2.0f) return true;
  int d = mm > h ? 1 : -1;
  if (mode != IDLE && dir != d) { pendingTarget = mm; pendingAt = 0; stopMotion(); return true; }
  return startMove(d, GOTO, mm);
}
void manual(int d) {
  if (mode == FAULT || mode == HOMING) return;
  deadmanUntil = millis() + DEADMAN_MS;
  pendingTarget = NAN;
  if (mode == IDLE || dir == d) startMove(d, MANUAL);
  else stopMotion();
}
void startHoming() {
  if (mode != IDLE) return;
  homed = false;
  startMove(-1, HOMING);
  beep(2);
}
void finishHoming() {
  allOff();
  noInterrupts(); cnt[0] = cnt[1] = cnt[2] = 0; interrupts();
  homed = true;
  endMotion();
  saveDueAt = 0; saveCounts();
  beep(2, 120);
}
void onObstacle() {
  int d = dir;
  float h = heightMM();
  allOff(); endMotion();
  beep(3, 80);
  showMsg(CH_S, CH_t, CH_O, CH_P, 1500);
  if (homed) { pendingTarget = h - d * BACKOFF_MM; pendingAt = millis() + 300; }
}

// ---------------------------------------------------------- control loop
void controlTick() {
  if (mode == IDLE || mode == FAULT) return;
  uint32_t now = millis();

  float leg[3], h = 0;
  float spd = 0;
  for (int i = 0; i < 3; i++) {
    int32_t c = cnt[i];
    leg[i] = cfg.minH + c / cfg.cpm; h += leg[i];
    int32_t dc = abs(c - lastCnt[i]);
    if (dc > 0) lastChangeMs[i] = now;
    spd += dc; lastCnt[i] = c;
  }
  h /= 3.0f; spd /= 3.0f;
  spdRing[spdIdx] = spd; spdIdx = (spdIdx + 1) % 5;
  float spdWin = (spdRing[0] + spdRing[1] + spdRing[2] + spdRing[3] + spdRing[4]) / 5.0f;

  // ---- decide where we're heading and how fast
  int baseTarget = (mode == HOMING) ? DUTY_HOMING : DUTY_CRUISE;
  bool slowing = false;
  if (!stopping) {
    if (mode == MANUAL && now > deadmanUntil) stopping = true;
    if (homed && mode != HOMING) {
      float room = dir > 0 ? cfg.maxH - h : h - cfg.minH;
      if (room <= 1.0f) stopping = true;
      else if (room < SLOW_ZONE_MM) {
        baseTarget = DUTY_MIN_MOVE + (int)((DUTY_CRUISE - DUTY_MIN_MOVE) * room / SLOW_ZONE_MM);
        slowing = true;
      }
    }
    if (mode == GOTO) {
      float rem = (targetMM - h) * dir;
      if (rem <= STOP_EARLY_MM) stopping = true;
      else if (rem < SLOW_ZONE_MM) {
        int t = DUTY_MIN_MOVE + (int)((DUTY_CRUISE - DUTY_MIN_MOVE) * rem / SLOW_ZONE_MM);
        if (t < baseTarget) baseTarget = t;
        slowing = true;
      }
    }
  }
  if (stopping) baseTarget = 0;

  // ---- soft start / soft stop ramp
  if (baseDuty < baseTarget) baseDuty = min(baseTarget, baseDuty + RAMP_UP);
  else                       baseDuty = max(baseTarget, baseDuty - RAMP_DOWN);
  if (stopping && baseDuty == 0) { endMotion(); return; }

  // ---- leg synchronisation
  float maxL = max(leg[0], max(leg[1], leg[2]));
  float minL = min(leg[0], min(leg[1], leg[2]));
  if (mode != HOMING && (maxL - minL) > SKEW_LIMIT_MM) { fault(1); return; }

  for (int i = 0; i < 3; i++) {
    int d;
    if (mode == HOMING) d = legDone[i] ? 0 : baseDuty;
    else {
      float lead = (leg[i] - h) * dir;              // + means this leg is ahead
      d = baseDuty - (int)(KP_SYNC * lead);
      if (baseDuty > 0 && d < 0) d = 0;
    }
    legDuty[i] = constrain(d, 0, DUTY_MAX);
    driveLeg(i, legDuty[i], dir);
  }

  // ---- stall & collision detection
  if (stopping || now - moveStartMs < 500) return;
  for (int i = 0; i < 3; i++) {
    if (legDuty[i] >= DUTY_MIN_MOVE && now - lastChangeMs[i] > STALL_MS) {
      if (mode == HOMING) { legDone[i] = true; }
      else { onObstacle(); return; }
    }
  }
  if (mode == HOMING) {
    if (legDone[0] && legDone[1] && legDone[2]) finishHoming();
    else if (now - moveStartMs > 60000) fault(2);
    return;
  }
  if (!slowing && baseDuty == DUTY_CRUISE) {
    if (!cruiseSince) cruiseSince = now;
    if (now - cruiseSince < 300) { speedEma = spdWin; return; }       // learn cruise speed
    if (spdWin < speedEma * COLLISION_DROP) { if (++dropTicks >= 3) { onObstacle(); return; } }
    else { dropTicks = 0; speedEma = speedEma * 0.95f + spdWin * 0.05f; }
  } else cruiseSince = 0;
}

// --------------------------------------------------------------- buttons
enum { B_UP, B_DOWN, B_P1, B_P2, B_P3, B_M, NBTN };
struct Btn { int pin; bool state, raw, consumed; uint32_t changedAt, pressedAt; };
Btn btn[NBTN] = {{PIN_BTN_UP}, {PIN_BTN_DOWN}, {PIN_BTN_P1}, {PIN_BTN_P2}, {PIN_BTN_P3}, {PIN_BTN_M}};

void savePreset(int n, float mm) {
  cfg.preset[n] = (int)lroundf(constrain(mm, (float)cfg.minH, (float)cfg.maxH));
  saveSettings();
  saveArmed = false;
  showMsg(CH_S, CH_DASH, 0, disp.encodeDigit(n + 1), 1200);
  beep(2);
}
void onPress(int b) {
  uint32_t now = millis();
  if (mode == GOTO || mode == HOMING) {            // any key = stop
    stopMotion(); pendingTarget = NAN; btn[b].consumed = true; beep(1); return;
  }
  beep(1, 20);
  if (b >= B_P1 && b <= B_P3) {
    int n = b - B_P1;
    if (saveArmed && now < saveArmedUntil) savePreset(n, heightMM());
    else gotoMM(cfg.preset[n]);
  } else if (b == B_M) {
    saveArmed = true; saveArmedUntil = now + SAVE_WINDOW_MS;
    showMsg(CH_S, CH_DASH, 0, 0, SAVE_WINDOW_MS);
  }
}
void onRelease(int b) {
  if ((b == B_UP || b == B_DOWN) && mode == MANUAL) stopMotion();
  btn[b].consumed = false;
}
void buttonTick() {
  uint32_t now = millis();
  for (int i = 0; i < NBTN; i++) {
    bool r = digitalRead(btn[i].pin) == LOW;
    if (r != btn[i].raw) { btn[i].raw = r; btn[i].changedAt = now; }
    if (now - btn[i].changedAt > 25 && btn[i].state != r) {
      btn[i].state = r;
      if (r) { btn[i].pressedAt = now; onPress(i); } else onRelease(i);
    }
  }
  Btn &m = btn[B_M], &u = btn[B_UP], &d = btn[B_DOWN];
  if (m.state) {
    if (mode == FAULT && now - m.pressedAt > HOLD_HOME_MS) { clearFault(); m.consumed = true; }
    if (d.state && mode == IDLE && now - d.pressedAt > HOLD_HOME_MS && now - m.pressedAt > HOLD_HOME_MS) {
      saveArmed = false; msgUntil = 0; startHoming(); d.consumed = true;
    }
    return;                                           // M held: no manual moves
  }
  if (u.state && !d.state && !u.consumed) manual(+1);
  if (d.state && !u.state && !d.consumed) manual(-1);
  if (saveArmed && now > saveArmedUntil) saveArmed = false;
}

// --------------------------------------------------------------- display
void displayTick() {
  if (mode == FAULT) {
    uint8_t s[4] = {CH_E, CH_r, CH_r, disp.encodeDigit(faultCode)};
    disp.setSegments(s); return;
  }
  if (millis() < msgUntil) { disp.setSegments(msgSeg); return; }
  if (mode == HOMING) { uint8_t s[4] = {CH_r, CH_S, CH_t, 0}; disp.setSegments(s); return; }
  if (!homed) { uint8_t s[4] = {CH_DASH, CH_DASH, CH_DASH, CH_DASH}; disp.setSegments(s); return; }
  int mm = (int)lroundf(heightMM());              // 725 mm shows as "72.5"
  disp.showNumberDecEx(mm, 0b00100000, false);
}

// ------------------------------------------------------------------ web
void sendState() {
  char buf[640];
  snprintf(buf, sizeof buf,
    "{\"h\":%.1f,\"legs\":[%.1f,%.1f,%.1f],\"mode\":\"%s\",\"dir\":%d,\"target\":%.1f,"
    "\"homed\":%s,\"fault\":%d,\"presets\":[%d,%d,%d],\"min\":%d,\"max\":%d,\"cpm\":%.3f,"
    "\"duty\":[%d,%d,%d],\"saveArmed\":%s,\"calStep\":%d,\"rssi\":%d}",
    heightMM(), legMM(0), legMM(1), legMM(2), MODE_NAME[mode], mode == IDLE ? 0 : dir, targetMM,
    homed ? "true" : "false", faultCode, cfg.preset[0], cfg.preset[1], cfg.preset[2],
    cfg.minH, cfg.maxH, cfg.cpm, legDuty[0], legDuty[1], legDuty[2],
    saveArmed ? "true" : "false", isnan(calMM1) ? 0 : 1, WiFi.RSSI());
  server.send(200, "application/json", buf);
}
void sendErr(const char *msg) {
  String s = String("{\"error\":\"") + msg + "\"}";
  server.send(400, "application/json", s);
}
float argF(const char *k) { return server.hasArg(k) ? server.arg(k).toFloat() : NAN; }

void setupWeb() {
  server.on("/", [] { server.send_P(200, "text/html", APP_HTML); });
  server.on("/manifest.json", [] { server.send_P(200, "application/manifest+json", APP_MANIFEST); });
  server.on("/icon.svg", [] { server.send_P(200, "image/svg+xml", APP_ICON); });
  server.on("/api/state", sendState);
  server.on("/api/move", [] {
    String d = server.arg("dir");
    if (d == "up") manual(+1); else if (d == "down") manual(-1); else return sendErr("dir must be up or down");
    sendState();
  });
  server.on("/api/stop", [] { pendingTarget = NAN; stopMotion(); sendState(); });
  server.on("/api/goto", [] {
    float mm = argF("mm"); if (isnan(mm)) return sendErr("mm missing");
    if (!gotoMM(mm)) return sendErr(homed ? "busy" : "reset the desk first");
    sendState();
  });
  server.on("/api/preset", [] {
    int n = server.arg("n").toInt() - 1; if (n < 0 || n > 2) return sendErr("n must be 1-3");
    if (!gotoMM(cfg.preset[n])) return sendErr(homed ? "busy" : "reset the desk first");
    sendState();
  });
  server.on("/api/save", [] {
    int n = server.arg("n").toInt() - 1; if (n < 0 || n > 2) return sendErr("n must be 1-3");
    float mm = argF("mm");
    savePreset(n, isnan(mm) ? heightMM() : mm);
    sendState();
  });
  server.on("/api/home", [] {
    if (mode != IDLE) return sendErr("stop the desk first");
    startHoming(); sendState();
  });
  server.on("/api/clear", [] { clearFault(); sendState(); });
  server.on("/api/calibrate", [] {                      // "the desk is now X mm"
    float mm = argF("mm"); if (isnan(mm) || mode != IDLE) return sendErr("give mm while idle");
    cfg.minH = (int)lroundf(mm - avgCounts() / cfg.cpm);
    homed = true; saveSettings(); saveCounts(); sendState();
  });
  server.on("/api/calpoint", [] {                       // two-point scale calibration
    float mm = argF("mm"); if (isnan(mm) || mode != IDLE) return sendErr("give mm while idle");
    if (isnan(calMM1)) { calMM1 = mm; calC1 = avgCounts(); return sendState(); }
    float span = mm - calMM1;
    if (fabsf(span) < 200) { calMM1 = NAN; return sendErr("move at least 200 mm between points"); }
    float cpm = (avgCounts() - calC1) / span;
    calMM1 = NAN;
    if (cpm <= 0.5f) return sendErr("no pulses counted - check Hall wiring");
    cfg.cpm = cpm;
    cfg.minH = (int)lroundf(mm - avgCounts() / cfg.cpm);
    saveSettings(); sendState();
  });
  server.on("/api/limits", [] {
    float mx = argF("max");
    if (!isnan(mx) && mx > cfg.minH + 100) cfg.maxH = (int)mx;
    saveSettings(); sendState();
  });
  server.onNotFound([] { server.send(404, "text/plain", "not found"); });
  server.begin();
}

void wifiTick() {
  static bool apOn = false;
  static uint32_t t0 = millis();
  if (WiFi.status() == WL_CONNECTED) {
    if (!netReady) {
      netReady = true;
      MDNS.begin(HOSTNAME);
      MDNS.addService("http", "tcp", 80);
      ArduinoOTA.setHostname(HOSTNAME);
      ArduinoOTA.setPassword(OTA_PASSWORD);
      ArduinoOTA.onStart([] { allOff(); digitalWrite(PIN_EN, LOW); mode = FAULT; faultCode = 9; });
      ArduinoOTA.begin();
      Serial.printf("Desk online: http://%s.local  (%s)\n", HOSTNAME, WiFi.localIP().toString().c_str());
    }
  } else if (!apOn && millis() - t0 > 20000) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    apOn = true;
    Serial.println("Home Wi-Fi not found. Hotspot Desk-Setup -> http://192.168.4.1");
  }
}

// ----------------------------------------------------------------- main
void setup() {
  pinMode(PIN_EN, OUTPUT); digitalWrite(PIN_EN, LOW);   // drivers off before anything else
  for (int i = 0; i < 3; i++) {
    pwmInit(PIN_RPWM[i], i * 2); pwmInit(PIN_LPWM[i], i * 2 + 1);
    driveLeg(i, 0, 0);
  }
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT); pinMode(PIN_BUZZER, OUTPUT);
  for (int i = 0; i < NBTN; i++) {
    pinMode(btn[i].pin, btn[i].pin >= 34 ? INPUT : INPUT_PULLUP);
  }
  pinMode(PIN_HALL[0], INPUT_PULLUP); pinMode(PIN_HALL[1], INPUT_PULLUP); pinMode(PIN_HALL[2], INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_HALL[0]), hall0, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_HALL[1]), hall1, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_HALL[2]), hall2, RISING);

  loadSettings();
  disp.setBrightness(5);
  displayTick();
  beep(1);

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  setupWeb();
}

void loop() {
  static uint32_t nextTick = 0, nextDisp = 0;
  server.handleClient();
  if (netReady) ArduinoOTA.handle();
  wifiTick();

  uint32_t now = millis();
  if ((int32_t)(now - nextTick) >= 0) {
    nextTick = now + TICK_MS;
    buttonTick();
    controlTick();
  }
  if (!isnan(pendingTarget) && mode == IDLE && now >= pendingAt) {
    float t = pendingTarget; pendingTarget = NAN; gotoMM(t);
  }
  if (saveDueAt && now >= saveDueAt) { saveDueAt = 0; saveCounts(); }
  if ((int32_t)(now - nextDisp) >= 0) { nextDisp = now + 100; displayTick(); }
  beepTick();
}
