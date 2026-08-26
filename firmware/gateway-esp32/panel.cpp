#include "panel.h"

#include <Wire.h>

namespace {

// PCF8574 → HD44780 backpack wiring 
// Fixed by the module, not a choice: P0 RS, P1 R/W, P2 E, P3 backlight,
// P4–P7 the high nibble D4–D7.
constexpr uint8_t LCD_RS = 0x01;
constexpr uint8_t LCD_EN = 0x04;
constexpr uint8_t LCD_BL = 0x08;

bool     g_lcdOk = false;
uint8_t  g_backlight = LCD_BL;

char g_shadow[LCD_ROWS][LCD_COLS + 1];   // what the glass is showing
char g_want[LCD_ROWS][LCD_COLS + 1];     // what it should be showing
bool g_dirty[LCD_ROWS] = {false};

void pcf(uint8_t v) {
  Wire.beginTransmission(LCD_I2C_ADDR);
  Wire.write(v | g_backlight);
  Wire.endTransmission();
}

void pulse(uint8_t v) {
  pcf(v | LCD_EN);
  delayMicroseconds(1);          // E high >= 450 ns
  pcf(v & ~LCD_EN);
  delayMicroseconds(50);         // most instructions settle in 37 us
}

void write4(uint8_t nibble, uint8_t rs) {
  const uint8_t v = (nibble & 0xF0) | rs;
  pcf(v);
  pulse(v);
}

void lcdCmd(uint8_t c) {
  write4(c & 0xF0, 0);
  write4(static_cast<uint8_t>(c << 4), 0);
}

void lcdData(uint8_t d) {
  write4(d & 0xF0, LCD_RS);
  write4(static_cast<uint8_t>(d << 4), LCD_RS);
}

// DDRAM origin of each line. Rows 2 and 3 are the continuations of rows 0 and 1
// — an HD44780 has two line buffers however many rows the glass is cut into.
uint8_t rowAddr(uint8_t row) {
  const uint8_t base = (row & 1) ? 0x40 : 0x00;
  return base + ((row >= 2) ? LCD_COLS : 0);
}

bool lcdPresent() {
  Wire.beginTransmission(LCD_I2C_ADDR);
  return Wire.endTransmission() == 0;
}

void lcdInit() {
  delay(50);                     // >= 40 ms after VDD reaches 4.5 V
  write4(0x30, 0); delay(5);     // the 8-bit reset triple, per the datasheet
  write4(0x30, 0); delayMicroseconds(150);
  write4(0x30, 0); delayMicroseconds(150);
  write4(0x20, 0); delayMicroseconds(150);   // and now 4-bit
  lcdCmd(0x28);                  // 4-bit, 2 line buffers, 5x8 font
  lcdCmd(0x08);                  // display off
  lcdCmd(0x01); delay(2);        // clear — the one instruction that needs ms
  lcdCmd(0x06);                  // entry mode: increment, no shift
  lcdCmd(0x0C);                  // display on, cursor and blink off
}

// Keypad
// One row is driven per tick, so a full scan takes four ticks and no single
// call holds the loop. Columns idle high on their external pull-ups (§4.2).

uint8_t  g_row = 0;
char     g_held = 0;             // key currently down, 0 when none
char     g_seen = 0;             // key found during the sweep in progress
char     g_pending = 0;          // debounced press waiting to be collected
uint32_t g_heldSince = 0;
uint32_t g_lastScan = 0;

// Annuciator

Annunciation g_ann = ANN_IDLE;
uint32_t     g_annSince = 0;
bool         g_toneOn = false;

void toneOn(uint16_t hz) {
  if (!g_cfg.buzzerEnabled) return;
  if (!g_toneOn) { tone(PIN_BUZZER, hz); g_toneOn = true; }
}

void toneOff() {
  if (g_toneOn) { noTone(PIN_BUZZER); digitalWrite(PIN_BUZZER, LOW); g_toneOn = false; }
}

void annTick() {
  const uint32_t t = millis() - g_annSince;
  switch (g_ann) {
    case ANN_OFF:
    case ANN_IDLE:
      digitalWrite(PIN_LED_GRANT, LOW);
      digitalWrite(PIN_LED_DENY, LOW);
      toneOff();
      break;
    case ANN_WAIT:                                   // 1 Hz, silent
      digitalWrite(PIN_LED_GRANT, (t % 1000) < 500);
      digitalWrite(PIN_LED_DENY, LOW);
      toneOff();
      break;
    case ANN_GRANT:
      digitalWrite(PIN_LED_GRANT, HIGH);
      digitalWrite(PIN_LED_DENY, LOW);
      if (t < 80 || (t >= 160 && t < 240)) toneOn(2200); else toneOff();
      break;
    case ANN_DENY:
      digitalWrite(PIN_LED_GRANT, LOW);
      digitalWrite(PIN_LED_DENY, HIGH);
      if (t < 400) toneOn(440); else toneOff();
      break;
    case ANN_ALARM:                                  // 4 Hz, insistent
      digitalWrite(PIN_LED_GRANT, LOW);
      digitalWrite(PIN_LED_DENY, (t % 250) < 125);
      if ((t % 500) < 250) toneOn(880); else toneOff();
      break;
  }
}

}  // namespace

