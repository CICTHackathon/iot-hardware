#include "access.h"

#include <ctype.h>
#include <esp_random.h>

#include <HTTPClient.h>
#include <MFRC522.h>
#include <Preferences.h>
#include <SPI.h>
#include <WiFi.h>
#include <mbedtls/md.h>

#include "panel.h"

namespace {

// ---------------------------------------------------------------- crypto ---

void sha256(const uint8_t* data, size_t len, uint8_t out[32]) {
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, data, len);
  mbedtls_md_finish(&ctx, out);
  mbedtls_md_free(&ctx);
}

void hmacSha256(const char* key, const char* msg, uint8_t out[32]) {
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&ctx, reinterpret_cast<const uint8_t*>(key), strlen(key));
  mbedtls_md_hmac_update(&ctx, reinterpret_cast<const uint8_t*>(msg), strlen(msg));
  mbedtls_md_hmac_finish(&ctx, out);
  mbedtls_md_free(&ctx);
}

void toHex(const uint8_t* in, size_t len, char* out) {
  static const char* H = "0123456789abcdef";
  for (size_t i = 0; i < len; ++i) { out[2 * i] = H[in[i] >> 4]; out[2 * i + 1] = H[in[i] & 15]; }
  out[2 * len] = 0;
}

// Compares in time independent of where the first mismatch is. A signature
// check that returns early leaks the digest one byte at a time to anything that
// can measure the reply.
bool equalsConstantTime(const char* a, const char* b, size_t len) {
  uint8_t diff = 0;
  for (size_t i = 0; i < len; ++i)
    diff |= static_cast<uint8_t>(tolower(a[i]) ^ tolower(b[i]));
  return diff == 0;
}

// memset through a volatile pointer. The compiler is entitled to delete a plain
// memset over a buffer that is never read again, which is exactly the buffer we
// most want cleared.
void zeroise(void* p, size_t n) {
  volatile uint8_t* v = static_cast<volatile uint8_t*>(p);
  while (n--) *v++ = 0;
}

time_t nowEpoch() {
  const time_t t = time(nullptr);
  return (t > 1600000000) ? t : 0;      // 0 until NTP has actually landed
}

// ------------------------------------------------------------- audit ring ---

AuditRecord g_audit[AUDIT_RING];
size_t      g_auditCount = 0, g_auditHead = 0;
uint32_t    g_auditId = 0;
SemaphoreHandle_t g_lock = nullptr;

struct Lock {
  Lock()  { if (g_lock) xSemaphoreTake(g_lock, portMAX_DELAY); }
  ~Lock() { if (g_lock) xSemaphoreGive(g_lock); }
};

// ------------------------------------------------------- cached authority ---

struct AclEntry {
  char     uid[15];
  uint8_t  pinDigest[32];
  bool     hasPin;
  uint32_t expiresEpoch;      // 0 = pushed before the clock was set; no expiry
  bool     used;
};

AclEntry    g_acl[ACL_ENTRIES];
Preferences g_nvs;

void aclSave() {
  g_nvs.begin("gwacl", false);
  g_nvs.putBytes("acl", g_acl, sizeof(g_acl));
  g_nvs.end();
}

void aclLoad() {
  memset(g_acl, 0, sizeof(g_acl));
  g_nvs.begin("gwacl", true);
  g_nvs.getBytes("acl", g_acl, sizeof(g_acl));
  g_nvs.end();
}

// ---------------------------------------------------------------- reader ---

MFRC522  g_rfid(PIN_RC522_SS, PIN_RC522_RST);
bool     g_readerOk = false;
uint32_t g_readerCheck = 0;

bool readerAlive() {
  const uint8_t v = g_rfid.PCD_ReadRegister(MFRC522::VersionReg);
  return v != 0x00 && v != 0xFF;        // both mean the SPI link is not answering
}

// ----------------------------------------------------------------- state ---

GateState g_state = ST_IDLE;
uint32_t  g_stateSince = 0;
char      g_txn[TXN_HEX] = {0};
char      g_uid[15] = {0};
char      g_pin[PIN_MAX_DIGITS + 1] = {0};
uint8_t   g_pinLen = 0;
uint8_t   g_attempts = 0;
bool      g_pinPresented = false;
uint32_t  g_lockoutUntil = 0;

