// The detection cascade and the capture task.
#pragma once

#include <time.h>

#include "app_config.h"

// stage 0: motion detection by frame differencing

struct MotionResult {
  bool     motion       = false;  // changed area exceeded the configured gate
  uint16_t changedCells = 0;
  float    areaFrac     = 0.0f;   // changedCells / GRID_CELLS
  float    fill         = 0.0f;   // changedCells / bounding-box area
  float    meanDelta    = 0.0f;   // mean |luminance delta| over changed cells
  uint8_t  x0 = 0, y0 = 0, x1 = 0, y1 = 0;  // bounding box, grid coordinates
  uint16_t decodeMs     = 0;
  uint16_t analyseMs    = 0;
};

// stage 1: logistic regression over the geometry of the motion mask

struct Features {
  float v[FEATURE_COUNT];
};
extern const char* const kFeatureNames[FEATURE_COUNT];

void  extractFeatures(const MotionResult& m, uint8_t persistence, Features* out);
float classifyScore(const Features& f);

// event log and stage 2: Edge Impulse model inference

enum Verdict : uint8_t {
  VERDICT_MOTION = 0,    // movement; stages beyond 0 not applied
  VERDICT_REJECTED_S1,   // geometry rejected it (lighting step, isolated noise)
  VERDICT_REJECTED_S2,   // the model rejected it
  VERDICT_CLASSIFIED,    // accepted by the full cascade, see mlLabel
};

struct DetectionEvent {
  uint32_t id;
  time_t   epoch;      // 0 until the clock has been set
  uint32_t uptimeMs;
  uint8_t  frames;     // consecutive positive samples at the time of decision
  float    stage1;
  float    stage2;      // -1 when the model was not run
  char     mlLabel[16]; // label reported by the impulse, empty if none
  uint8_t  x0, y0, x1, y1;
  int8_t   label;       // operator ground truth: -1 unlabelled, 0 no, 1 yes
  Verdict  verdict;
  Features feat;
  char     path[48];    // stored image, empty when nothing was written
};

// A working ring for the operator and for collecting training data - not the
// audit log. Per README §3 the authoritative record lives in the decision tier;
// anything retained only here is lost on reset, and is meant to be.
void        eventsAdd(const DetectionEvent& e);
bool        eventsLabel(uint32_t id, int8_t label);
size_t      eventsCount();
bool        eventsGet(size_t i, DetectionEvent* out);   // 0 = most recent
const char* verdictName(Verdict v);

// node status, for the web interface and the JSON API. The capture task updates
// this once per second, and the web handlers copy it under a lock.

struct NodeStatus {
  uint32_t frames = 0;       // frames captured
  uint32_t samples = 0;      // frames analysed
  uint32_t eventCount = 0;
  bool     eventOpen = false;
  uint8_t  persistence = 0;
  float    fps = 0.0f;
  float    lastStage1 = -1.0f;
  float    lastStage2 = -1.0f;
  uint16_t lastMlMs = 0;
  MotionResult lastMotion;
};

bool       detectBegin();
NodeStatus detectStatus();
void       detectCopyMask(uint8_t* dst);          // GRID_CELLS bytes
void       detectResetReference();                // after a sensor change

// Copies the most recent stage-2 input crop as RGB888. `dst` must hold
// ML_MAX_W * ML_MAX_H * 3 bytes; the actual geometry is returned in w and h.
bool       detectMlPreview(uint8_t* dst, int* w, int* h);

// Sink invoked from the capture task whenever an event is decided. This is the
// seam for the uplink described in README §5.2 (MQTT access requests, clip
// correlation); nothing in this firmware transmits an event by itself.
typedef void (*EventSink)(const DetectionEvent&);
void detectSetSink(EventSink sink);

void lampSet(uint8_t brightness);
