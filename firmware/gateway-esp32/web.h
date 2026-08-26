// HTTP surface of the enforcement node: the operator console and the JSON
// control API on port 80.
//
// The page served at `/` is the shared console from `console/index.html`, the
// same document the recorder serves. Point a browser at either node and the
// interface is identical; it discovers which node answered and asks for the
// other one's address.
//
// This is the interface the web application speaks to. Three things go through
// it that nothing else can do: it returns the decision for the transaction the
// node is currently blocked on, it maintains the cached authorisation set that
// keeps the door usable during a partition, and it drains the audit ring.
//
// Every mutating route requires the API token. The node generates one on first
// boot and prints it on the serial console — an access-control device whose
// unlock endpoint is reachable unauthenticated is not an access-control device.
#pragma once

#include <Arduino.h>
#include <esp_http_server.h>

#include "app_config.h"

bool webBegin();

// Percent-decoding, shared by the query-string handlers.
String urlDecode(const String& in);
