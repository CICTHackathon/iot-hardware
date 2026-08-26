// ESP32 DevKit V1 enforcement node for the Smart Gateway edge tier.

#include <Preferences.h>
#include <WiFi.h>
#include <esp_random.h>

#include "access.h"
#include "app_config.h"
#include "latch.h"
#include "panel.h"
#include "web.h"

RuntimeConfig g_cfg = defaultConfig();

// Configuration persistence

void configLoad() {
  Preferences p;
  p.begin("gateway", true);
  // The blob is versioned by its own size: a firmware whose RuntimeConfig has
  // grown ignores the old record rather than reading a shorter one into a
  // longer struct and inheriting whatever happened to follow it in NVS.
  RuntimeConfig stored;
  if (p.getBytesLength("cfg") == sizeof(stored) &&
      p.getBytes("cfg", &stored, sizeof(stored)) == sizeof(stored)) {
    g_cfg = stored;
  }
  p.end();
}

void configSave() {
  Preferences p;
  p.begin("gateway", false);
  p.putBytes("cfg", &g_cfg, sizeof(g_cfg));
  p.end();
}

static uint32_t clampU(const char* v, uint32_t lo, uint32_t hi) {
  const uint32_t n = strtoul(v, nullptr, 10);
  return n < lo ? lo : (n > hi ? hi : n);
}

static bool truthy(const char* v) {
  return v[0] == '1' || v[0] == 't' || v[0] == 'T' || v[0] == 'y' || v[0] == 'Y';
}

// Every settable key, in the order the console renders them. web.cpp walks this
// to pick a whole settings record out of one request body.
const char* const kConfigKeys[] = {
    "closedAngle", "openAngle", "openHoldMs", "pinTimeoutMs", "pdpTimeoutMs",
    "pinAttempts", "requirePin", "degradedAllow", "buzzerEnabled",
    "doorId", "pdpUrl", "pdpToken", "decisionKey", "apiToken",
};
const size_t kConfigKeyCount = sizeof(kConfigKeys) / sizeof(kConfigKeys[0]);

// Writes one key into a candidate record. Deliberately does not validate: a
// setting that is only wrong in combination with another one cannot be judged
// until every field of the request has been applied.
bool configApplyOne(RuntimeConfig* c, const char* key, const char* value) {
  if (!strcmp(key, "closedAngle"))        c->closedAngle = clampU(value, 0, 180);
  else if (!strcmp(key, "openAngle"))     c->openAngle = clampU(value, 0, 180);
  else if (!strcmp(key, "openHoldMs"))    c->openHoldMs = clampU(value, 500, 60000);
  else if (!strcmp(key, "pinTimeoutMs"))  c->pinTimeoutMs = clampU(value, 2000, 60000);
  else if (!strcmp(key, "pdpTimeoutMs"))  c->pdpTimeoutMs = clampU(value, 500, 30000);
  else if (!strcmp(key, "pinAttempts"))   c->pinAttempts = clampU(value, 1, 10);
  else if (!strcmp(key, "requirePin"))    c->requirePin = truthy(value);
  else if (!strcmp(key, "degradedAllow")) c->degradedAllow = truthy(value);
  else if (!strcmp(key, "buzzerEnabled")) c->buzzerEnabled = truthy(value);
  else if (!strcmp(key, "pdpUrl"))      strlcpy(c->pdpUrl, value, sizeof(c->pdpUrl));
  else if (!strcmp(key, "pdpToken"))    strlcpy(c->pdpToken, value, sizeof(c->pdpToken));
  else if (!strcmp(key, "decisionKey")) strlcpy(c->decisionKey, value, sizeof(c->decisionKey));
  else if (!strcmp(key, "apiToken"))    strlcpy(c->apiToken, value, sizeof(c->apiToken));
  else if (!strcmp(key, "doorId"))      strlcpy(c->doorId, value, sizeof(c->doorId));
  else return false;
  return true;
}

// Judges a candidate record as a whole. Nothing here can be decided one field
// at a time, which is the reason the candidate exists.
bool configValidate(const RuntimeConfig& c, const char** why) {
  // Equal angles leave a latch that never moves and a node that reports every
  // transit as successful.
  if (c.openAngle == c.closedAngle) {
    if (why) *why = "openAngle and closedAngle must differ";
    return false;
  }
  // Nothing else here is a pair. openHoldMs needs no relation to the sweep:
  // latchTick starts the hold when the servo reaches the open angle, not when
  // the sweep is commanded, so a short hold shortens the wait and never cuts
  // the opening travel short.
  return true;
}

