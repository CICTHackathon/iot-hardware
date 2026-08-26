#include "web.h"

#include <Preferences.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

#include "classifier_weights.h"
#include "detect.h"
#include "ml.h"
#include "storage.h"
#include "console_ui.h"

// Boundary token carried over from the ESP32WebCam reference implementation.
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

namespace {

httpd_handle_t g_ui = nullptr;
httpd_handle_t g_stream = nullptr;

// Frame bus for the capture task publishes the latest JPEG frame, and HTTP handlers
SemaphoreHandle_t g_frameLock = nullptr;
uint8_t* g_frameBuf = nullptr;
size_t   g_frameCap = 0, g_frameLen = 0;
volatile uint32_t g_frameSeq = 0;
volatile int g_streamClients = 0;

void* psramGrow(void* p, size_t want) {
  void* n = heap_caps_realloc(p, want, MALLOC_CAP_SPIRAM);
  if (!n) n = heap_caps_realloc(p, want, MALLOC_CAP_8BIT);   // board without PSRAM
  return n;
}

// Small helpers for the HTTP handlers to parse a query string and return the value of a key.
bool queryValue(httpd_req_t* req, const char* key, char* out, size_t len) {
  const size_t qlen = httpd_req_get_url_query_len(req) + 1;
  if (qlen <= 1) return false;
  char* q = static_cast<char*>(malloc(qlen));
  if (!q) return false;
  bool found = false;
  if (httpd_req_get_url_query_str(req, q, qlen) == ESP_OK) {
    found = httpd_query_key_value(q, key, out, len) == ESP_OK;
  }
  free(q);
  return found;
}

bool queryInt(httpd_req_t* req, const char* key, long* out) {
  char v[24];
  if (!queryValue(req, key, v, sizeof(v))) return false;
  char* end = nullptr;
  const long n = strtol(v, &end, 10);
  if (end == v) return false;
  *out = n;
  return true;
}

esp_err_t sendJson(httpd_req_t* req, const char* json) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_sendstr(req, json);
}

void isoTime(time_t epoch, uint32_t uptimeMs, char* out, size_t len) {
  if (epoch) {
    struct tm tmv;
    localtime_r(&epoch, &tmv);
    strftime(out, len, "%Y-%m-%d %H:%M:%S", &tmv);
  } else {
    snprintf(out, len, "+%lus", static_cast<unsigned long>(uptimeMs / 1000));
  }
}

// Handler for the root page, which is the web UI. The HTML is embedded in the binary as a string.

esp_err_t indexHandler(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, CONSOLE_HTML, HTTPD_RESP_USE_STRLEN);
}

esp_err_t statusHandler(httpd_req_t* req) {
  const NodeStatus s = detectStatus();
  uint8_t mask[GRID_CELLS];
  detectCopyMask(mask);

  char* buf = static_cast<char*>(malloc(GRID_CELLS + 704));
  if (!buf) return httpd_resp_send_500(req);

  int n = snprintf(buf, 684,
      "{\"ip\":\"%s\",\"uptime\":%lu,\"epoch\":%lld,\"clockSet\":%s,"
      "\"fps\":%.1f,\"frames\":%lu,\"samples\":%lu,"
      "\"events\":%lu,\"eventOpen\":%s,\"persistence\":%u,\"area\":%.4f,"
      "\"changed\":%u,\"x0\":%u,\"y0\":%u,\"x1\":%u,\"y1\":%u,"
      "\"s1\":%.3f,\"s2\":%.3f,\"decodeMs\":%u,\"analyseMs\":%u,\"mlMs\":%u,"
      "\"gw\":%d,\"gh\":%d,\"heap\":%lu,\"psram\":%lu,\"store\":\"%s\","
      "\"storeUsed\":%llu,\"storeTotal\":%llu,\"mask\":\"",
      WiFi.isConnected() ? WiFi.localIP().toString().c_str()
                         : WiFi.softAPIP().toString().c_str(),
      static_cast<unsigned long>(millis() / 1000),
      // This node is not wired to the enforcement node, so the epoch is the
      // only thing the console can join its events on (README §4.3). Reporting
      // the clock as status is what lets §5.2 step 4 be a check rather than a
      // guess, and what lets the Timeline tab say the join is unavailable
      // before any event has been recorded.
      static_cast<long long>(time(nullptr)), clockSet() ? "true" : "false", s.fps,
      static_cast<unsigned long>(s.frames), static_cast<unsigned long>(s.samples),
      static_cast<unsigned long>(s.eventCount), s.eventOpen ? "true" : "false",
      s.persistence, s.lastMotion.areaFrac, s.lastMotion.changedCells,
      s.lastMotion.x0, s.lastMotion.y0, s.lastMotion.x1, s.lastMotion.y1,
      s.lastStage1, s.lastStage2, s.lastMotion.decodeMs, s.lastMotion.analyseMs,
      s.lastMlMs, GRID_W, GRID_H,
      static_cast<unsigned long>(ESP.getFreeHeap()),
      static_cast<unsigned long>(ESP.getFreePsram() / 1024),
      storageKindName(), storageUsedBytes(), storageTotalBytes());

  for (int i = 0; i < GRID_CELLS; ++i) buf[n++] = mask[i] ? '1' : '0';
  n += snprintf(buf + n, 8, "\"}");

  httpd_resp_set_type(req, "application/json");
  // Read by the console when it is being served by the other node.
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  const esp_err_t r = httpd_resp_send(req, buf, n);
  free(buf);
  return r;
}

