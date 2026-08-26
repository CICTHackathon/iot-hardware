#include "latch.h"

#include <ESP32Servo.h>

namespace {

Servo    g_servo;
uint8_t  g_angle = 90;
uint8_t  g_target = 90;
DoorState g_state = DOOR_LOCKED;
uint32_t g_lastStep = 0;
uint32_t g_holdUntil = 0;
uint32_t g_holdMs = 0;
uint32_t g_trigUntil = 0;

}  // namespace

bool latchBegin() {
  pinMode(PIN_REC_TRIG, OUTPUT);
  digitalWrite(PIN_REC_TRIG, LOW);

  // Timers 2 and 3 are claimed for the servo, which leaves 0 and 1 to the LEDC
  // channel tone() takes for the buzzer. Sharing one timer between them makes
  // the servo twitch on every chirp.
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  g_servo.setPeriodHertz(50);
  if (!g_servo.attach(PIN_SERVO, 500, 2400)) return false;

  g_angle = g_target = g_cfg.closedAngle;
  g_servo.write(g_angle);
  g_state = DOOR_LOCKED;
  return true;
}

void latchOpen(uint32_t holdMs) {
  g_target = g_cfg.openAngle;
  g_holdMs = holdMs ? holdMs : g_cfg.openHoldMs;
  g_holdUntil = 0;                     // starts once the sweep has finished
  g_state = DOOR_OPENING;
}

void latchClose() {
  g_target = g_cfg.closedAngle;
  g_holdUntil = 0;
  g_state = DOOR_CLOSING;
}

DoorState latchState() { return g_state; }
uint8_t   latchAngle() { return g_angle; }

uint32_t latchHoldRemainingMs() {
  if (!g_holdUntil) return 0;
  const uint32_t now = millis();
  return (now < g_holdUntil) ? g_holdUntil - now : 0;
}

void recAssert(uint32_t ms) {
  digitalWrite(PIN_REC_TRIG, HIGH);
  const uint32_t until = millis() + ms;
  if (until > g_trigUntil) g_trigUntil = until;
}

void recRelease() {
  digitalWrite(PIN_REC_TRIG, LOW);
  g_trigUntil = 0;
}

bool recAsserted() { return g_trigUntil != 0; }

void latchTick() {
  const uint32_t now = millis();

  if (g_trigUntil && now >= g_trigUntil) recRelease();

  if (g_angle != g_target) {
    if (now - g_lastStep >= SERVO_STEP_MS) {
      g_lastStep = now;
      g_angle += (g_target > g_angle) ? 1 : -1;
      g_servo.write(g_angle);
      if (g_angle == g_target) {
        if (g_state == DOOR_OPENING) {
          g_state = DOOR_OPEN;
          g_holdUntil = now + g_holdMs;
        } else if (g_state == DOOR_CLOSING) {
          g_state = DOOR_LOCKED;
        }
      }
    }
    return;
  }

  // Already at the commanded angle: only the hold timer is left to run.
  if (g_state == DOOR_OPENING) { g_state = DOOR_OPEN; g_holdUntil = now + g_holdMs; }
  if (g_state == DOOR_CLOSING) g_state = DOOR_LOCKED;
  if (g_state == DOOR_OPEN && g_holdUntil && now >= g_holdUntil) latchClose();
}