uint32_t g_transactions = 0, g_grants = 0, g_denials = 0, g_degradedCount = 0;
uint32_t g_lastRequestMs = 0;
int16_t  g_lastHttp = 0;

uint32_t g_enrolUntil = 0;
char     g_enrolUid[15] = {0};
bool     g_enrolReady = false;

// Written by the PDP task or an HTTP handler, drained by the FSM. One inbox for
// both sources, so a decision typed by an operator is signature-checked, bound
// to a transaction and audited exactly like one from the decision tier.
struct Inbox {
  bool full = false;
  char txn[TXN_HEX] = {0};
  bool permit = false;
  bool unsignedDecision = false;
  char reason[24] = {0};
};
Inbox g_inbox;

struct PdpJob {
  char txn[TXN_HEX];
  char body[400];
};
QueueHandle_t g_pdpQ = nullptr;

}  // namespace

namespace {

void audit(GateState s, Effect e, Reason r, const char* note,
           bool unsignedDecision = false) {
  Lock guard;
  AuditRecord& rec = g_audit[g_auditHead];
  memset(&rec, 0, sizeof(rec));
  rec.id = ++g_auditId;
  strlcpy(rec.txn, g_txn, sizeof(rec.txn));
  rec.epoch = nowEpoch();
  rec.uptimeMs = millis();
  strlcpy(rec.uid, g_uid, sizeof(rec.uid));
  rec.state = s;
  rec.effect = e;
  rec.reason = r;
  rec.pinPresented = g_pinPresented;
  rec.unsignedDecision = unsignedDecision;
  if (note) strlcpy(rec.note, note, sizeof(rec.note));
  g_auditHead = (g_auditHead + 1) % AUDIT_RING;
  if (g_auditCount < AUDIT_RING) ++g_auditCount;

  Serial.printf("[audit %lu] %s %s/%s uid=%s txn=%s %s\n",
                static_cast<unsigned long>(rec.id), stateName(s), effectName(e),
                reasonName(r), rec.uid[0] ? rec.uid : "-",
                rec.txn[0] ? rec.txn : "-", rec.note);
}

void clearPin() {
  zeroise(g_pin, sizeof(g_pin));
  g_pinLen = 0;
}

void newTxn() {
  uint8_t r[8];
  esp_fill_random(r, sizeof(r));
  toHex(r, sizeof(r), g_txn);
}

void paintIdle() {
  char head[LCD_COLS + 1];
  snprintf(head, sizeof(head), "%-*s%s", LCD_COLS - 3, g_cfg.doorId,
           WiFi.isConnected() ? "NET" : "---");
  panelShow(head, "Scan your card", "to open the door", nullptr);
}

void paintPin() {
  char mask[PIN_MAX_DIGITS + 1];
  memset(mask, '*', g_pinLen);
  mask[g_pinLen] = 0;
  char foot[LCD_COLS + 1];
  snprintf(foot, sizeof(foot), "attempt %u of %u", g_attempts + 1, g_cfg.pinAttempts);
  panelShow("Enter PIN", mask[0] ? mask : "____", "# submit  * clear", foot);
}

// --------------------------------------------------------------- request ---

void queuePdpRequest() {
  g_lastHttp = 0;
  if (!g_cfg.pdpUrl[0] || !WiFi.isConnected()) return;    // T_pdp will expire

  PdpJob job;
  strlcpy(job.txn, g_txn, sizeof(job.txn));

  // The PIN never leaves as digits. The digest is salted with the transaction
  // nonce, so a captured request cannot be replayed into a later transaction
  // and cannot be tabled against a second door.
  char digestHex[65] = "";
  if (g_pinLen) {
    uint8_t d[32];
    char salted[TXN_HEX + 2 + PIN_MAX_DIGITS];
    const int n = snprintf(salted, sizeof(salted), "%s:%s", g_txn, g_pin);
    sha256(reinterpret_cast<const uint8_t*>(salted), n, d);
    zeroise(salted, sizeof(salted));
    toHex(d, sizeof(d), digestHex);
  }

  snprintf(job.body, sizeof(job.body),
           "{\"txn\":\"%s\",\"door\":\"%s\",\"uid\":\"%s\",\"pinDigest\":\"%s\","
           "\"pinSalt\":\"txn\",\"uptime\":%lu,\"epoch\":%lld,\"rssi\":%d,"
           "\"node\":\"gateway-esp32\"}",
           g_txn, g_cfg.doorId, g_uid, digestHex,
           static_cast<unsigned long>(millis() / 1000),
           static_cast<long long>(nowEpoch()), WiFi.RSSI());

  if (g_pdpQ && xQueueSend(g_pdpQ, &job, 0) != pdPASS)
    Serial.println("[pdp] queue full, request dropped");
}

// Minimal string-valued field lookup. The decision documents this node accepts
// are four flat fields; linking a JSON parser to read them would cost more
// flash than the whole enforcement path.
bool jsonField(const char* body, const char* key, char* out, size_t len) {
  char pat[32];
  snprintf(pat, sizeof(pat), "\"%s\"", key);
  const char* p = strstr(body, pat);
  if (!p) return false;
  p = strchr(p + strlen(pat), ':');
  if (!p) return false;
  ++p;
  while (*p == ' ' || *p == '"') ++p;
  size_t i = 0;
  while (*p && *p != '"' && *p != ',' && *p != '}' && i + 1 < len) out[i++] = *p++;
  out[i] = 0;
  return i > 0;
}

// The only network I/O in the firmware, and it is deliberately not in the FSM.
// A blocking POST inside the state machine would put the whole latency of the
// decision tier between a keypress and the display responding to it.
void pdpTask(void*) {
  PdpJob job;
  for (;;) {
    if (xQueueReceive(g_pdpQ, &job, portMAX_DELAY) != pdTRUE) continue;
    if (!WiFi.isConnected()) continue;

    const uint32_t t0 = millis();
    HTTPClient http;
    http.setTimeout(g_cfg.pdpTimeoutMs);
    http.setConnectTimeout(g_cfg.pdpTimeoutMs);
    if (!http.begin(g_cfg.pdpUrl)) { g_lastHttp = -1; continue; }
    http.addHeader("Content-Type", "application/json");
    if (g_cfg.pdpToken[0])
      http.addHeader("Authorization", String("Bearer ") + g_cfg.pdpToken);

    const int code = http.POST(reinterpret_cast<uint8_t*>(job.body), strlen(job.body));
    g_lastHttp = static_cast<int16_t>(code);
    g_lastRequestMs = millis() - t0;

    if (code == 200) {
      const String body = http.getString();
      char effect[16] = "", reason[24] = "", sig[80] = "", txn[TXN_HEX] = "";
      jsonField(body.c_str(), "effect", effect, sizeof(effect));
      jsonField(body.c_str(), "reason", reason, sizeof(reason));
      jsonField(body.c_str(), "sig", sig, sizeof(sig));
      if (!jsonField(body.c_str(), "txn", txn, sizeof(txn)))
        strlcpy(txn, job.txn, sizeof(txn));
      accessDecision(txn, strcasecmp(effect, "permit") == 0,
                     reason[0] ? reason : effect, sig[0] ? sig : nullptr);
    } else {
      Serial.printf("[pdp] %s -> %d\n", g_cfg.pdpUrl, code);
    }
    http.end();
  }
}

}  // namespace

