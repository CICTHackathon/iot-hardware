// Board pin map, build options, and the runtime-tunable configuration record.
#pragma once

#include <Arduino.h>

// ESP32 DevKit V1 (ESP32-WROOM-32, 30-pin), README §4.2. Four of these pins are
// not silkscreened with their GPIO number, so the silkscreen is in the comment:
// wire against that, there is no hole marked D17, D36 or D39 to find.
#define PIN_RC522_SS     5      // D5  — VSPI slave select
#define PIN_RC522_RST    2      // D2
#define PIN_RC522_SCK   18      // D18
#define PIN_RC522_MOSI  23      // D23
#define PIN_RC522_MISO  19      // D19

#define PIN_I2C_SDA     21      // D21 — display + recorder
#define PIN_I2C_SCL     22      // D22
#define I2C_HZ      100000UL    // through the BSS138; 400 kHz works, 100 kHz is
                                // what the translator is comfortable with on
                                // breadboard capacitance

#define PIN_SERVO       17      // TX2 — LEDC-driven 50 Hz PWM
#define PIN_LED_GRANT   25      // D25
#define PIN_LED_DENY    33      // D33
#define PIN_BUZZER      32      // D32 — active high
#define PIN_REC_TRIG     4      // D4  — level-triggered recording request

// Keypad. Rows are driven low one at a time; the columns are input-only pins
// with no internal pull-up, so each carries a discrete 10 kΩ to 3V3 (§4.2).
static const uint8_t KEYPAD_ROW_PINS[4] = {13, 14, 27, 26};   // D13 D14 D27 D26
static const uint8_t KEYPAD_COL_PINS[4] = {34, 35, 36, 39};   // D34 D35 VP  VN
static const char    KEYPAD_MAP[4][4] = {{'1', '2', '3', 'A'},
                                         {'4', '5', '6', 'B'},
                                         {'7', '8', '9', 'C'},
                                         {'*', '0', '#', 'D'}};

// HD44780 behind a PCF8574 I²C backpack. 0x27 is the PCF8574 part; a PCF8574A
// backpack answers at 0x3F instead. Set 16/2 for a 1602 — the panel code drops
// the lines the glass does not have rather than wrapping them.
#define LCD_I2C_ADDR  0x27
#define LCD_COLS      20
#define LCD_ROWS       4

// Enforcement timings, README §5.1. All of them are runtime-tunable below;
// these are the values the node boots with before NVS is read.
#define T_PIN_MS      15000     // silence in PIN_ENTRY before the transaction dies
#define T_PDP_MS       4000     // decision-tier wait before falling to DEGRADED
#define T_OPEN_MS      5000     // latch held open
#define T_ANNUNCIATE_MS 2500    // how long a denial is shown and sounded
#define PIN_MIN_DIGITS    4
#define PIN_MAX_DIGITS    8
#define PIN_MAX_ATTEMPTS  3

#define SERVO_STEP_MS     10    // one degree per tick — a sweep, not a slam
#define REC_PREROLL_MS  1200    // REC_TRIG asserted this far ahead of actuation

#define AUDIT_RING     48       // records held for the web tier to drain
#define ACL_ENTRIES    16       // cached authorisation set, §3
#define TXN_HEX        17       // 8 random bytes as hex, plus NUL

// Access point raised when no station credentials are stored, or when the
// stored ones fail. The node is useless to an operator it cannot be reached
// from, so it fails to provisioning rather than to silence.
#define SETUP_AP_SSID "gateway-setup"
#define SETUP_AP_PASS "attestation"     // >= 8 chars, or the AP starts open
#define WIFI_CONNECT_TIMEOUT_MS 15000

#define HTTP_UI_PORT 80

#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.nist.gov"

// Runtime configuration (persisted in NVS, namespace "gateway")
struct RuntimeConfig {
  // Actuation
  uint8_t  closedAngle;      // servo angle with the latch shot
  uint8_t  openAngle;
  uint16_t openHoldMs;

  // Enforcement
  uint16_t pinTimeoutMs;
  uint16_t pdpTimeoutMs;
  uint8_t  pinAttempts;
  bool     requirePin;       // false makes the card the only factor — bench only
  bool     degradedAllow;    // consult the cached ACL when the PDP is unreachable
  bool     buzzerEnabled;

  // Decision tier
  char pdpUrl[96];           // empty: the node never calls out, and waits to be
                             // told through POST /api/decision instead
  char pdpToken[48];         // bearer token presented to the PDP
  char decisionKey[65];      // HMAC-SHA256 key for inbound decisions, hex or
                             // ASCII; empty accepts unsigned ones (audited)
  char apiToken[33];         // required by every mutating HTTP endpoint
  char doorId[24];
};

inline RuntimeConfig defaultConfig() {
  RuntimeConfig c = {};
  c.closedAngle   = 90;      // the reference geometry of [6]: 90 shot, 20 clear
  c.openAngle     = 20;
  c.openHoldMs    = T_OPEN_MS;
  c.pinTimeoutMs  = T_PIN_MS;
  c.pdpTimeoutMs  = T_PDP_MS;
  c.pinAttempts   = PIN_MAX_ATTEMPTS;
  c.requirePin    = true;
  c.degradedAllow = true;
  c.buzzerEnabled = true;
  strlcpy(c.doorId, "door-1", sizeof(c.doorId));
  return c;
}

extern RuntimeConfig g_cfg;

void configLoad();
void configSave();
bool configSetKey(const char* key, const char* value);   // one setting, from HTTP
