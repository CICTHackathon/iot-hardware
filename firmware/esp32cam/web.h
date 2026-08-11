// HTTP surface of the node in the operator interface and JSON control API on port 80,
// MJPEG stream on port 81, WebDAV export of the recording store on /dav.

#pragma once

#include <Arduino.h>
#include <esp_http_server.h>

#include "app_config.h"

bool webBegin();

// Single-producer handoff of the most recent JPEG. Only the capture task calls
// publish; HTTP handlers never touch the camera driver, so frame acquisition
// stays at a predictable rate no matter how many browsers are attached.
void   webPublishFrame(const uint8_t* jpeg, size_t len);
size_t webTakeFrame(uint8_t** buf, size_t* cap, uint32_t* seq, uint32_t timeoutMs);

// Percent-decoding, shared with the WebDAV handlers.
String urlDecode(const String& in);

#if ENABLE_WEBDAV
// Registers the WebDAV method handlers on the given server (webdav.cpp).
esp_err_t webdavRegister(httpd_handle_t server);
#endif