namespace {

// ------------------------------------------------------------ transitions ---

void enter(GateState s) {
  g_state = s;
  g_stateSince = millis();

  switch (s) {
    case ST_IDLE:
      clearPin();
      g_pinPresented = false;
      g_uid[0] = 0;
      g_txn[0] = 0;
      g_attempts = 0;
      panelAnnunciate(ANN_IDLE);
      paintIdle();
      break;

    case ST_CARD_READ:
      panelShow("Card read", g_uid, "", nullptr);
      break;

    case ST_PIN_ENTRY:
      panelKey();                    // drop anything pressed before the card
      paintPin();
      break;

    case ST_AWAIT_DECISION:
      panelAnnunciate(ANN_WAIT);
      panelShow("Checking access", "please wait", "", nullptr);
      break;

    case ST_DEGRADED:
      ++g_degradedCount;
      panelShow("Decision tier down", "offline check", "", nullptr);
      break;

    case ST_GRANT:
      ++g_grants;
      panelAnnunciate(ANN_GRANT);
      panelShow("Access granted", "Door opening", "", nullptr);
      recAssert(REC_PREROLL_MS + g_cfg.openHoldMs + 2000);
      latchOpen(g_cfg.openHoldMs);
      break;

    case ST_DENY:
      ++g_denials;
      panelAnnunciate(ANN_DENY);
      panelShow("Access denied", "The door stays", "locked", nullptr);
      break;

    case ST_RELOCK:
      panelAnnunciate(ANN_IDLE);
      panelShow("Door closing", "", "", nullptr);
      break;

    case ST_LOCKOUT:
      panelAnnunciate(ANN_ALARM);
      panelShow("Too many attempts", "Reader locked", "", nullptr);
      break;
  }
}

void denyAndReturn(Reason r, const char* note) {
  audit(ST_DENY, EFF_DENY, r, note);
  clearPin();
  enter(ST_DENY);
}

// Reads a card if one is on the antenna. Returns false when nothing is there,
// which is the common case and has to stay cheap.
bool pollCard(char* out, size_t len) {
  if (!g_rfid.PICC_IsNewCardPresent() || !g_rfid.PICC_ReadCardSerial()) return false;
  const uint8_t n = (g_rfid.uid.size < 5) ? g_rfid.uid.size : 5;
  char hex[11];
  toHex(g_rfid.uid.uidByte, n, hex);
  snprintf(out, len, "0x%s", hex);
  const bool wellFormed = g_rfid.uid.size >= 4;
  g_rfid.PICC_HaltA();
  g_rfid.PCD_StopCrypto1();
  return wellFormed;                    // a UID under 4 bytes is a bad read
}

// Offline verification against the cached authorisation set. The entry holds a
// digest of "<uid>:<pin>" pushed by the decision tier, so degraded operation
// stays two-factor instead of quietly becoming card-only.
bool aclCheck(const char* uid, const char* pin) {
  const time_t now = nowEpoch();
  for (auto& e : g_acl) {
    if (!e.used || strcmp(e.uid, uid) != 0) continue;
    if (e.expiresEpoch && now && static_cast<uint32_t>(now) > e.expiresEpoch) return false;
    if (!e.hasPin) return !g_cfg.requirePin;
    uint8_t d[32];
    char salted[16 + PIN_MAX_DIGITS + 2];
    const int n = snprintf(salted, sizeof(salted), "%s:%s", uid, pin ? pin : "");
    sha256(reinterpret_cast<const uint8_t*>(salted), n, d);
    zeroise(salted, sizeof(salted));
    return memcmp(d, e.pinDigest, sizeof(d)) == 0;
  }
  return false;
}

}  // namespace