esp_err_t configGetHandler(httpd_req_t* req) {
  char json[768];
  snprintf(json, sizeof(json),
      "{\"frameSize\":%u,\"jpegQuality\":%u,\"hMirror\":%u,\"vFlip\":%u,"
      "\"cellDelta\":%u,\"motionPercent\":%u,\"sampleIdle\":%u,\"sampleActive\":%u,"
      "\"minFrames\":%u,\"clearFrames\":%u,\"classifierPercent\":%u,"
      "\"mlUse\":%u,\"mlProbability\":%u,\"saveEvents\":%u,\"lampOnEvent\":%u,"
      "\"lampBrightness\":%u,\"classifierOrigin\":\"%s\",\"mlStatus\":\"%s\","
      "\"mlInput\":\"%dx%d\",\"mlKind\":\"%s\"}",
      g_cfg.frameSize, g_cfg.jpegQuality, g_cfg.hMirror, g_cfg.vFlip,
      g_cfg.cellDelta, g_cfg.motionPercent, g_cfg.sampleIdle, g_cfg.sampleActive,
      g_cfg.minFrames, g_cfg.clearFrames, g_cfg.classifierPercent,
      g_cfg.mlUse, g_cfg.mlProbability, g_cfg.saveEvents, g_cfg.lampOnEvent,
      g_cfg.lampBrightness, CLASSIFIER_ORIGIN, ml::status(),
      ml::inputWidth(), ml::inputHeight(),
      ml::isObjectDetection() ? "object detection" : "classification");
  return sendJson(req, json);
}

esp_err_t configPostHandler(httpd_req_t* req) {
  long v = 0;
  bool sensorDirty = false;

  if (queryInt(req, "frameSize", &v))   { g_cfg.frameSize = constrain(v, 0, FRAMESIZE_UXGA); sensorDirty = true; }
  if (queryInt(req, "jpegQuality", &v)) { g_cfg.jpegQuality = constrain(v, 10, 63); sensorDirty = true; }
  if (queryInt(req, "hMirror", &v))     { g_cfg.hMirror = v != 0; sensorDirty = true; }
  if (queryInt(req, "vFlip", &v))       { g_cfg.vFlip = v != 0; sensorDirty = true; }
  if (queryInt(req, "cellDelta", &v))          g_cfg.cellDelta = constrain(v, 1, 200);
  if (queryInt(req, "motionPercent", &v))      g_cfg.motionPercent = constrain(v, 1, 100);
  if (queryInt(req, "sampleIdle", &v))         g_cfg.sampleIdle = constrain(v, 1, 60);
  if (queryInt(req, "sampleActive", &v))       g_cfg.sampleActive = constrain(v, 1, 60);
  if (queryInt(req, "minFrames", &v))          g_cfg.minFrames = constrain(v, 1, 60);
  if (queryInt(req, "clearFrames", &v))        g_cfg.clearFrames = constrain(v, 1, 200);
  if (queryInt(req, "classifierPercent", &v))  g_cfg.classifierPercent = constrain(v, 0, 100);
  if (queryInt(req, "mlProbability", &v))      g_cfg.mlProbability = constrain(v, 0, 100);
  if (queryInt(req, "mlUse", &v))              g_cfg.mlUse = (v != 0) && ml::available();
  if (queryInt(req, "saveEvents", &v))         g_cfg.saveEvents = v != 0;
  if (queryInt(req, "lampOnEvent", &v))        g_cfg.lampOnEvent = v != 0;
  if (queryInt(req, "lampBrightness", &v)) {
    g_cfg.lampBrightness = constrain(v, 0, 255);
    if (!g_cfg.lampOnEvent) lampSet(g_cfg.lampBrightness);   // live preview
  }

  if (sensorDirty) {
    configApplySensor();
    // The reference frame belongs to the old geometry; keeping it would report
    // the resolution change itself as movement.
    detectResetReference();
  }
  configSave();
  return sendJson(req, "{\"ok\":true}");
}

