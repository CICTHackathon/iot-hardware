#include "detect.h"

#include <esp_camera.h>
#include <esp_heap_caps.h>
#include <math.h>

#include "classifier_weights.h"
#include "img_converters.h"
#include "ml.h"
#include "storage.h"
#include "web.h"

// esp32-camera renamed the decoder scale enumerators between Arduino-ESP32 2.x
// and 3.x. Nothing about the decode changed, only the spelling, so the sketch
// keeps building on either core rather than pinning one.
#ifndef JPG_SCALE_8
#define JPG_SCALE_8 JPG_SCALE_8X
#endif

// Stage 0 in movement detection by frame differencing

namespace {

uint8_t* g_rgb  = nullptr;                 // 1/8-scale RGB565 scratch
uint8_t* g_gray = nullptr;                 // 1/8-scale luminance
uint8_t  g_grid[GRID_CELLS] = {0};
uint8_t  g_prev[GRID_CELLS] = {0};
uint8_t  g_mask[GRID_CELLS] = {0};
int      g_gw = 0, g_gh = 0;
bool     g_primed = false;

bool analyseFrame(const uint8_t* jpeg, size_t len, int frameW, int frameH, MotionResult* out) {
  if (!g_rgb || !g_gray || !jpeg || !len) return false;

  const int w = frameW / 8;
  const int h = frameH / 8;
  // 1/8 is the coarsest scale the esp32-camera JPEG decoder offers, so frame
  // sizes above UXGA cannot be analysed within the scratch buffer. Same ceiling
  // as upstream.
  if (w <= 0 || h <= 0 || w > GRAY_MAX_W || h > GRAY_MAX_H) return false;

  const uint32_t t0 = millis();
  if (!jpg2rgb565(jpeg, len, g_rgb, JPG_SCALE_8)) return false;
  const uint32_t t1 = millis();

  g_gw = w;
  g_gh = h;

  // jpg2rgb565() emits the high byte of each pixel first.
  for (int i = 0; i < w * h; ++i) {
    const uint8_t hi = g_rgb[2 * i], lo = g_rgb[2 * i + 1];
    const uint8_t r = hi & 0xF8;
    const uint8_t g = static_cast<uint8_t>(((hi & 0x07) << 5) | ((lo & 0xE0) >> 3));
    const uint8_t b = static_cast<uint8_t>((lo & 0x1F) << 3);
    g_gray[i] = static_cast<uint8_t>((77 * r + 150 * g + 29 * b) >> 8);  // BT.601
  }

  // Box-average into the fixed cell grid. Integer edge mapping keeps this
  // correct when the decoded image is smaller than the grid (QQVGA and below),
  // where cells simply repeat source pixels.
  for (int gy = 0; gy < GRID_H; ++gy) {
    const int sy0 = (gy * h) / GRID_H;
    const int sy1 = max(sy0 + 1, ((gy + 1) * h) / GRID_H);
    for (int gx = 0; gx < GRID_W; ++gx) {
      const int sx0 = (gx * w) / GRID_W;
      const int sx1 = max(sx0 + 1, ((gx + 1) * w) / GRID_W);
      uint32_t sum = 0, n = 0;
      for (int y = sy0; y < sy1; ++y) {
        const uint8_t* row = g_gray + y * w;
        for (int x = sx0; x < sx1; ++x) { sum += row[x]; ++n; }
      }
      g_grid[gy * GRID_W + gx] = static_cast<uint8_t>(sum / n);
    }
  }

  MotionResult r;
  r.decodeMs = static_cast<uint16_t>(t1 - t0);

  if (!g_primed) {
    memcpy(g_prev, g_grid, GRID_CELLS);
    memset(g_mask, 0, GRID_CELLS);
    g_primed = true;
    r.analyseMs = static_cast<uint16_t>(millis() - t1);
    *out = r;
    return true;
  }

  uint16_t changed = 0;
  uint32_t deltaSum = 0;
  uint8_t x0 = GRID_W, y0 = GRID_H, x1 = 0, y1 = 0;

  for (int gy = 0; gy < GRID_H; ++gy) {
    for (int gx = 0; gx < GRID_W; ++gx) {
      const int i = gy * GRID_W + gx;
      const int d = abs(static_cast<int>(g_grid[i]) - static_cast<int>(g_prev[i]));
      if (d >= g_cfg.cellDelta) {
        g_mask[i] = 1;
        ++changed;
        deltaSum += d;
        if (gx < x0) x0 = gx;
        if (gy < y0) y0 = gy;
        if (gx > x1) x1 = gx;
        if (gy > y1) y1 = gy;
      } else {
        g_mask[i] = 0;
      }
    }
  }
  memcpy(g_prev, g_grid, GRID_CELLS);

  r.changedCells = changed;
  r.areaFrac = static_cast<float>(changed) / GRID_CELLS;
  r.motion = (r.areaFrac * 100.0f) >= static_cast<float>(g_cfg.motionPercent);
  if (changed) {
    r.meanDelta = static_cast<float>(deltaSum) / changed;
    r.x0 = x0; r.y0 = y0; r.x1 = x1; r.y1 = y1;
    r.fill = changed / (static_cast<float>(x1 - x0 + 1) * static_cast<float>(y1 - y0 + 1));
  }
  r.analyseMs = static_cast<uint16_t>(millis() - t1);
  *out = r;
  return true;
}

// Nearest-neighbour crop of the 1/8-scale image around the motion bounding box,
// expanded to a square with margin, resampled to the model input as RGB888.
// Feeding the model a crop rather than the whole frame matters: a person
// occupying a fifth of a corridor frame is close to invisible after a full
// frame is squeezed into 96x96.
bool cropToModel(const MotionResult& m, uint8_t* dst, int dstW, int dstH) {
  if (!g_rgb || g_gw <= 0 || g_gh <= 0 || !dst) return false;

  const float fx0 = static_cast<float>(m.x0) * g_gw / GRID_W;
  const float fy0 = static_cast<float>(m.y0) * g_gh / GRID_H;
  const float fx1 = static_cast<float>(m.x1 + 1) * g_gw / GRID_W;
  const float fy1 = static_cast<float>(m.y1 + 1) * g_gh / GRID_H;

  const float cx = (fx0 + fx1) * 0.5f;
  const float cy = (fy0 + fy1) * 0.5f;
  float side = max(fx1 - fx0, fy1 - fy0) * 1.2f;   // 20% margin: impulses are
  side = max(side, 8.0f);                          // trained on framed subjects
  side = min(side, static_cast<float>(min(g_gw, g_gh)));

  const int s = static_cast<int>(side);
  const int sx = constrain(static_cast<int>(cx - side * 0.5f), 0, g_gw - s);
  const int sy = constrain(static_cast<int>(cy - side * 0.5f), 0, g_gh - s);

  for (int y = 0; y < dstH; ++y) {
    const int srcY = sy + (y * s) / dstH;
    for (int x = 0; x < dstW; ++x) {
      const int srcX = sx + (x * s) / dstW;
      const int i = srcY * g_gw + srcX;
      const uint8_t hi = g_rgb[2 * i], lo = g_rgb[2 * i + 1];
      uint8_t* o = dst + (y * dstW + x) * 3;
      o[0] = hi & 0xF8;
      o[1] = static_cast<uint8_t>(((hi & 0x07) << 5) | ((lo & 0xE0) >> 3));
      o[2] = static_cast<uint8_t>((lo & 0x1F) << 3);
    }
  }
  return true;
}

}  // namespace