// ------------------------------------------------------------------ names ---

const char* stateName(uint8_t s) {
  switch (s) {
    case ST_IDLE: return "idle";
    case ST_CARD_READ: return "card_read";
    case ST_PIN_ENTRY: return "pin_entry";
    case ST_AWAIT_DECISION: return "await_decision";
    case ST_DEGRADED: return "degraded";
    case ST_GRANT: return "grant";
    case ST_DENY: return "deny";
    case ST_RELOCK: return "relock";
    case ST_LOCKOUT: return "lockout";
  }
  return "?";
}

const char* effectName(uint8_t e) {
  switch (e) {
    case EFF_PERMIT: return "permit";
    case EFF_DENY: return "deny";
    case EFF_INDETERMINATE: return "indeterminate";
  }
  return "none";
}

const char* reasonName(uint8_t r) {
  switch (r) {
    case RSN_PDP: return "pdp";
    case RSN_CACHE_HIT: return "cache_hit";
    case RSN_CACHE_MISS: return "cache_miss";
    case RSN_PIN_TIMEOUT: return "pin_timeout";
    case RSN_PIN_ATTEMPTS: return "pin_attempts";
    case RSN_PIN_MALFORMED: return "pin_malformed";
    case RSN_READ_ERROR: return "read_error";
    case RSN_REMOTE: return "remote";
    case RSN_TAMPER: return "tamper";
    case RSN_BAD_SIGNATURE: return "bad_signature";
  }
  return "none";
}

// -------------------------------------------------------------------- API ---

bool accessBegin() {
  g_lock = xSemaphoreCreateMutex();
  aclLoad();

  SPI.begin(PIN_RC522_SCK, PIN_RC522_MISO, PIN_RC522_MOSI, PIN_RC522_SS);
  g_rfid.PCD_Init();
  delay(4);                              // oscillator start-up, per the datasheet
  g_readerOk = readerAlive();
  if (!g_readerOk)
    Serial.println("[rc522] not answering - check 3V3 and the SPI wiring");

  g_pdpQ = xQueueCreate(4, sizeof(PdpJob));
  xTaskCreatePinnedToCore(pdpTask, "pdp", 6144, nullptr, 4, nullptr, 0);

  enter(ST_IDLE);
  return g_readerOk;
}

