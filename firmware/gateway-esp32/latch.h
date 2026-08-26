#pragma once

#include <Arduino.h>

#include "app_config.h"

enum DoorState : uint8_t {
  DOOR_LOCKED = 0,
  DOOR_OPENING,
  DOOR_OPEN,
  DOOR_CLOSING,
};

bool latchBegin();

// Advances the sweep by at most one degree and services the hold and trigger
// timers. The sweep is stepped rather than commanded in one write so the SG90
// draws its stall current in a ramp instead of a step — a slam browns out the
// 5 V rail and takes the RC522 with it.
void latchTick();

void      latchOpen(uint32_t holdMs);   // sweeps open, then relocks by itself
void      latchClose();
DoorState latchState();
uint8_t   latchAngle();
uint32_t  latchHoldRemainingMs();

// Level-triggered recording request to the ESP32-CAM (README §4.3). Asserted
// before actuation so the clip's pre-roll contains the approach, not just the
// latch event.
void recAssert(uint32_t ms);
void recRelease();
bool recAsserted();