esp_err_t eventsHandler(httpd_req_t* req) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_sendstr_chunk(req, "[");

  const size_t n = eventsCount();
  char item[480], when[32];
  for (size_t i = 0; i < n; ++i) {
    DetectionEvent e;
    if (!eventsGet(i, &e)) break;
    isoTime(e.epoch, e.uptimeMs, when, sizeof(when));
    snprintf(item, sizeof(item),
             "%s{\"id\":%u,\"time\":\"%s\",\"epoch\":%lld,\"uptime\":%lu,"
             "\"verdict\":\"%s\",\"mlLabel\":\"%s\","
             "\"s1\":%.3f,\"s2\":%.3f,\"x0\":%u,\"y0\":%u,\"x1\":%u,\"y1\":%u,"
             "\"frames\":%u,\"label\":%d,\"path\":\"%s\"}",
             i ? "," : "", e.id, when, static_cast<long long>(e.epoch),
             static_cast<unsigned long>(e.uptimeMs / 1000),
             verdictName(e.verdict), e.mlLabel,
             e.stage1, e.stage2, e.x0, e.y0, e.x1, e.y1, e.frames, e.label, e.path);
    httpd_resp_sendstr_chunk(req, item);
  }
  httpd_resp_sendstr_chunk(req, "]");
  return httpd_resp_sendstr_chunk(req, nullptr);
}

esp_err_t labelHandler(httpd_req_t* req) {
  long id = 0, label = 0;
  if (!queryInt(req, "id", &id) || !queryInt(req, "label", &label)) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "id and label required");
    return ESP_FAIL;
  }
  const bool ok = eventsLabel(static_cast<uint32_t>(id),
                              static_cast<int8_t>(constrain(label, -1, 1)));
  return sendJson(req, ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

// Ground truth for retraining stage 1. Only labelled events are emitted: an
// unlabelled row is not a negative example, it is an unexamined one.
esp_err_t datasetHandler(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/csv");
  httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=dataset.csv");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  char line[320];
  int n = snprintf(line, sizeof(line), "id,time,verdict,stage1,stage2,ml_label,label");
  for (int i = 0; i < FEATURE_COUNT; ++i)
    n += snprintf(line + n, sizeof(line) - n, ",%s", kFeatureNames[i]);
  snprintf(line + n, sizeof(line) - n, "\n");
  httpd_resp_sendstr_chunk(req, line);

  const size_t count = eventsCount();
  char when[32];
  for (size_t i = 0; i < count; ++i) {
    DetectionEvent e;
    if (!eventsGet(i, &e)) break;
    if (e.label < 0) continue;
    isoTime(e.epoch, e.uptimeMs, when, sizeof(when));
    n = snprintf(line, sizeof(line), "%u,%s,%s,%.4f,%.4f,%s,%d",
                 e.id, when, verdictName(e.verdict), e.stage1, e.stage2, e.mlLabel, e.label);
    for (int k = 0; k < FEATURE_COUNT; ++k)
      n += snprintf(line + n, sizeof(line) - n, ",%.4f", e.feat.v[k]);
    snprintf(line + n, sizeof(line) - n, "\n");
    httpd_resp_sendstr_chunk(req, line);
  }
  return httpd_resp_sendstr_chunk(req, nullptr);
}

esp_err_t captureHandler(httpd_req_t* req) {
  uint8_t* buf = nullptr;
  size_t cap = 0;
  uint32_t seq = g_frameSeq - 1;   // accept the frame already held
  const size_t len = webTakeFrame(&buf, &cap, &seq, 3000);
  if (!len) {
    free(buf);
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no frame");
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  const esp_err_t r = httpd_resp_send(req, reinterpret_cast<const char*>(buf), len);
  free(buf);
  return r;
}

// The crop the model was last given, as a 24-bit BMP. Seeing the network's
// actual input is the fastest way to find out that a detector is being fed a
// wall, and BMP avoids a re-encode on a device already busy encoding JPEG.
esp_err_t mlPreviewHandler(httpd_req_t* req) {
  uint8_t* rgb = static_cast<uint8_t*>(malloc(ML_MAX_W * ML_MAX_H * 3));
  int w = 0, h = 0;
  if (!rgb || !detectMlPreview(rgb, &w, &h)) {
    free(rgb);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "no inference yet");
    return ESP_FAIL;
  }

  const int pad = (4 - (w * 3) % 4) % 4;
  const uint32_t rowBytes = w * 3 + pad;
  const uint32_t pixBytes = rowBytes * h;
  const uint32_t fileBytes = 54 + pixBytes;

  uint8_t* bmp = static_cast<uint8_t*>(malloc(fileBytes));
  if (!bmp) {
    free(rgb);
    return httpd_resp_send_500(req);
  }
  memset(bmp, 0, 54);
  bmp[0] = 'B'; bmp[1] = 'M';
  memcpy(bmp + 2, &fileBytes, 4);
  const uint32_t off = 54, hdr = 40;
  memcpy(bmp + 10, &off, 4);
  memcpy(bmp + 14, &hdr, 4);
  memcpy(bmp + 18, &w, 4);
  memcpy(bmp + 22, &h, 4);
  const uint16_t planes = 1, bpp = 24;
  memcpy(bmp + 26, &planes, 2);
  memcpy(bmp + 28, &bpp, 2);
  memcpy(bmp + 34, &pixBytes, 4);

  for (int y = 0; y < h; ++y) {
    const uint8_t* src = rgb + static_cast<size_t>(y) * w * 3;
    uint8_t* dst = bmp + 54 + static_cast<size_t>(h - 1 - y) * rowBytes;  // bottom-up
    for (int x = 0; x < w; ++x) {
      dst[x * 3 + 0] = src[x * 3 + 2];   // BMP is BGR
      dst[x * 3 + 1] = src[x * 3 + 1];
      dst[x * 3 + 2] = src[x * 3 + 0];
    }
  }
  free(rgb);

  httpd_resp_set_type(req, "image/bmp");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  const esp_err_t r = httpd_resp_send(req, reinterpret_cast<const char*>(bmp), fileBytes);
  free(bmp);
  return r;
}

esp_err_t wifiHandler(httpd_req_t* req) {
  char ssid[64] = {0}, pass[80] = {0};
  if (!queryValue(req, "ssid", ssid, sizeof(ssid))) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "ssid required");
    return ESP_FAIL;
  }
  queryValue(req, "pass", pass, sizeof(pass));

  Preferences p;
  p.begin("esp32cam", false);
  p.putString("ssid", urlDecode(ssid));
  p.putString("pass", urlDecode(pass));
  p.end();

  sendJson(req, "{\"ok\":true,\"restarting\":true}");
  delay(300);
  ESP.restart();
  return ESP_OK;
}

