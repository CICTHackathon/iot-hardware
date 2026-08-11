#include "storage.h"

#include <LittleFS.h>
#include <SD_MMC.h>

namespace {

StorageKind g_kind = STORE_NONE;
fs::FS* g_fs = nullptr;

// File::name() returns a full path on Arduino-ESP32 2.x and a bare name on 3.x.
String baseName(File& f) {
  String n = f.name();
  const int i = n.lastIndexOf('/');
  return (i >= 0) ? n.substring(i + 1) : n;
}

bool ensureDir(const char* path) {
  if (!g_fs) return false;
  if (g_fs->exists(path)) return true;
  return g_fs->mkdir(path);
}

}  // namespace

void storageRemoveTree(const String& path) {
  if (!g_fs) return;
  fs::FS& fs = *g_fs;
  File dir = fs.open(path);
  if (!dir) return;
  if (!dir.isDirectory()) {
    dir.close();
    fs.remove(path);
    return;
  }
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    const String child = String(path) + "/" + baseName(f);
    const bool isDir = f.isDirectory();
    f.close();
    if (isDir) storageRemoveTree(child);
    else fs.remove(child);
  }
  dir.close();
  fs.rmdir(path);
}

bool storageBegin() {
  // 1-line mode is mandatory here, not a tuning choice: it releases GPIO 12/13
  // for the I2C link to the enforcement node (README §4.3).
  if (SD_MMC.begin("/sdcard", true)) {
    if (SD_MMC.cardType() != CARD_NONE) {
      g_fs = &SD_MMC;
      g_kind = STORE_SD;
    } else {
      SD_MMC.end();
    }
  }

  if (!g_fs && LittleFS.begin(true)) {
    g_fs = &LittleFS;
    g_kind = STORE_FLASH;
  }

  if (g_fs) {
    ensureDir("/events");
    Serial.printf("[fs] %s mounted, %llu of %llu KB used\n", storageKindName(),
                  storageUsedBytes() / 1024, storageTotalBytes() / 1024);
  } else {
    Serial.println("[fs] no storage - events will not be retained");
  }
  return g_fs != nullptr;
}

StorageKind storageKind() { return g_kind; }
fs::FS* storageFs() { return g_fs; }

const char* storageKindName() {
  switch (g_kind) {
    case STORE_SD:    return "sd";
    case STORE_FLASH: return "flash";
    default:          return "none";
  }
}

uint64_t storageTotalBytes() {
  if (g_kind == STORE_SD) return SD_MMC.totalBytes();
  if (g_kind == STORE_FLASH) return LittleFS.totalBytes();
  return 0;
}

uint64_t storageUsedBytes() {
  if (g_kind == STORE_SD) return SD_MMC.usedBytes();
  if (g_kind == STORE_FLASH) return LittleFS.usedBytes();
  return 0;
}

String storageSaveEvent(const DetectionEvent& e, const uint8_t* jpeg, size_t len) {
  if (!g_fs || !jpeg || !len) return String();

  // Reserve a working margin so that the write about to happen cannot be the
  // one that fills the card.
  storageReclaim(len + 256 * 1024);

  char folder[32];
  char stem[64];
  if (e.epoch) {
    struct tm tmv;
    localtime_r(&e.epoch, &tmv);
    strftime(folder, sizeof(folder), "/events/%Y%m%d", &tmv);
    char hhmmss[16];
    strftime(hhmmss, sizeof(hhmmss), "%H%M%S", &tmv);
    snprintf(stem, sizeof(stem), "%s/%s_%04u_%s", folder, hhmmss, e.id, verdictName(e.verdict));
  } else {
    // Clock not yet set. Uptime-named files still sort correctly within a boot,
    // and the sidecar records that the timestamp is unknown rather than wrong.
    strcpy(folder, "/events/nodate");
    snprintf(stem, sizeof(stem), "%s/up%09u_%04u", folder, e.uptimeMs, e.id);
  }
  if (!ensureDir(folder)) return String();

  const String jpgPath = String(stem) + ".jpg";
  File f = g_fs->open(jpgPath, FILE_WRITE);
  if (!f) return String();
  const size_t written = f.write(jpeg, len);
  f.close();
  if (written != len) {
    g_fs->remove(jpgPath);
    return String();
  }

  // The sidecar is what makes the clip joinable against the audit record held
  // by the decision tier - the image on its own attests to nothing.
  File j = g_fs->open(String(stem) + ".json", FILE_WRITE);
  if (j) {
    j.printf("{\"id\":%u,\"epoch\":%ld,\"uptimeMs\":%u,\"verdict\":\"%s\","
             "\"stage1\":%.4f,\"stage2\":%.4f,\"mlLabel\":\"%s\","
             "\"box\":[%u,%u,%u,%u],\"frames\":%u,\"features\":[",
             e.id, static_cast<long>(e.epoch), e.uptimeMs, verdictName(e.verdict),
             e.stage1, e.stage2, e.mlLabel, e.x0, e.y0, e.x1, e.y1, e.frames);
    for (int i = 0; i < FEATURE_COUNT; ++i) j.printf("%s%.4f", i ? "," : "", e.feat.v[i]);
    j.print("]}");
    j.close();
  }
  return jpgPath;
}

void storageReclaim(uint64_t wantFreeBytes) {
  if (!g_fs) return;
  const uint64_t total = storageTotalBytes();
  if (!total) return;

  while (total - storageUsedBytes() < wantFreeBytes) {
    const uint64_t before = storageUsedBytes();

    // Day folders are named YYYYMMDD, so lexicographic order is chronological
    // order and the oldest is simply the smallest name.
    File root = g_fs->open("/events");
    if (!root) return;
    String oldest;
    for (File d = root.openNextFile(); d; d = root.openNextFile()) {
      if (d.isDirectory()) {
        const String n = baseName(d);
        if (oldest.isEmpty() || n < oldest) oldest = n;
      }
      d.close();
    }
    root.close();
    if (oldest.isEmpty()) return;   // nothing left to give back

    Serial.printf("[fs] reclaiming /events/%s\n", oldest.c_str());
    storageRemoveTree("/events/" + oldest);

    // A folder that refuses to shrink would otherwise spin here forever.
    if (storageUsedBytes() >= before) return;
  }
}