// Stage 1 in logistic regression over the geometry of the motion mask

const char* const kFeatureNames[FEATURE_COUNT] = {
    "area", "fill", "aspect", "height", "centroid_y", "delta", "persistence", "global",
};

// Scaling lives here rather than in the trainer so that fitted weights stay
// interpretable and survive a change of grid geometry.
void extractFeatures(const MotionResult& m, uint8_t persistence, Features* out) {
  const float boxW = static_cast<float>(m.x1 - m.x0 + 1);
  const float boxH = static_cast<float>(m.y1 - m.y0 + 1);
  const float aspect = (m.changedCells == 0) ? 0.0f : boxH / boxW;

  out->v[0] = min(m.areaFrac / 0.25f, 2.0f);                 // area
  out->v[1] = m.fill;                                        // fill
  out->v[2] = min(aspect / 2.0f, 1.5f);                      // aspect
  out->v[3] = boxH / GRID_H;                                 // height
  out->v[4] = (m.y0 + m.y1) * 0.5f / GRID_H;                 // centroid_y
  out->v[5] = min(m.meanDelta / 64.0f, 2.0f);                // delta
  out->v[6] = min(persistence / 8.0f, 1.0f);                 // persistence
  out->v[7] = (m.areaFrac > 0.45f) ? 1.0f : 0.0f;            // global change
}

