// Board pin map, build options, and the runtime-tunable configuration record.
#pragma once

#include <Arduino.h>

#include "esp_camera.h"

// AI-Thinker ESP32-CAM (OV2640), flashed through the ESP32-CAM-MB baseboard.
#define CAM_PIN_PWDN   32
#define CAM_PIN_RESET  -1   // not broken out on this module
#define CAM_PIN_XCLK    0
#define CAM_PIN_SIOD   26
#define CAM_PIN_SIOC   27
#define CAM_PIN_D7     35
#define CAM_PIN_D6     34
#define CAM_PIN_D5     39
#define CAM_PIN_D4     36
#define CAM_PIN_D3     21
#define CAM_PIN_D2     19
#define CAM_PIN_D1     18
#define CAM_PIN_D0      5
#define CAM_PIN_VSYNC  25
#define CAM_PIN_HREF   23
#define CAM_PIN_PCLK   22

#define CAM_PIN_LAMP        4   // white illumination LED, driven through LEDC
#define LAMP_LEDC_CHANNEL   7
#define CAM_PIN_STATUS_LED 33   // red module LED, active LOW


#define INCLUDE_TINYML false
#define TINY_ML_LIB "your_impulse_edge_library.h"  // replace with your library

// Buffer ceiling for the model input crop (96x96 geometry recommended) 
// upstream for transfer-learning impulses and raise if your impulse is larger.
#define ML_MAX_W 160
#define ML_MAX_H 160

// WebDAV export of the recording store, mounted read/write by the operator.
#ifndef ENABLE_WEBDAV
#define ENABLE_WEBDAV 1
#endif
#define WEBDAV_ROOT "/dav"

// Access point raised when no station credentials are stored, or when the
// stored ones fail. Matches README §5.2 step 3 (192.168.4.1).
#define SETUP_AP_SSID "esp32cam-setup"
#define SETUP_AP_PASS "attestation"    // >= 8 chars, or the AP starts open
#define WIFI_CONNECT_TIMEOUT_MS 15000

#define HTTP_UI_PORT       80
#define HTTP_STREAM_PORT   81
#define MAX_STREAM_CLIENTS  2

#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.nist.gov"

// Detection geometry

// Frames are decoded at 1/8 scale for analysis, then reduced again to a fixed
// cell grid. Differencing on the grid rather than on pixels is what supplies
// the smoothing removes sensor noise and JPEG ringing without a blur pass.
#define GRID_W 32
#define GRID_H 24
#define GRID_CELLS (GRID_W * GRID_H)

// Largest 1/8-scale image we allocate for (UXGA 1600x1200 to 200x150).
#define GRAY_MAX_W 200
#define GRAY_MAX_H 150

#define FEATURE_COUNT   8
#define EVENT_LOG_SIZE 32

// Runtime configuration (persisted in NVS)
struct RuntimeConfig {
  // Sensor
  uint8_t frameSize;        // framesize_t, capped at FRAMESIZE_UXGA
  uint8_t jpegQuality;      // 10 (best) to 63 (worst)
  bool    hMirror;
  bool    vFlip;

  // Stage 0 - frame differencing
  uint8_t cellDelta;        // per-cell luminance delta counted as "changed"
  uint8_t motionPercent;    // percent of changed cells that declares motion
  uint8_t sampleIdle;       // analyse 1 frame in N while idle
  uint8_t sampleActive;     // analyse 1 frame in N while an event is open
  uint8_t minFrames;        // consecutive positive samples to open an event
  uint8_t clearFrames;      // consecutive negative samples to close an event

  // Stage 1 in logistic regression over motion-mask geometry
  uint8_t classifierPercent;

  // Stage 2 in Edge Impulse model. mlProbability is the upstream (0.0 to 1.0)
  bool    mlUse;
  uint8_t mlProbability;

  // Retention and annunciation
  bool    saveEvents;       // write JPEG + JSON sidecar to the store
  bool    lampOnEvent;
  uint8_t lampBrightness;   // 0..255
};

// Defaults are deliberately conservative: they under-report rather than flood
// the decision tier, which is the correct bias for an anomaly channel.
inline RuntimeConfig defaultConfig() {
  RuntimeConfig c;
  c.frameSize         = FRAMESIZE_SVGA;
  c.jpegQuality       = 12;
  c.hMirror           = false;
  c.vFlip             = false;
  c.cellDelta         = 18;
  c.motionPercent     = 3;
  c.sampleIdle        = 2;
  c.sampleActive      = 10;
  c.minFrames         = 2;
  c.clearFrames       = 8;
  c.classifierPercent = 50;
  c.mlUse             = INCLUDE_TINYML;
  c.mlProbability     = 60;
  c.saveEvents        = true;
  c.lampOnEvent       = false;
  c.lampBrightness    = 40;
  return c;
}

extern RuntimeConfig g_cfg;

void configLoad();
void configSave();
void configApplySensor();   // pushes frameSize / quality / flip to the sensor