GateStatus accessStatus() {
  GateStatus s;
  s.state = g_state;
  s.door = latchState();
  strlcpy(s.txn, g_txn, sizeof(s.txn));
  strlcpy(s.uid, g_uid, sizeof(s.uid));
  s.pinDigits = g_pinLen;
  s.attempts = g_attempts;
  s.stateMs = millis() - g_stateSince;
  s.waitMs = (g_state == ST_AWAIT_DECISION) ? s.stateMs : 0;
  s.transactions = g_transactions;
  s.grants = g_grants;
  s.denials = g_denials;
  s.degraded = g_degradedCount;
  s.lastRequestMs = g_lastRequestMs;
  s.lastHttpCode = g_lastHttp;
  s.pdpConfigured = g_cfg.pdpUrl[0] != 0;
  return s;
}

bool accessDecision(const char* txn, bool permit, const char* reason, const char* sig) {
  if (!txn || !txn[0]) return false;

  // A decision is only ever accepted for the transaction actually in flight.
  // Without this binding the endpoint is an unlock button that happens to take
  // an argument, and a replayed permit opens the door a second time.
  if (g_state != ST_AWAIT_DECISION || strcmp(txn, g_txn) != 0) return false;

  bool unsignedDecision = true;
  if (g_cfg.decisionKey[0]) {
    if (!sig) return false;
    char msg[TXN_HEX + 16];
    snprintf(msg, sizeof(msg), "%s|%s", txn, permit ? "permit" : "deny");
    uint8_t mac[32];
    char want[65];
    hmacSha256(g_cfg.decisionKey, msg, mac);
    toHex(mac, sizeof(mac), want);
    if (strlen(sig) != 64 || !equalsConstantTime(sig, want, 64)) {
      audit(g_state, EFF_INDETERMINATE, RSN_BAD_SIGNATURE, "rejected");
      return false;
    }
    unsignedDecision = false;
  }

  Lock guard;
  strlcpy(g_inbox.txn, txn, sizeof(g_inbox.txn));
  g_inbox.permit = permit;
  g_inbox.unsignedDecision = unsignedDecision;
  strlcpy(g_inbox.reason, reason ? reason : "", sizeof(g_inbox.reason));
  g_inbox.full = true;
  return true;
}

bool accessRemoteUnlock(const char* who) {
  if (g_state == ST_GRANT) return true;
  strlcpy(g_uid, "remote", sizeof(g_uid));
  newTxn();
  audit(ST_GRANT, EFF_PERMIT, RSN_REMOTE, who ? who : "api");
  enter(ST_GRANT);
  return true;
}

bool accessRemoteLock(const char* who) {
  audit(ST_RELOCK, EFF_NONE, RSN_REMOTE, who ? who : "api");
  latchClose();
  enter(ST_RELOCK);
  return true;
}

size_t   auditCount()  { return g_auditCount; }
uint32_t auditLastId() { return g_auditId; }

bool auditGet(size_t i, AuditRecord* out) {
  Lock guard;
  if (i >= g_auditCount) return false;
  *out = g_audit[(g_auditHead + AUDIT_RING - 1 - i) % AUDIT_RING];
  return true;
}

bool aclAdd(const char* uid, const char* pinDigestHex, uint32_t ttlSec) {
  if (!uid || strlen(uid) < 4) return false;
  AclEntry* slot = nullptr;
  for (auto& e : g_acl) if (e.used && strcmp(e.uid, uid) == 0) { slot = &e; break; }
  if (!slot) for (auto& e : g_acl) if (!e.used) { slot = &e; break; }
  if (!slot) return false;                       // bounded set, by design

  memset(slot, 0, sizeof(*slot));
  strlcpy(slot->uid, uid, sizeof(slot->uid));
  if (pinDigestHex && strlen(pinDigestHex) == 64) {
    for (int i = 0; i < 32; ++i) {
      const char b[3] = {pinDigestHex[2 * i], pinDigestHex[2 * i + 1], 0};
      slot->pinDigest[i] = static_cast<uint8_t>(strtoul(b, nullptr, 16));
    }
    slot->hasPin = true;
  }
  const time_t now = nowEpoch();
  slot->expiresEpoch = (ttlSec && now) ? static_cast<uint32_t>(now) + ttlSec : 0;
  slot->used = true;
  aclSave();
  return true;
}