float classifyScore(const Features& f) {
  float z = kClassifierBias;
  for (int i = 0; i < FEATURE_COUNT; ++i) z += kClassifierWeights[i] * f.v[i];
  return 1.0f / (1.0f + expf(-z));
}

// Event ring
namespace {
DetectionEvent g_ring[EVENT_LOG_SIZE];
size_t g_head = 0, g_used = 0;
SemaphoreHandle_t g_evLock = nullptr;

struct EvGuard {
  EvGuard() { if (g_evLock) xSemaphoreTake(g_evLock, portMAX_DELAY); }
  ~EvGuard() { if (g_evLock) xSemaphoreGive(g_evLock); }
};
}  // namespace

void eventsAdd(const DetectionEvent& e) {
  EvGuard g;
  g_ring[g_head] = e;
  g_head = (g_head + 1) % EVENT_LOG_SIZE;
  if (g_used < EVENT_LOG_SIZE) ++g_used;
}

bool eventsLabel(uint32_t id, int8_t label) {
  EvGuard g;
  for (size_t i = 0; i < g_used; ++i) {
    if (g_ring[i].id == id) { g_ring[i].label = label; return true; }
  }
  return false;
}

size_t eventsCount() { EvGuard g; return g_used; }

bool eventsGet(size_t i, DetectionEvent* out) {
  EvGuard g;
  if (i >= g_used || !out) return false;
  *out = g_ring[(g_head + EVENT_LOG_SIZE - 1 - i) % EVENT_LOG_SIZE];
  return true;
}

const char* verdictName(Verdict v) {
  switch (v) {
    case VERDICT_MOTION:      return "motion";
    case VERDICT_REJECTED_S1: return "rejected-geometry";
    case VERDICT_REJECTED_S2: return "rejected-model";
    case VERDICT_CLASSIFIED:  return "classified";
  }
  return "?";
}

