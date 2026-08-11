// Stage 2 of the cascade in inference against a user-supplied Edge Impulse model.
#pragma once

#include <Arduino.h>

namespace ml {

struct Result {
  float       score = 0.0f;     // best probability, 0 to 1
  const char* label = "";       // label carried by the impulse
  bool        hasBox = false;   // true for object-detection impulses
  uint16_t    x = 0, y = 0, w = 0, h = 0;  // box in model-input coordinates
  uint8_t     boxCount = 0;
};

bool        begin();
bool        available();
const char* status();      // "ready", "not built in", "library missing", etc.
bool        isObjectDetection();
int         inputWidth();
int         inputHeight();

// Runs one inference over an inputWidth() * inputHeight() RGB888 buffer.
bool infer(const uint8_t* rgb888, Result* out, uint16_t* latencyMs);

}  // namespace ml
