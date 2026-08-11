// Retention surface for the node in microSD when present, internal flash when not.

#pragma once

#include <Arduino.h>
#include <FS.h>

#include "detect.h"

enum StorageKind : uint8_t { STORE_NONE = 0, STORE_SD, STORE_FLASH };

bool         storageBegin();
StorageKind  storageKind();
const char*  storageKindName();
fs::FS*      storageFs();
uint64_t     storageTotalBytes();
uint64_t     storageUsedBytes();

// Writes the JPEG that opened an event, plus a JSON sidecar carrying the
// classification, under /events/YYYYMMDD/. Returns the image path, or an empty
// string if nothing was written.
String storageSaveEvent(const DetectionEvent& e, const uint8_t* jpeg, size_t len);

// Deletes oldest day-folders until at least `wantFreeBytes` is available, so an
// unattended node degrades by forgetting the distant past rather than by
// failing to record the present.
void storageReclaim(uint64_t wantFreeBytes);

// Recursive delete, used by WebDAV DELETE on a collection.
void storageRemoveTree(const String& path);
