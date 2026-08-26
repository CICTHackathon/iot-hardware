// The enforcement state machine: credential capture, the request to the
// decision tier, the degraded path, and the audit ring the web tier drains.
//
// This node holds no organisational policy and no employee credentials. A UID
// and a PIN are evidence marshalled into an AccessRequest; the grant is issued
// by the PDP, or — only while the PDP is unreachable — by the short-lived
// cached authorisation set. A cache miss denies (README §3).
#pragma once

#include <time.h>

#include "app_config.h"
#include "latch.h"

enum GateState : uint8_t {
  ST_IDLE = 0,
  ST_CARD_READ,
  ST_PIN_ENTRY,
  ST_AWAIT_DECISION,
  ST_DEGRADED,
  ST_GRANT,
  ST_DENY,
  ST_RELOCK,
  ST_LOCKOUT,        // attempt limit reached; the reader is deaf for a while
};

enum Effect : uint8_t {
  EFF_NONE = 0,
  EFF_PERMIT,
  EFF_DENY,
  EFF_INDETERMINATE,
};

enum Reason : uint8_t {
  RSN_NONE = 0,
  RSN_PDP,             // the decision tier answered
  RSN_CACHE_HIT,       // degraded, cached authorisation matched
  RSN_CACHE_MISS,      // degraded, nothing matched — fail secure
  RSN_PIN_TIMEOUT,
  RSN_PIN_ATTEMPTS,
  RSN_PIN_MALFORMED,
  RSN_READ_ERROR,
  RSN_REMOTE,          // an operator acted through the HTTP API
  RSN_TAMPER,
  RSN_BAD_SIGNATURE,   // an inbound decision failed its HMAC
};

struct AuditRecord {
  uint32_t id;
  char     txn[TXN_HEX];
  time_t   epoch;         // 0 until NTP has set the clock
  uint32_t uptimeMs;
  char     uid[15];       // "0x" + up to 5 bytes of hex, or empty
  uint8_t  state;         // GateState the node was entering
  uint8_t  effect;
  uint8_t  reason;
  bool     pinPresented;
  bool     unsignedDecision;
  char     note[24];
};

struct GateStatus {
  GateState state;
  DoorState door;
  char      txn[TXN_HEX];
  char      uid[15];
  uint8_t   pinDigits;      // how many are buffered — never the digits
  uint8_t   attempts;
  uint32_t  stateMs;        // time in the current state
  uint32_t  waitMs;         // decision outstanding for this long
  uint32_t  transactions, grants, denials, degraded;
  uint32_t  lastRequestMs;  // round trip of the last PDP call, 0 if none
  int16_t   lastHttpCode;   // last PDP status, negative for a transport error
  bool      pdpConfigured;
};

bool       accessBegin();
void       accessTick();
GateStatus accessStatus();

// Decision intake. Called from the PDP client task and from POST /api/decision;
// both go through the same inbox, so a decision typed by an operator is
// audited, signature-checked and rate-limited exactly like one from the PDP.
// `sig` may be null; it is required whenever RuntimeConfig::decisionKey is set.
bool accessDecision(const char* txn, bool permit, const char* reason, const char* sig);

// Operator override. Bypasses the FSM and is recorded as such — a grant with no
// credential behind it is the single most interesting line in an audit log.
bool accessRemoteUnlock(const char* who);
bool accessRemoteLock(const char* who);

// Audit ring, index 0 = most recent. The authoritative log lives in the
// decision tier; this is a buffer to survive a partition, not a record.
size_t   auditCount();
bool     auditGet(size_t i, AuditRecord* out);
uint32_t auditLastId();

// Cached authorisation set. `pinDigestHex` is SHA-256("<uid>:<pin>") as 64 hex
// characters, computed by the decision tier when it pushes the entry; pass null
// for a card-only entry. ttlSec bounds how long the node may act on it.
bool   aclAdd(const char* uid, const char* pinDigestHex, uint32_t ttlSec);
bool   aclRemove(const char* uid);
void   aclClear();
size_t aclCount();
bool   aclGet(size_t i, char* uid, size_t uidLen, bool* hasPin, uint32_t* secsLeft);

// Enrolment: arms the reader so the next card presented is reported to the web
// tier instead of starting a transaction. Nothing is authorised by this.
void accessArmEnrol(uint32_t ms);
bool accessEnrolTake(char* uid, size_t len);   // true once, when a card arrived
bool accessEnrolArmed();

const char* stateName(uint8_t s);
const char* effectName(uint8_t e);
const char* reasonName(uint8_t r);