// Capture task and cascade
namespace {

NodeStatus g_status;
SemaphoreHandle_t g_statusLock = nullptr;
uint8_t  g_pubMask[GRID_CELLS] = {0};
uint8_t* g_mlBuf = nullptr;      // model input crop, RGB888
int      g_mlW = 0, g_mlH = 0;   // geometry of the crop currently held
EventSink g_sink = nullptr;
uint32_t g_nextEventId = 1;
uint32_t g_clearRun = 0;
uint32_t g_fpsStart = 0, g_fpsFrames = 0;

// The LEDC API was reworked in Arduino-ESP32 3.x: channels are no longer
// allocated by the sketch and ledcWrite() addresses the pin directly.
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
#define LAMP_ATTACH() ledcAttach(CAM_PIN_LAMP, 5000, 8)
#define LAMP_WRITE(v) ledcWrite(CAM_PIN_LAMP, (v))
#else
#define LAMP_ATTACH()                                 \
  do {                                                \
    ledcSetup(LAMP_LEDC_CHANNEL, 5000, 8);            \
    ledcAttachPin(CAM_PIN_LAMP, LAMP_LEDC_CHANNEL);   \
  } while (0)
#define LAMP_WRITE(v) ledcWrite(LAMP_LEDC_CHANNEL, (v))
#endif

void publishStatus(const NodeStatus& s, const uint8_t* mask) {
  if (xSemaphoreTake(g_statusLock, pdMS_TO_TICKS(50)) != pdTRUE) return;
  g_status = s;
  if (mask) memcpy(g_pubMask, mask, GRID_CELLS);
  xSemaphoreGive(g_statusLock);
}

// Runs stages 1 and 2 and files the event. Called once per opened event, not
// once per sample: an event is a transit, and a transit that produced forty
// samples is still one thing that happened.
void decide(const MotionResult& m, const camera_fb_t* fb, NodeStatus* s) {
  DetectionEvent e = {};
  e.id = g_nextEventId++;
  e.uptimeMs = millis();
  // 0 until NTP has landed. A zero here is not a missing nicety: the console
  // joins this event to a credential event on the epoch and on nothing else
  // (README §4.3), so an event stamped 1970 is an event that cannot be
  // correlated, and it says so rather than pretending otherwise.
  e.epoch = clockSet() ? time(nullptr) : 0;
  e.frames = s->persistence;
  e.x0 = m.x0; e.y0 = m.y0; e.x1 = m.x1; e.y1 = m.y1;
  e.label = -1;
  e.stage2 = -1.0f;

  extractFeatures(m, s->persistence, &e.feat);
  e.stage1 = classifyScore(e.feat);

  if (e.stage1 * 100.0f < g_cfg.classifierPercent) {
    e.verdict = VERDICT_REJECTED_S1;
  } else if (g_cfg.mlUse && ml::available() && g_mlBuf &&
             cropToModel(m, g_mlBuf, ml::inputWidth(), ml::inputHeight())) {
    g_mlW = ml::inputWidth();
    g_mlH = ml::inputHeight();
    ml::Result r;
    uint16_t ms = 0;
    if (ml::infer(g_mlBuf, &r, &ms)) {
      e.stage2 = r.score;
      s->lastMlMs = ms;
      strncpy(e.mlLabel, r.label ? r.label : "", sizeof(e.mlLabel) - 1);
      e.verdict = (r.score * 100.0f >= g_cfg.mlProbability) ? VERDICT_CLASSIFIED
                                                            : VERDICT_REJECTED_S2;
    } else {
      e.verdict = VERDICT_MOTION;
    }
  } else {
    // No second opinion available: report movement as movement rather than
    // asserting a classification the node cannot make.
    e.verdict = VERDICT_MOTION;
  }

  if (g_cfg.saveEvents && e.verdict != VERDICT_REJECTED_S1 && fb) {
    const String p = storageSaveEvent(e, fb->buf, fb->len);
    strncpy(e.path, p.c_str(), sizeof(e.path) - 1);
  }

  s->lastStage1 = e.stage1;
  s->lastStage2 = e.stage2;
  s->eventCount++;

  eventsAdd(e);
  if (g_sink) g_sink(e);

  Serial.printf("[evt %u] %s %s area=%.3f s1=%.2f s2=%.2f box=%u,%u-%u,%u %s\n",
                e.id, verdictName(e.verdict), e.mlLabel, m.areaFrac, e.stage1,
                e.stage2, e.x0, e.y0, e.x1, e.y1, e.path);
}

void captureTask(void*) {
  NodeStatus s;
  g_fpsStart = millis();

  for (;;) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }

    webPublishFrame(fb->buf, fb->len);
    s.frames++;
    g_fpsFrames++;

    const uint32_t nowMs = millis();
    if (nowMs - g_fpsStart >= 1000) {
      s.fps = (g_fpsFrames * 1000.0f) / (nowMs - g_fpsStart);
      g_fpsStart = nowMs;
      g_fpsFrames = 0;
    }

