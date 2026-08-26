// ESP32 DevKit V1 enforcement node for the Smart Gateway edge tier.
//
// Credential capture (RC522 + 4x4 keypad), the access FSM of README §5.1, the
// latch, and the HTTP surface the web application drives. The display is on
// this node's own I²C bus — there is no display co-processor and no firmware
// anywhere else in the enforcement path.

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

bool configSetKey(const char* key, const char* value) {
  if (!strcmp(key, "closedAngle"))   g_cfg.closedAngle = clampU(value, 0, 180);
  else if (!strcmp(key, "openAngle"))     g_cfg.openAngle = clampU(value, 0, 180);
  else if (!strcmp(key, "openHoldMs"))    g_cfg.openHoldMs = clampU(value, 500, 60000);
  else if (!strcmp(key, "pinTimeoutMs"))  g_cfg.pinTimeoutMs = clampU(value, 2000, 60000);
  else if (!strcmp(key, "pdpTimeoutMs"))  g_cfg.pdpTimeoutMs = clampU(value, 500, 30000);
  else if (!strcmp(key, "pinAttempts"))   g_cfg.pinAttempts = clampU(value, 1, 10);
  else if (!strcmp(key, "requirePin"))    g_cfg.requirePin = truthy(value);
  else if (!strcmp(key, "degradedAllow")) g_cfg.degradedAllow = truthy(value);
  else if (!strcmp(key, "buzzerEnabled")) g_cfg.buzzerEnabled = truthy(value);
  else if (!strcmp(key, "pdpUrl"))      strlcpy(g_cfg.pdpUrl, value, sizeof(g_cfg.pdpUrl));
  else if (!strcmp(key, "pdpToken"))    strlcpy(g_cfg.pdpToken, value, sizeof(g_cfg.pdpToken));
  else if (!strcmp(key, "decisionKey")) strlcpy(g_cfg.decisionKey, value, sizeof(g_cfg.decisionKey));
  else if (!strcmp(key, "apiToken"))    strlcpy(g_cfg.apiToken, value, sizeof(g_cfg.apiToken));
  else if (!strcmp(key, "doorId"))      strlcpy(g_cfg.doorId, value, sizeof(g_cfg.doorId));
  else return false;

  // The two angles being equal would leave a latch that never moves and a node
  // that reports every transit as successful.
  if (g_cfg.openAngle == g_cfg.closedAngle) {
    g_cfg = defaultConfig();
    return false;
  }
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
  }
  delay(1);
}
