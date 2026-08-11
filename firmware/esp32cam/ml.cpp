#include "ml.h"

#include "app_config.h"

// The library is only pulled in when it is both wanted and installed, so a
// checkout without a model still compiles and runs (stages 0 and 1 only).
#if INCLUDE_TINYML && __has_include(TINY_ML_LIB)
#include TINY_ML_LIB
#define ML_ACTIVE 1
#if !defined(EI_CLASSIFIER_INPUT_WIDTH)
#error "TINY_ML_LIB is not an image impulse: no EI_CLASSIFIER_INPUT_WIDTH"
#endif
#endif

#ifdef ML_ACTIVE

namespace {

const char* g_status = "not started";
bool  g_ready = false;
const uint8_t* g_frame = nullptr;   // borrowed for the duration of one inference

// Edge Impulse pulls signal data through a callback rather than taking a
// buffer, so the DSP block can stream a window without a second copy. Pixels
// are handed over packed as 0xRRGGBB in a float, which is what the image DSP
// block expects; a greyscale impulse converts internally, so feeding true
// colour here costs nothing and helps colour impulses.
int signalGetData(size_t offset, size_t length, float* out) {
  if (!g_frame) return EIDSP_REQUESTED_OUT_OF_BOUNDS;
  for (size_t i = 0; i < length; ++i) {
    const uint8_t* p = g_frame + (offset + i) * 3;
    out[i] = static_cast<float>((static_cast<uint32_t>(p[0]) << 16) |
                                (static_cast<uint32_t>(p[1]) << 8) |
                                static_cast<uint32_t>(p[2]));
  }
  return EIDSP_OK;
}

}  // namespace

namespace ml {

bool begin() {
  if (g_ready) return true;
  if (EI_CLASSIFIER_INPUT_WIDTH > ML_MAX_W || EI_CLASSIFIER_INPUT_HEIGHT > ML_MAX_H) {
    g_status = "impulse larger than ML_MAX_W/H";
    return false;
  }
  Serial.printf("[ml] %s, %dx%d, %s\n", EI_CLASSIFIER_PROJECT_NAME,
                EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT,
                isObjectDetection() ? "object detection" : "classification");
  g_status = "ready";
  g_ready = true;
  return true;
}

bool available() { return g_ready; }
const char* status() { return g_status; }
int inputWidth() { return EI_CLASSIFIER_INPUT_WIDTH; }
int inputHeight() { return EI_CLASSIFIER_INPUT_HEIGHT; }

bool isObjectDetection() {
#if defined(EI_CLASSIFIER_OBJECT_DETECTION) && EI_CLASSIFIER_OBJECT_DETECTION == 1
  return true;
#else
  return false;
#endif
}

bool infer(const uint8_t* rgb888, Result* out, uint16_t* latencyMs) {
  if (!g_ready || !rgb888 || !out) return false;

  const uint32_t t0 = millis();
  g_frame = rgb888;

  signal_t signal;
  signal.total_length = static_cast<size_t>(EI_CLASSIFIER_INPUT_WIDTH) *
                        static_cast<size_t>(EI_CLASSIFIER_INPUT_HEIGHT);
  signal.get_data = &signalGetData;

  ei_impulse_result_t result = {0};
  const EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
  g_frame = nullptr;

  if (err != EI_IMPULSE_OK) {
    g_status = "run_classifier failed";
    return false;
  }

  Result r;
#if defined(EI_CLASSIFIER_OBJECT_DETECTION) && EI_CLASSIFIER_OBJECT_DETECTION == 1
  // FOMO returns a variable number of boxes; the strongest one decides, and its
  // geometry is kept so the operator can see what the model latched onto.
  for (uint32_t i = 0; i < result.bounding_boxes_count; ++i) {
    const auto& bb = result.bounding_boxes[i];
    if (bb.value <= 0) continue;
    r.boxCount++;
    if (bb.value > r.score) {
      r.score = bb.value;
      r.label = bb.label;
      r.hasBox = true;
      r.x = bb.x; r.y = bb.y; r.w = bb.width; r.h = bb.height;
    }
  }
#else
  for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; ++i) {
    if (result.classification[i].value > r.score) {
      r.score = result.classification[i].value;
      r.label = result.classification[i].label;
    }
  }
#endif

  *out = r;
  if (latencyMs) *latencyMs = static_cast<uint16_t>(millis() - t0);
  return true;
}

}  // namespace ml

#else  // no model library

namespace ml {
bool begin() { return false; }
bool available() { return false; }
bool isObjectDetection() { return false; }
int  inputWidth() { return 96; }
int  inputHeight() { return 96; }
bool infer(const uint8_t*, Result*, uint16_t*) { return false; }
const char* status() {
#if INCLUDE_TINYML
  return "library " TINY_ML_LIB " not installed";
#else
  return "not built in (INCLUDE_TINYML false)";
#endif
}
}  // namespace ml

#endif