bool aclRemove(const char* uid) {
  for (auto& e : g_acl) {
    if (e.used && strcmp(e.uid, uid) == 0) {
      zeroise(&e, sizeof(e));
      aclSave();
      return true;
    }
  }
  return false;
}

void aclClear() {
  zeroise(g_acl, sizeof(g_acl));
  aclSave();
}

size_t aclCount() {
  size_t n = 0;
  for (auto& e : g_acl) if (e.used) ++n;
  return n;
}

bool aclGet(size_t i, char* uid, size_t uidLen, bool* hasPin, uint32_t* secsLeft) {
  size_t n = 0;
  const time_t now = nowEpoch();
  for (auto& e : g_acl) {
    if (!e.used) continue;
    if (n++ != i) continue;
    strlcpy(uid, e.uid, uidLen);
    *hasPin = e.hasPin;
    *secsLeft = (e.expiresEpoch && now && e.expiresEpoch > static_cast<uint32_t>(now))
                    ? e.expiresEpoch - static_cast<uint32_t>(now) : 0;
    return true;
  }
  return false;
}

void accessArmEnrol(uint32_t ms) {
  g_enrolUntil = millis() + ms;
  g_enrolReady = false;
  g_enrolUid[0] = 0;
  panelShow("Enrolment", "Present a card", "", nullptr);
}

bool accessEnrolArmed() { return g_enrolUntil && millis() < g_enrolUntil; }

bool accessEnrolTake(char* uid, size_t len) {
  if (!g_enrolReady) return false;
  strlcpy(uid, g_enrolUid, len);
  g_enrolReady = false;
  g_enrolUid[0] = 0;
  return true;
}

// ------------------------------------------------------------------- tick ---

