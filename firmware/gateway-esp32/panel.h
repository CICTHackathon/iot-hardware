#pragma once

#include <Arduino.h>

#include "app_config.h"

enum Annunciation : uint8_t {
  ANN_OFF = 0,
  ANN_IDLE,      // both LEDs dark
  ANN_WAIT,      // green breathing: a decision is outstanding
  ANN_GRANT,     // green steady + two short chirps
  ANN_DENY,      // red steady + one long tone
  ANN_ALARM,     // red flashing + intermittent tone (tamper, lockout)
};

bool panelBegin();

// Bounded work, called from the main loop: scans one keypad row, advances the
// annunciator, and flushes at most one dirty LCD line. An HD44780 clear costs
// ~1.5 ms of settling and a full 20×4 repaint ~20 ms at 100 kHz; doing either
// inline would put that latency between a keypress and the FSM seeing it.
void panelTick();

// Repaint request. Lines are copied, padded and truncated to LCD_COLS; a line
// the glass does not have is discarded. Passing nullptr blanks that line.
void panelShow(const char* l0, const char* l1 = nullptr,
               const char* l2 = nullptr, const char* l3 = nullptr);
void panelClear();
void panelBacklight(bool on);

// Debounced, one key per press, 0 when nothing is waiting. '*' clears the entry
// and '#' submits it — the soft keys of the reference kit [4] are retained so
// operator muscle memory carries over.
char panelKey();

void panelAnnunciate(Annunciation a);

// False when the backpack did not acknowledge at LCD_I2C_ADDR. The node keeps
// enforcing without a display; it just cannot say anything to the operator.
bool panelLcdOk();
