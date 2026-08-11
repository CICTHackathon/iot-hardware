// ESP32-CAM (OV2640) movement recorder for the Smart Gateway edge tier.

#include <Preferences.h>
#include <WiFi.h>
#include <esp_camera.h>

#include "app_config.h"
#include "detect.h"
#include "ml.h"
#include "storage.h"
#include "web.h"

RuntimeConfig g_cfg = defaultConfig();

// Configuration persistence

void configLoad() {
  Preferences p;
  p.begin("esp32cam", true);
  // The blob is versioned by its own size: a firmware whose RuntimeConfig has
  // grown ignores the old record rather than reading a shorter one into a
  // longer struct and inheriting whatever happened to follow it in NVS.
  RuntimeConfig stored;
  if (p.getBytesLength("cfg") == sizeof(stored) &&
      p.getBytes("cfg", &stored, sizeof(stored)) == sizeof(stored)) {
    g_cfg = stored;
  }
  p.end();
}

void configSave() {
  Preferences p;
  p.begin("esp32cam", false);
  p.putBytes("cfg", &g_cfg, sizeof(g_cfg));
  p.end();
}

void configApplySensor() {
  sensor_t* s = esp_camera_sensor_get();
  if (!s) return;
  s->set_framesize(s, static_cast<framesize_t>(g_cfg.frameSize));
  s->set_quality(s, g_cfg.jpegQuality);
  s->set_hmirror(s, g_cfg.hMirror);
  s->set_vflip(s, g_cfg.vFlip);
}

// Bring-up

static bool cameraInit() {
  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer   = LEDC_TIMER_0;
  c.pin_d0 = CAM_PIN_D0;   c.pin_d1 = CAM_PIN_D1;
  c.pin_d2 = CAM_PIN_D2;   c.pin_d3 = CAM_PIN_D3;
  c.pin_d4 = CAM_PIN_D4;   c.pin_d5 = CAM_PIN_D5;
  c.pin_d6 = CAM_PIN_D6;   c.pin_d7 = CAM_PIN_D7;
  c.pin_xclk = CAM_PIN_XCLK;   c.pin_pclk = CAM_PIN_PCLK;
  c.pin_vsync = CAM_PIN_VSYNC; c.pin_href = CAM_PIN_HREF;
  c.pin_sccb_sda = CAM_PIN_SIOD; c.pin_sccb_scl = CAM_PIN_SIOC;
  c.pin_pwdn = CAM_PIN_PWDN;   c.pin_reset = CAM_PIN_RESET;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size = static_cast<framesize_t>(g_cfg.frameSize);
  c.jpeg_quality = g_cfg.jpegQuality;
  // Two buffers so the capture task can hold one frame for analysis while the
  // sensor fills the next. LATEST discards the backlog that would otherwise
  // build up behind a slow inference.
  c.fb_count = psramFound() ? 2 : 1;
  c.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  c.grab_mode = CAMERA_GRAB_LATEST;

  const esp_err_t err = esp_camera_init(&c);
  if (err != ESP_OK) {
    Serial.printf("[cam] init failed: 0x%x\n", err);
    return false;
  }

  sensor_t* s = esp_camera_sensor_get();
  if (s) {
    // The OV2640 on this module ships with a washed-out default; a small
    // saturation lift makes the stream legible without touching exposure.
    s->set_saturation(s, 1);
  }
  configApplySensor();
  return true;
}

static void wifiConnect() {
  Preferences p;
  p.begin("esp32cam", true);
  const String ssid = p.getString("ssid", "");
  const String pass = p.getString("pass", "");
  p.end();

  if (ssid.length()) {
    Serial.printf("[net] joining %s\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);       // sleep costs frames on a streaming node
    WiFi.begin(ssid.c_str(), pass.c_str());
    const uint32_t deadline = millis() + WIFI_CONNECT_TIMEOUT_MS;
    while (WiFi.status() != WL_CONNECTED && millis() < deadline) delay(250);
    if (WiFi.isConnected()) {
      Serial.printf("[net] %s\n", WiFi.localIP().toString().c_str());
      configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);
      return;
    }
    Serial.println("[net] join failed");
  }

  // Fall back to provisioning rather than to silence: an unreachable camera is
  // indistinguishable from a dead one.
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SETUP_AP_SSID, SETUP_AP_PASS);
  Serial.printf("[net] setup AP %s at %s\n", SETUP_AP_SSID,
                WiFi.softAPIP().toString().c_str());
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  pinMode(CAM_PIN_STATUS_LED, OUTPUT);
  digitalWrite(CAM_PIN_STATUS_LED, HIGH);   // active low: off

  Serial.printf("\n[boot] esp32cam movement recorder, psram %s\n",
                psramFound() ? "present" : "MISSING");

  configLoad();
  if (!cameraInit()) {
    Serial.println("[boot] halted: camera unavailable");
    return;                    // leave the serial console up for diagnosis
  }

  storageBegin();
  wifiConnect();

  if (g_cfg.mlUse && !ml::begin()) {
    Serial.printf("[ml] unavailable: %s\n", ml::status());
    g_cfg.mlUse = false;       // do not pretend stage 2 is running
  }

  if (!detectBegin()) Serial.println("[boot] detector failed to start");
  if (!webBegin())    Serial.println("[boot] web server failed to start");
}

void loop() {
  // Everything runs in the capture task and the HTTP tasks. This loop only
  // keeps the station link alive and blinks the module LED as a sign of life.
  static uint32_t lastCheck = 0;
  static bool led = false;

  if (millis() - lastCheck > 5000) {
    lastCheck = millis();
    if (WiFi.getMode() == WIFI_STA && !WiFi.isConnected()) {
      Serial.println("[net] link lost, reconnecting");
      WiFi.reconnect();
    }
    led = !led;
    digitalWrite(CAM_PIN_STATUS_LED, led ? LOW : HIGH);
  }
  delay(100);
}
