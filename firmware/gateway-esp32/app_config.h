// Board pin map, build options, and the runtime-tunable configuration record.
#pragma once

#include <Arduino.h>
#include <time.h>

// ESP32 DevKit V1 (ESP32-WROOM-32, 30-pin), README §4.2. Four of these pins are
// not silkscreened with their GPIO number, so the silkscreen is in the comment:
// wire against that, there is no hole marked D17, D36 or D39 to find.
#define PIN_RC522_SS     5      // D5  — VSPI slave select
#define PIN_RC522_RST    2      // D2
#define PIN_RC522_SCK   18      // D18
#define PIN_RC522_MOSI  23      // D23
#define PIN_RC522_MISO  19      // D19

// The whole I²C bus is the LCD backpack and nothing else. The recorder used to
// be a second device on this pair; it is now a standalone node sharing no net
// with this one (README §4.3), so the bus never leaves the enclosure.
#define PIN_I2C_SDA     21      // D21 — display
#define PIN_I2C_SCL     22      // D22
#define I2C_HZ      100000UL    // 400 kHz works; 100 kHz is what the backpack is
                                // comfortable with on breadboard capacitance

#define PIN_SERVO       17      // TX2 — LEDC-driven 50 Hz PWM
#define PIN_LED_GRANT   25      // D25
#define PIN_LED_DENY    33      // D33
#define PIN_BUZZER      32      // D32 — active high

// D4 and RX2 (GPIO 4 and 16) are the node's only spares. D4 carried REC_TRIG to
// the recorder before the two nodes were separated; nothing drives it now, and
// this firmware deliberately does not claim it.

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

// A clock counts as set once it is past a moment this firmware could not have
// been built before. The console joins credential events to motion events on
// the epoch and on nothing else (README §4.3), so this predicate is what
// separates evidence from two stopwatches started at different moments. Both
// firmwares carry the identical test; a node that fails it says so rather than
// reporting 1970 as a time.
#define CLOCK_SET_AFTER 1700000000L   // 2023-11-14, well before any build here

inline bool clockSet() { return time(nullptr) > CLOCK_SET_AFTER; }

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

// Settings are edited through a candidate record: fields are applied to a copy,
// the copy is judged as a whole, and only then does it become the live one. A
// pair of settings that is only invalid in combination — the two servo angles —
// cannot be checked any other way, and a rejection that has already touched the
// live record is how a node loses its API token to a typo.
extern const char* const kConfigKeys[];
extern const size_t      kConfigKeyCount;

bool configApplyOne(RuntimeConfig* into, const char* key, const char* value);
bool configValidate(const RuntimeConfig& candidate, const char** why);
void configCommit(const RuntimeConfig& candidate);        // assigns and persists
bool configSetKey(const char* key, const char* value);    // one setting, atomic