esp_err_t rebootHandler(httpd_req_t* req) {
  sendJson(req, "{\"ok\":true}");
  delay(300);
  ESP.restart();
  return ESP_OK;
}

// --- MJPEG stream (port 81) ------------------------------------------------

esp_err_t streamHandler(httpd_req_t* req) {
  if (g_streamClients >= MAX_STREAM_CLIENTS) {
    // esp_http_server has no 503 in httpd_err_code_t, so it is written out
    // directly rather than being downgraded to something misleading.
    httpd_resp_set_status(req, "503 Service Unavailable");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "too many stream clients");
    return ESP_FAIL;
  }
  g_streamClients++;

  esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  httpd_resp_set_hdr(req, "X-Framerate", "60");

  uint8_t* buf = nullptr;
  size_t cap = 0;
  uint32_t seq = 0;
  char part[80];

  while (res == ESP_OK) {
    const size_t len = webTakeFrame(&buf, &cap, &seq, 5000);
    if (!len) {                      // producer stalled; keep the socket alive
      if (!g_frameSeq) break;
      continue;
    }
    res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    if (res == ESP_OK) {
      const int n = snprintf(part, sizeof(part), STREAM_PART, static_cast<unsigned>(len));
      res = httpd_resp_send_chunk(req, part, n);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, reinterpret_cast<const char*>(buf), len);
    }
  }

  free(buf);
  g_streamClients--;
  return res;
}

void registerUri(httpd_handle_t s, const char* uri, httpd_method_t m, esp_err_t (*h)(httpd_req_t*)) {
  httpd_uri_t u = {};
  u.uri = uri;
  u.method = m;
  u.handler = h;
  httpd_register_uri_handler(s, &u);
}

}  // namespace

// --- frame bus, public side ------------------------------------------------