    // 1-in-N sampling, as upstream: sample aggressively while idle and back off
    // once an event is open, so the stream keeps its frame rate during the part
    // of the transit that is actually being watched.
    const uint8_t every = s.eventOpen ? max<uint8_t>(1, g_cfg.sampleActive)
                                      : max<uint8_t>(1, g_cfg.sampleIdle);
    if (s.frames % every == 0) {
      MotionResult m;
      if (analyseFrame(fb->buf, fb->len, fb->width, fb->height, &m)) {
        s.samples++;
        s.lastMotion = m;

        if (m.motion) {
          g_clearRun = 0;
          if (s.persistence < 255) s.persistence++;
          if (!s.eventOpen && s.persistence >= max<uint8_t>(1, g_cfg.minFrames)) {
            s.eventOpen = true;
            if (g_cfg.lampOnEvent) lampSet(g_cfg.lampBrightness);
            decide(m, fb, &s);
          }
        } else if (s.eventOpen && ++g_clearRun >= max<uint8_t>(1, g_cfg.clearFrames)) {
          s.eventOpen = false;
          s.persistence = 0;
          if (g_cfg.lampOnEvent) lampSet(0);
          Serial.printf("[evt] closed after %u quiet samples\n",
                        static_cast<unsigned>(g_clearRun));
        } else if (!s.eventOpen) {
          s.persistence = 0;
        }
        publishStatus(s, g_mask);
      }
    } else {
      publishStatus(s, nullptr);
    }

    esp_camera_fb_return(fb);
    vTaskDelay(pdMS_TO_TICKS(5));   // never starve the HTTP tasks
  }
}

}  // namespace

void lampSet(uint8_t brightness) { LAMP_WRITE(brightness); }

bool detectBegin() {
  g_statusLock = xSemaphoreCreateMutex();
  g_evLock = xSemaphoreCreateMutex();
  if (!g_statusLock || !g_evLock) return false;

  g_rgb  = static_cast<uint8_t*>(heap_caps_malloc(GRAY_MAX_W * GRAY_MAX_H * 2, MALLOC_CAP_SPIRAM));
  g_gray = static_cast<uint8_t*>(heap_caps_malloc(GRAY_MAX_W * GRAY_MAX_H, MALLOC_CAP_SPIRAM));
  g_mlBuf = static_cast<uint8_t*>(heap_caps_malloc(ML_MAX_W * ML_MAX_H * 3, MALLOC_CAP_SPIRAM));
  if (!g_rgb || !g_gray) {
    Serial.println("[det] motion buffers unavailable - is PSRAM enabled?");
    return false;
  }

  LAMP_ATTACH();
  lampSet(0);

  // Pinned to core 1, with the HTTP servers left on core 0: an inference that
  // runs for most of a second must not sit in the same run queue as the TCP
  // stack, or the stream stalls every time something walks past.
  return xTaskCreatePinnedToCore(captureTask, "capture", 8192, nullptr, 4, nullptr, 1) == pdPASS;
}

NodeStatus detectStatus() {
  NodeStatus copy;
  if (xSemaphoreTake(g_statusLock, pdMS_TO_TICKS(100)) == pdTRUE) {
    copy = g_status;
    xSemaphoreGive(g_statusLock);
  }
  return copy;
}

void detectCopyMask(uint8_t* dst) {
  if (!dst) return;
  if (xSemaphoreTake(g_statusLock, pdMS_TO_TICKS(100)) == pdTRUE) {
    memcpy(dst, g_pubMask, GRID_CELLS);
    xSemaphoreGive(g_statusLock);
  } else {
    memset(dst, 0, GRID_CELLS);
  }
}

void detectResetReference() { g_primed = false; }

bool detectMlPreview(uint8_t* dst, int* w, int* h) {
  if (!g_mlBuf || !dst || g_mlW <= 0) return false;
  memcpy(dst, g_mlBuf, static_cast<size_t>(g_mlW) * g_mlH * 3);
  *w = g_mlW;
  *h = g_mlH;
  return true;
}

void detectSetSink(EventSink sink) { g_sink = sink; }