bool panelBegin() {
  pinMode(PIN_LED_GRANT, OUTPUT);
  pinMode(PIN_LED_DENY, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_LED_GRANT, LOW);
  digitalWrite(PIN_LED_DENY, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  for (uint8_t r = 0; r < 4; ++r) {
    pinMode(KEYPAD_ROW_PINS[r], OUTPUT);
    digitalWrite(KEYPAD_ROW_PINS[r], HIGH);
  }
  // No INPUT_PULLUP here: it is silently a no-op on GPIO 34–39, which is
  // exactly why those four columns carry discrete resistors.
  for (uint8_t c = 0; c < 4; ++c) pinMode(KEYPAD_COL_PINS[c], INPUT);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_HZ);

  for (uint8_t r = 0; r < LCD_ROWS; ++r) {
    memset(g_shadow[r], ' ', LCD_COLS); g_shadow[r][LCD_COLS] = 0;
    memset(g_want[r], ' ', LCD_COLS);   g_want[r][LCD_COLS] = 0;
  }

  g_lcdOk = lcdPresent();
  if (g_lcdOk) lcdInit();
  return g_lcdOk;
}

bool panelLcdOk() { return g_lcdOk; }

void panelBacklight(bool on) {
  g_backlight = on ? LCD_BL : 0;
  if (g_lcdOk) pcf(0);
}

void panelShow(const char* l0, const char* l1, const char* l2, const char* l3) {
  const char* in[4] = {l0, l1, l2, l3};
  for (uint8_t r = 0; r < LCD_ROWS; ++r) {
    char line[LCD_COLS + 1];
    memset(line, ' ', LCD_COLS);
    line[LCD_COLS] = 0;
    if (in[r]) {
      const size_t n = strnlen(in[r], LCD_COLS);
      memcpy(line, in[r], n);
    }
    if (memcmp(line, g_want[r], LCD_COLS) != 0) {
      memcpy(g_want[r], line, LCD_COLS + 1);
      g_dirty[r] = true;
    }
  }
}

void panelClear() { panelShow(nullptr, nullptr, nullptr, nullptr); }

char panelKey() {
  const char k = g_pending;
  g_pending = 0;
  return k;
}

void panelAnnunciate(Annunciation a) {
  if (a == g_ann) return;
  g_ann = a;
  g_annSince = millis();
  toneOff();
}

void panelTick() {
  const uint32_t now = millis();

  // One keypad row 
  if (now - g_lastScan >= 2) {
    g_lastScan = now;
    digitalWrite(KEYPAD_ROW_PINS[g_row], LOW);
    delayMicroseconds(30);                     // let the column settle through 10k
    char seen = 0;
    for (uint8_t c = 0; c < 4; ++c) {
      if (digitalRead(KEYPAD_COL_PINS[c]) == LOW) { seen = KEYPAD_MAP[g_row][c]; break; }
    }
    digitalWrite(KEYPAD_ROW_PINS[g_row], HIGH);

    if (seen) {
      g_seen = seen;
      if (seen != g_held) { g_held = seen; g_heldSince = now; }
      else if (g_heldSince && now - g_heldSince >= 12) {   // debounce, then latch
        g_pending = seen;
        g_heldSince = 0;                       // one event per press, no repeat
      }
    }

    // A key is released only when a whole sweep has found nothing; testing it
    // per row would report a release every time the scan moved off that row.
    g_row = (g_row + 1) & 3;
    if (g_row == 0) {
      if (!g_seen) { g_held = 0; g_heldSince = 0; }
      g_seen = 0;
    }
  }

  annTick();

  // LCD line
  if (!g_lcdOk) return;
  for (uint8_t r = 0; r < LCD_ROWS; ++r) {
    if (!g_dirty[r]) continue;
    lcdCmd(0x80 | rowAddr(r));
    for (uint8_t c = 0; c < LCD_COLS; ++c) lcdData(static_cast<uint8_t>(g_want[r][c]));
    memcpy(g_shadow[r], g_want[r], LCD_COLS + 1);
    g_dirty[r] = false;
    return;
  }
}