void webPublishFrame(const uint8_t* jpeg, size_t len) {
  if (!g_frameLock || !jpeg || !len) return;
  if (xSemaphoreTake(g_frameLock, pdMS_TO_TICKS(200)) != pdTRUE) return;

  if (len > g_frameCap) {
    // Round up so a slowly growing JPEG does not realloc on every frame.
    const size_t want = ((len * 5) / 4 + 1023) & ~static_cast<size_t>(1023);
    void* n = psramGrow(g_frameBuf, want);
    if (n) {
      g_frameBuf = static_cast<uint8_t*>(n);
      g_frameCap = want;
    }
  }
  if (g_frameBuf && len <= g_frameCap) {
    memcpy(g_frameBuf, jpeg, len);
    g_frameLen = len;
    g_frameSeq++;
  }
  xSemaphoreGive(g_frameLock);
}

size_t webTakeFrame(uint8_t** out, size_t* outCap, uint32_t* seq, uint32_t timeoutMs) {
  // Consumers poll rather than block on a semaphore: with several stream
  // clients a binary semaphore wakes exactly one of them, and an event group
  // would need per-client bookkeeping for no gain at this frame rate.
  const uint32_t deadline = millis() + timeoutMs;
  while (g_frameSeq == *seq) {
    if (static_cast<int32_t>(millis() - deadline) >= 0) return 0;
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  if (xSemaphoreTake(g_frameLock, pdMS_TO_TICKS(200)) != pdTRUE) return 0;

  const size_t len = g_frameLen;
  if (len > *outCap) {
    void* n = psramGrow(*out, len);
    if (!n) {
      xSemaphoreGive(g_frameLock);
      return 0;
    }
    *out = static_cast<uint8_t*>(n);
    *outCap = len;
  }
  if (g_frameBuf && len) memcpy(*out, g_frameBuf, len);
  *seq = g_frameSeq;
  xSemaphoreGive(g_frameLock);
  return len;
}

String urlDecode(const String& in) {
  String out;
  out.reserve(in.length());
  for (size_t i = 0; i < in.length(); ++i) {
    const char c = in[i];
    if (c == '%' && i + 2 < in.length()) {
      out += static_cast<char>(strtol(in.substring(i + 1, i + 3).c_str(), nullptr, 16));
      i += 2;
    } else if (c == '+') {
      out += ' ';
    } else {
      out += c;
    }
  }
  return out;
}

bool webBegin() {
  g_frameLock = xSemaphoreCreateMutex();
  if (!g_frameLock) return false;

  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = HTTP_UI_PORT;
  cfg.ctrl_port = 32768;
  cfg.max_uri_handlers = 32;      // the WebDAV methods take one slot each
  cfg.stack_size = 8192;
  cfg.lru_purge_enable = true;
  cfg.core_id = 0;                // leave core 1 to the capture task
#if ENABLE_WEBDAV
  cfg.uri_match_fn = httpd_uri_match_wildcard;
#endif

  if (httpd_start(&g_ui, &cfg) != ESP_OK) return false;

  registerUri(g_ui, "/",                  HTTP_GET,  indexHandler);
  registerUri(g_ui, "/api/status",        HTTP_GET,  statusHandler);
  registerUri(g_ui, "/api/config",        HTTP_GET,  configGetHandler);
  registerUri(g_ui, "/api/config",        HTTP_POST, configPostHandler);
  registerUri(g_ui, "/api/events",        HTTP_GET,  eventsHandler);
  registerUri(g_ui, "/api/label",         HTTP_POST, labelHandler);
  registerUri(g_ui, "/api/dataset.csv",   HTTP_GET,  datasetHandler);
  registerUri(g_ui, "/api/mlpreview.bmp", HTTP_GET,  mlPreviewHandler);
  registerUri(g_ui, "/api/wifi",          HTTP_POST, wifiHandler);
  registerUri(g_ui, "/api/reboot",        HTTP_POST, rebootHandler);
  registerUri(g_ui, "/capture",           HTTP_GET,  captureHandler);

#if ENABLE_WEBDAV
  webdavRegister(g_ui);
#endif

  httpd_config_t scfg = HTTPD_DEFAULT_CONFIG();
  scfg.server_port = HTTP_STREAM_PORT;
  scfg.ctrl_port = 32769;
  scfg.max_uri_handlers = 2;
  scfg.stack_size = 8192;
  scfg.core_id = 0;
  if (httpd_start(&g_stream, &scfg) == ESP_OK) {
    registerUri(g_stream, "/stream", HTTP_GET, streamHandler);
  }

  Serial.printf("[web] ui :%d  stream :%d/stream  dav :%d%s/\n", HTTP_UI_PORT,
                HTTP_STREAM_PORT, HTTP_UI_PORT, ENABLE_WEBDAV ? WEBDAV_ROOT : " (off)");
  return true;
}