void accessTick() {
  const uint32_t now = millis();

  // Reader health. A module that has stopped answering is reported and retried,
  // never silently ignored: a dead reader denies everyone, which from the
  // outside looks exactly like a door that is working correctly.
  if (now - g_readerCheck > 2000) {
    g_readerCheck = now;
    const bool alive = readerAlive();
    if (alive != g_readerOk) {
      g_readerOk = alive;
      if (!alive) {
        audit(g_state, EFF_INDETERMINATE, RSN_TAMPER, "reader silent");
        panelAnnunciate(ANN_ALARM);
      } else {
        g_rfid.PCD_Init();
        panelAnnunciate(ANN_IDLE);
      }
    }
  }

  // Drain the decision inbox here so every effect is applied from one task.
  bool haveDecision = false, havePermit = false, wasUnsigned = false;
  char reason[24] = "";
  {
    Lock guard;
    if (g_inbox.full && strcmp(g_inbox.txn, g_txn) == 0) {
      haveDecision = true;
      havePermit = g_inbox.permit;
      wasUnsigned = g_inbox.unsignedDecision;
      strlcpy(reason, g_inbox.reason, sizeof(reason));
    }
    g_inbox.full = false;
  }

  switch (g_state) {
    case ST_IDLE: {
      if (!g_readerOk) break;
      char uid[15];
      if (!pollCard(uid, sizeof(uid))) {
        if (now - g_stateSince > 1000) { paintIdle(); g_stateSince = now; }
        break;
      }
      if (accessEnrolArmed()) {
        // Enrolment reads a card and reports it. It authorises nothing: the
        // decision tier decides what the UID means, this node only spells it.
        strlcpy(g_enrolUid, uid, sizeof(g_enrolUid));
        g_enrolReady = true;
        g_enrolUntil = 0;
        panelShow("Enrolment", uid, "sent to the server", nullptr);
        audit(ST_IDLE, EFF_NONE, RSN_NONE, "enrol read");
        break;
      }
      strlcpy(g_uid, uid, sizeof(g_uid));
      newTxn();
      ++g_transactions;
      audit(ST_CARD_READ, EFF_NONE, RSN_NONE, "uid captured");
      // Pre-roll starts at the card, not at the latch: for a denial to be worth
      // anything as evidence the clip has to contain the approach.
      recAssert(REC_PREROLL_MS);
      enter(ST_CARD_READ);
      break;
    }

    case ST_CARD_READ:
      if (strlen(g_uid) < 6) { denyAndReturn(RSN_READ_ERROR, "short uid"); break; }
      // Dwell long enough for the UID to actually reach the glass. The panel
      // flushes one line per tick, so leaving this state immediately would
      // paint half of it and then overwrite it.
      if (now - g_stateSince < 700) break;
      if (g_cfg.requirePin) enter(ST_PIN_ENTRY);
      else { queuePdpRequest(); enter(ST_AWAIT_DECISION); }
      break;

    case ST_PIN_ENTRY: {
      if (now - g_stateSince > g_cfg.pinTimeoutMs) {
        denyAndReturn(RSN_PIN_TIMEOUT, "no input");
        break;
      }
      const char k = panelKey();
      if (!k) break;
      g_stateSince = now;                        // any key restarts T_pin
      if (k >= '0' && k <= '9') {
        if (g_pinLen < PIN_MAX_DIGITS) { g_pin[g_pinLen++] = k; g_pin[g_pinLen] = 0; }
        paintPin();
      } else if (k == '*') {
        clearPin();
        paintPin();
      } else if (k == '#') {
        if (g_pinLen < PIN_MIN_DIGITS) {
          clearPin();
          if (++g_attempts >= g_cfg.pinAttempts) {
            audit(ST_LOCKOUT, EFF_DENY, RSN_PIN_ATTEMPTS, "malformed entry");
            g_lockoutUntil = now + 30000;
            enter(ST_LOCKOUT);
          } else {
            panelShow("PIN too short", "try again", "# submit  * clear", nullptr);
          }
          break;
        }
        g_pinPresented = true;
        queuePdpRequest();
        audit(ST_AWAIT_DECISION, EFF_NONE, RSN_NONE, "request sent");
        enter(ST_AWAIT_DECISION);
      }
      break;
    }

    case ST_AWAIT_DECISION:
      if (haveDecision) {
        audit(havePermit ? ST_GRANT : ST_DENY, havePermit ? EFF_PERMIT : EFF_DENY,
              RSN_PDP, reason, wasUnsigned);
        clearPin();
        enter(havePermit ? ST_GRANT : ST_DENY);
        break;
      }
      if (now - g_stateSince > g_cfg.pdpTimeoutMs) enter(ST_DEGRADED);
      break;

    case ST_DEGRADED: {
      // The one place the node decides anything by itself, and it decides in a
      // single direction: a miss denies, and so does a cache that is turned off.
      const bool hit = g_cfg.degradedAllow && aclCheck(g_uid, g_pin);
      audit(hit ? ST_GRANT : ST_DENY, hit ? EFF_PERMIT : EFF_DENY,
            hit ? RSN_CACHE_HIT : RSN_CACHE_MISS, "partition");
      clearPin();
      enter(hit ? ST_GRANT : ST_DENY);
      break;
    }

    case ST_GRANT:
      if (latchState() == DOOR_OPEN) {
        char foot[LCD_COLS + 1];
        snprintf(foot, sizeof(foot), "closes in %lus",
                 static_cast<unsigned long>((latchHoldRemainingMs() + 999) / 1000));
        panelShow("Access granted", "Door open", foot, nullptr);
      }
      if (latchState() == DOOR_CLOSING) enter(ST_RELOCK);
      break;

    case ST_DENY:
      if (now - g_stateSince > T_ANNUNCIATE_MS) enter(ST_IDLE);
      break;

    case ST_RELOCK:
      if (latchState() == DOOR_LOCKED) {
        audit(ST_RELOCK, EFF_NONE, RSN_NONE, "latch shot");
        recRelease();
        enter(ST_IDLE);
      }
      break;

    case ST_LOCKOUT: {
      const uint32_t left = (g_lockoutUntil > now) ? (g_lockoutUntil - now) / 1000 + 1 : 0;
      char foot[LCD_COLS + 1];
      snprintf(foot, sizeof(foot), "wait %lus", static_cast<unsigned long>(left));
      panelShow("Too many attempts", "Reader locked", foot, nullptr);
      if (!left) { g_attempts = 0; enter(ST_IDLE); }
      break;
    }
  }
}