void configCommit(const RuntimeConfig& c) {
  g_cfg = c;
  configSave();
}

// One key, applied atomically. The candidate is what gets written to, so a
// rejected value leaves the live record untouched — which matters because the
// API token is in that record, and a node that loses it answers /api/unlock to
// anyone who can reach port 80.
bool configSetKey(const char* key, const char* value) {
  RuntimeConfig c = g_cfg;
  if (!configApplyOne(&c, key, value)) return false;
  if (!configValidate(c, nullptr)) return false;
  g_cfg = c;
  return true;
}

// A node whose unlock endpoint answers unauthenticated is not an access-control
// device. If no token has been set, one is minted here and printed once.
static void ensureApiToken() {
  if (g_cfg.apiToken[0]) return;
  static const char* H = "0123456789abcdef";
  uint8_t r[16];
  esp_fill_random(r, sizeof(r));
  for (size_t i = 0; i < sizeof(r); ++i) {
    g_cfg.apiToken[2 * i] = H[r[i] >> 4];
    g_cfg.apiToken[2 * i + 1] = H[r[i] & 15];
  }
  g_cfg.apiToken[32] = 0;
  configSave();
  Serial.printf("[api] token generated: %s\n", g_cfg.apiToken);
}

// NTP is a dependency of this node, not a convenience. The recorder shares no
// wire with it (README §4.3), so the epoch stamped on an audit record is the
// only thing the console can correlate it against. Report it once, so §5.2
// step 4 is something an operator can read off the serial console.
static void clockReport() {
  static bool reported = false;
  if (reported || !clockSet()) return;
  reported = true;
  const time_t t = time(nullptr);
  struct tm tmv;
  localtime_r(&t, &tmv);
  char stamp[24];
  strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tmv);
  Serial.printf("[net] clock set: %s UTC\n", stamp);
}

static void wifiConnect() {
  Preferences p;
  p.begin("gateway", true);
  const String ssid = p.getString("ssid", "");
  const String pass = p.getString("pass", "");
  p.end();

  if (ssid.length()) {
    Serial.printf("[net] joining %s\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);          // sleep adds hundreds of ms to a decision
    WiFi.begin(ssid.c_str(), pass.c_str());
    const uint32_t deadline = millis() + WIFI_CONNECT_TIMEOUT_MS;
    while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
      panelTick();                 // the panel keeps painting while we wait
      delay(50);
    }
    if (WiFi.isConnected()) {
      Serial.printf("[net] %s\n", WiFi.localIP().toString().c_str());
      configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);
      return;
    }
    Serial.println("[net] join failed");
  }

  // The door still works without a network — it just falls to the cached
  // authorisation set on every transaction, which is the DEGRADED path.
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SETUP_AP_SSID, SETUP_AP_PASS);
  Serial.printf("[net] setup AP %s at %s\n", SETUP_AP_SSID,
                WiFi.softAPIP().toString().c_str());
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.println("\n[boot] smart gateway enforcement node");

  configLoad();
  ensureApiToken();

  if (!panelBegin())
    Serial.printf("[lcd] no backpack at 0x%02X - is it a PCF8574A at 0x3F?\n", LCD_I2C_ADDR);
  panelShow("Smart Gateway", "starting", "", nullptr);

  if (!latchBegin()) Serial.println("[servo] attach failed");
  if (!accessBegin()) Serial.println("[boot] reader unavailable, node will deny");

  wifiConnect();

  if (!webBegin()) Serial.println("[boot] web server failed to start");
  else Serial.printf("[web] http://%s/\n",
                     (WiFi.isConnected() ? WiFi.localIP() : WiFi.softAPIP()).toString().c_str());
}

void loop() {
  // Three bounded calls and nothing else. Every long-running thing in this
  // firmware — the PDP round trip, the HTTP handlers — is a separate task, so
  // the worst-case path from a card being presented to the latch moving does
  // not depend on the network being awake.
  panelTick();
  latchTick();
  accessTick();

  static uint32_t lastCheck = 0;
  if (millis() - lastCheck > 5000) {
    lastCheck = millis();
    if (WiFi.getMode() == WIFI_STA && !WiFi.isConnected()) {
      Serial.println("[net] link lost, reconnecting");
      WiFi.reconnect();
    }
    clockReport();
  }
  delay(1);
}
