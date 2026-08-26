#include "web.h"

#include <Preferences.h>
#include <WiFi.h>

#include "access.h"
#include "latch.h"
#include "panel.h"
#include "console_ui.h"

namespace {

httpd_handle_t g_srv = nullptr;

// ------------------------------------------------------------- plumbing ----

void cors(httpd_req_t* req) {
  // The web application is served from somewhere else — a workstation, the
  // decision tier, a phone — so every reply carries the headers that let a
  // browser on another origin read it, and the token stays the only gate.
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, X-Api-Token");
  httpd_resp_set_hdr(req, "Access-Control-Max-Age", "600");
}

esp_err_t sendJson(httpd_req_t* req, const char* json) {
  httpd_resp_set_type(req, "application/json");
  cors(req);
  return httpd_resp_sendstr(req, json);
}

esp_err_t sendOk(httpd_req_t* req) { return sendJson(req, "{\"ok\":true}"); }

esp_err_t sendErr(httpd_req_t* req, const char* status, const char* msg) {
  char json[128];
  snprintf(json, sizeof(json), "{\"ok\":false,\"error\":\"%s\"}", msg);
  httpd_resp_set_status(req, status);
  httpd_resp_set_type(req, "application/json");
  cors(req);
  return httpd_resp_sendstr(req, json);
}

bool queryValue(httpd_req_t* req, const char* key, char* out, size_t len) {
  const size_t qlen = httpd_req_get_url_query_len(req) + 1;
  if (qlen <= 1) return false;
  char* q = static_cast<char*>(malloc(qlen));
  if (!q) return false;
  bool found = false;
  if (httpd_req_get_url_query_str(req, q, qlen) == ESP_OK)
    found = httpd_query_key_value(q, key, out, len) == ESP_OK;
  free(q);
  return found;
}

long queryLong(httpd_req_t* req, const char* key, long fallback) {
  char v[24];
  if (!queryValue(req, key, v, sizeof(v))) return fallback;
  char* end = nullptr;
  const long n = strtol(v, &end, 10);
  return (end == v) ? fallback : n;
}

// Reads the request body, capped. Anything this API accepts is a handful of
// short fields; a node that will buffer an arbitrary POST for you is a node
// that can be pushed out of memory by one request.
bool readBody(httpd_req_t* req, char* buf, size_t len) {
  const size_t total = req->content_len;
  if (total == 0 || total >= len) { buf[0] = 0; return total == 0; }
  size_t got = 0;
  while (got < total) {
    const int r = httpd_req_recv(req, buf + got, total - got);
    if (r <= 0) { buf[0] = 0; return false; }
    got += r;
  }
  buf[got] = 0;
  return true;
}

// Same flat-field reader as the PDP client uses: this API's documents are a few
// strings and integers, and a JSON library for them would be all cost.
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

// A field may arrive either in the query string or in a JSON body, so the same
// endpoint serves a browser form and an application posting JSON.
bool field(httpd_req_t* req, const char* body, const char* key, char* out, size_t len) {
  if (body && body[0] && jsonField(body, key, out, len)) return true;
  if (queryValue(req, key, out, len)) {
    const String d = urlDecode(String(out));
    strlcpy(out, d.c_str(), len);
    return true;
  }
  return false;
}

bool authorised(httpd_req_t* req) {
  if (!g_cfg.apiToken[0]) return true;          // token cleared: bench mode
  char given[48] = "";
  if (httpd_req_get_hdr_value_str(req, "X-Api-Token", given, sizeof(given)) == ESP_OK &&
      strcmp(given, g_cfg.apiToken) == 0)
    return true;
  if (queryValue(req, "token", given, sizeof(given)) &&
      strcmp(given, g_cfg.apiToken) == 0)
    return true;
  return false;
}

// -------------------------------------------------------------- handlers ---

esp_err_t optionsHandler(httpd_req_t* req) {
  cors(req);
  httpd_resp_set_status(req, "204 No Content");
  return httpd_resp_send(req, nullptr, 0);
}

esp_err_t indexHandler(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, CONSOLE_HTML, HTTPD_RESP_USE_STRLEN);
}

esp_err_t statusHandler(httpd_req_t* req) {
  if (!authorised(req)) return sendErr(req, "401 Unauthorized", "token required");
  const GateStatus s = accessStatus();
  static const char* kDoor[] = {"locked", "opening", "open", "closing"};

  char json[720];
  snprintf(json, sizeof(json),
      "{\"door\":\"%s\",\"state\":\"%s\",\"txn\":\"%s\",\"uid\":\"%s\","
      "\"pinDigits\":%u,\"attempts\":%u,\"stateMs\":%lu,\"waitMs\":%lu,"
      "\"angle\":%u,\"holdMs\":%lu,\"clockSet\":%s,"
      "\"transactions\":%lu,\"grants\":%lu,\"denials\":%lu,\"degraded\":%lu,"
      "\"pdp\":{\"configured\":%s,\"url\":\"%s\",\"lastCode\":%d,\"lastMs\":%lu},"
      "\"acl\":%u,\"auditLast\":%lu,\"lcd\":%s,\"enrolArmed\":%s,"
      "\"ip\":\"%s\",\"rssi\":%d,\"uptime\":%lu,\"epoch\":%lld,\"heap\":%lu,"
      "\"doorId\":\"%s\",\"signedDecisions\":%s}",
      kDoor[s.door], stateName(s.state), s.txn, s.uid,
      s.pinDigits, s.attempts,
      static_cast<unsigned long>(s.stateMs), static_cast<unsigned long>(s.waitMs),
      latchAngle(), static_cast<unsigned long>(latchHoldRemainingMs()),
      // The console's evidence join is on the epoch and nothing else, so
      // whether this node has a real clock is node status, not a detail.
      clockSet() ? "true" : "false",
      static_cast<unsigned long>(s.transactions), static_cast<unsigned long>(s.grants),
      static_cast<unsigned long>(s.denials), static_cast<unsigned long>(s.degraded),
      s.pdpConfigured ? "true" : "false", g_cfg.pdpUrl, s.lastHttpCode,
      static_cast<unsigned long>(s.lastRequestMs),
      static_cast<unsigned>(aclCount()), static_cast<unsigned long>(auditLastId()),
      panelLcdOk() ? "true" : "false", accessEnrolArmed() ? "true" : "false",
      WiFi.isConnected() ? WiFi.localIP().toString().c_str()
                         : WiFi.softAPIP().toString().c_str(),
      WiFi.RSSI(), static_cast<unsigned long>(millis() / 1000),
      static_cast<long long>(time(nullptr)),
      static_cast<unsigned long>(ESP.getFreeHeap()),
      g_cfg.doorId, g_cfg.decisionKey[0] ? "true" : "false");
  return sendJson(req, json);
}

// The audit ring, newest first. `since` returns only records the caller has not
// already collected, which is what makes polling cheap enough to do often.
esp_err_t eventsHandler(httpd_req_t* req) {
  if (!authorised(req)) return sendErr(req, "401 Unauthorized", "token required");
  const uint32_t since = static_cast<uint32_t>(queryLong(req, "since", 0));
  const size_t limit = static_cast<size_t>(queryLong(req, "limit", AUDIT_RING));

  httpd_resp_set_type(req, "application/json");
  cors(req);
  httpd_resp_sendstr_chunk(req, "{\"last\":");
  char head[24];
  snprintf(head, sizeof(head), "%lu,\"events\":[", static_cast<unsigned long>(auditLastId()));
  httpd_resp_sendstr_chunk(req, head);

  AuditRecord r;
  bool first = true;
  for (size_t i = 0; i < auditCount() && i < limit; ++i) {
    if (!auditGet(i, &r)) break;
    if (r.id <= since) break;                  // the ring is ordered, so stop
    char item[400];
    snprintf(item, sizeof(item),
        "%s{\"id\":%lu,\"txn\":\"%s\",\"epoch\":%lld,\"uptime\":%lu,\"uid\":\"%s\","
        "\"state\":\"%s\",\"effect\":\"%s\",\"reason\":\"%s\",\"pin\":%s,"
        "\"unsigned\":%s,\"note\":\"%s\"}",
        first ? "" : ",", static_cast<unsigned long>(r.id), r.txn,
        static_cast<long long>(r.epoch), static_cast<unsigned long>(r.uptimeMs / 1000),
        r.uid, stateName(r.state), effectName(r.effect), reasonName(r.reason),
        r.pinPresented ? "true" : "false", r.unsignedDecision ? "true" : "false",
        r.note);
    httpd_resp_sendstr_chunk(req, item);
    first = false;
  }
  httpd_resp_sendstr_chunk(req, "]}");
  return httpd_resp_sendstr_chunk(req, nullptr);
}

// The decision the node is blocked on. Effect, transaction and — when a key is
// configured — the HMAC over both. accessDecision() rejects anything that does
// not name the transaction currently in flight.
esp_err_t decisionHandler(httpd_req_t* req) {
  if (!authorised(req)) return sendErr(req, "401 Unauthorized", "token required");
  char body[512];
  if (!readBody(req, body, sizeof(body))) return sendErr(req, "400 Bad Request", "body");

  char txn[TXN_HEX] = "", effect[16] = "", reason[24] = "", sig[80] = "";
  field(req, body, "txn", txn, sizeof(txn));
  field(req, body, "effect", effect, sizeof(effect));
  field(req, body, "reason", reason, sizeof(reason));
  const bool haveSig = field(req, body, "sig", sig, sizeof(sig));

  if (!txn[0] || !effect[0]) return sendErr(req, "400 Bad Request", "txn and effect");
  const bool permit = strcasecmp(effect, "permit") == 0;
  if (!permit && strcasecmp(effect, "deny") != 0)
    return sendErr(req, "400 Bad Request", "effect must be permit or deny");

  if (!accessDecision(txn, permit, reason[0] ? reason : effect, haveSig ? sig : nullptr))
    return sendErr(req, "409 Conflict", "no such transaction in flight, or bad signature");
  return sendOk(req);
}

esp_err_t unlockHandler(httpd_req_t* req) {
  if (!authorised(req)) return sendErr(req, "401 Unauthorized", "token required");
  char body[256], who[24] = "api";
  readBody(req, body, sizeof(body));
  field(req, body, "who", who, sizeof(who));
  accessRemoteUnlock(who);
  return sendOk(req);
}

esp_err_t lockHandler(httpd_req_t* req) {
  if (!authorised(req)) return sendErr(req, "401 Unauthorized", "token required");
  char body[256], who[24] = "api";
  readBody(req, body, sizeof(body));
  field(req, body, "who", who, sizeof(who));
  accessRemoteLock(who);
  return sendOk(req);
}

esp_err_t aclGetHandler(httpd_req_t* req) {
  if (!authorised(req)) return sendErr(req, "401 Unauthorized", "token required");
  httpd_resp_set_type(req, "application/json");
  cors(req);
  httpd_resp_sendstr_chunk(req, "{\"entries\":[");
  char uid[15];
  bool hasPin = false;
  uint32_t ttl = 0;
  for (size_t i = 0; i < aclCount(); ++i) {
    if (!aclGet(i, uid, sizeof(uid), &hasPin, &ttl)) break;
    char item[128];
    snprintf(item, sizeof(item), "%s{\"uid\":\"%s\",\"pin\":%s,\"ttl\":%lu}",
             i ? "," : "", uid, hasPin ? "true" : "false",
             static_cast<unsigned long>(ttl));
    httpd_resp_sendstr_chunk(req, item);
  }
  char tail[64];
  snprintf(tail, sizeof(tail), "],\"capacity\":%d}", ACL_ENTRIES);
  httpd_resp_sendstr_chunk(req, tail);
  return httpd_resp_sendstr_chunk(req, nullptr);
}

// Maintenance of the cached authorisation set. `pin` is SHA-256("<uid>:<pin>")
// as 64 hex characters, computed by the decision tier — the node is never told
// a PIN, not even one it is expected to accept later.
esp_err_t aclPostHandler(httpd_req_t* req) {
  if (!authorised(req)) return sendErr(req, "401 Unauthorized", "token required");
  char body[512];
  if (!readBody(req, body, sizeof(body))) return sendErr(req, "400 Bad Request", "body");

  char v[80];
  if (field(req, body, "clear", v, sizeof(v))) { aclClear(); return sendOk(req); }
  if (field(req, body, "remove", v, sizeof(v)))
    return aclRemove(v) ? sendOk(req) : sendErr(req, "404 Not Found", "no such entry");

  char uid[15] = "", pin[80] = "";
  if (!field(req, body, "uid", uid, sizeof(uid)))
    return sendErr(req, "400 Bad Request", "uid, remove or clear");
  const bool havePin = field(req, body, "pin", pin, sizeof(pin));
  if (havePin && strlen(pin) != 64)
    return sendErr(req, "400 Bad Request", "pin must be a 64-char sha256 digest");

  char ttlStr[16] = "";
  const uint32_t ttl = field(req, body, "ttl", ttlStr, sizeof(ttlStr))
                           ? static_cast<uint32_t>(strtoul(ttlStr, nullptr, 10))
                           : 3600;
  return aclAdd(uid, havePin ? pin : nullptr, ttl)
             ? sendOk(req)
             : sendErr(req, "507 Insufficient Storage", "cached set is full");
}

// Enrolment. POST arms the reader; GET collects the UID once a card has been
// presented. Nothing here authorises anything — it exists so an operator does
// not have to read a UID off a serial console to register a card.
esp_err_t enrolPostHandler(httpd_req_t* req) {
  if (!authorised(req)) return sendErr(req, "401 Unauthorized", "token required");
  char body[128];
  readBody(req, body, sizeof(body));
  accessArmEnrol(static_cast<uint32_t>(queryLong(req, "ms", 20000)));
  return sendOk(req);
}

esp_err_t enrolGetHandler(httpd_req_t* req) {
  if (!authorised(req)) return sendErr(req, "401 Unauthorized", "token required");
  char uid[15], json[128];
  if (accessEnrolTake(uid, sizeof(uid)))
    snprintf(json, sizeof(json), "{\"armed\":false,\"uid\":\"%s\"}", uid);
  else
    snprintf(json, sizeof(json), "{\"armed\":%s,\"uid\":\"\"}",
             accessEnrolArmed() ? "true" : "false");
  return sendJson(req, json);
}

esp_err_t configGetHandler(httpd_req_t* req) {
  if (!authorised(req)) return sendErr(req, "401 Unauthorized", "token required");
  // The three secrets are reported as present or absent, never echoed. An API
  // that reads its own token back to you only has to be reached once.
  char json[560];
  snprintf(json, sizeof(json),
      "{\"closedAngle\":%u,\"openAngle\":%u,\"openHoldMs\":%u,\"pinTimeoutMs\":%u,"
      "\"pdpTimeoutMs\":%u,\"pinAttempts\":%u,\"requirePin\":%s,\"degradedAllow\":%s,"
      "\"buzzerEnabled\":%s,\"pdpUrl\":\"%s\",\"doorId\":\"%s\","
      "\"pdpToken\":\"%s\",\"decisionKey\":\"%s\",\"apiToken\":\"%s\","
      "\"pinMin\":%d,\"pinMax\":%d,\"lcd\":\"%dx%d\"}",
      g_cfg.closedAngle, g_cfg.openAngle, g_cfg.openHoldMs, g_cfg.pinTimeoutMs,
      g_cfg.pdpTimeoutMs, g_cfg.pinAttempts,
      g_cfg.requirePin ? "true" : "false", g_cfg.degradedAllow ? "true" : "false",
      g_cfg.buzzerEnabled ? "true" : "false", g_cfg.pdpUrl, g_cfg.doorId,
      g_cfg.pdpToken[0] ? "set" : "", g_cfg.decisionKey[0] ? "set" : "",
      g_cfg.apiToken[0] ? "set" : "",
      PIN_MIN_DIGITS, PIN_MAX_DIGITS, LCD_COLS, LCD_ROWS);
  return sendJson(req, json);
}

esp_err_t configPostHandler(httpd_req_t* req) {
  if (!authorised(req)) return sendErr(req, "401 Unauthorized", "token required");
  // Sized for the whole record at once: the five string settings alone are 266
  // characters before the key names, and readBody refuses anything it cannot
  // hold rather than acting on half a request.
  char body[1024];
  if (!readBody(req, body, sizeof(body))) return sendErr(req, "400 Bad Request", "body");

  // Two shapes, one transaction either way. {"key":…,"value":…} sets a single
  // setting; a body naming settings directly sets all of them at once.
  //
  // The second shape is not a convenience. The servo angles are only wrong in
  // combination, so applying them one request at a time means every swap of the
  // pair passes through a state the node has to refuse — the operator cannot
  // get from one valid pair to the other. Judging the whole record at once is
  // what makes that reachable, and it is also a stricter check than the
  // per-key path it replaces, not a looser one.
  char key[32] = "", value[128] = "";
  if (field(req, body, "key", key, sizeof(key)) &&
      field(req, body, "value", value, sizeof(value))) {
    if (!configSetKey(key, value))
      return sendErr(req, "400 Bad Request", "unknown key, or rejected value");
    configSave();
    return sendOk(req);
  }

  RuntimeConfig candidate = g_cfg;
  size_t named = 0;
  char v[128];
  for (size_t i = 0; i < kConfigKeyCount; ++i) {
    if (!field(req, body, kConfigKeys[i], v, sizeof(v))) continue;
    if (!configApplyOne(&candidate, kConfigKeys[i], v))
      return sendErr(req, "400 Bad Request", "unknown key");
    ++named;
  }
  if (!named) return sendErr(req, "400 Bad Request", "no settable field in the request");

  const char* why = nullptr;
  if (!configValidate(candidate, &why))
    return sendErr(req, "409 Conflict", why ? why : "invalid combination");

  configCommit(candidate);
  return sendOk(req);
}

esp_err_t wifiHandler(httpd_req_t* req) {
  if (!authorised(req)) return sendErr(req, "401 Unauthorized", "token required");
  char body[256], ssid[40] = "", pass[64] = "";
  readBody(req, body, sizeof(body));
  if (!field(req, body, "ssid", ssid, sizeof(ssid)))
    return sendErr(req, "400 Bad Request", "ssid");
  field(req, body, "pass", pass, sizeof(pass));
  Preferences p;
  p.begin("gateway", false);
  p.putString("ssid", ssid);
  p.putString("pass", pass);
  p.end();
  return sendOk(req);
}

esp_err_t rebootHandler(httpd_req_t* req) {
  if (!authorised(req)) return sendErr(req, "401 Unauthorized", "token required");
  // Refused unless the latch is shot: a reboot mid-transit leaves the servo
  // wherever it happened to be, and that position is usually "open".
  if (latchState() != DOOR_LOCKED)
    return sendErr(req, "409 Conflict", "door is not locked");
  sendOk(req);
  delay(200);
  ESP.restart();
  return ESP_OK;
}

struct Route {
  const char*    uri;
  httpd_method_t method;
  esp_err_t (*fn)(httpd_req_t*);
};

const Route kRoutes[] = {
    {"/",             HTTP_GET,  indexHandler},
    {"/api/status",   HTTP_GET,  statusHandler},
    {"/api/events",   HTTP_GET,  eventsHandler},
    {"/api/decision", HTTP_POST, decisionHandler},
    {"/api/unlock",   HTTP_POST, unlockHandler},
    {"/api/lock",     HTTP_POST, lockHandler},
    {"/api/acl",      HTTP_GET,  aclGetHandler},
    {"/api/acl",      HTTP_POST, aclPostHandler},
    {"/api/enrol",    HTTP_GET,  enrolGetHandler},
    {"/api/enrol",    HTTP_POST, enrolPostHandler},
    {"/api/config",   HTTP_GET,  configGetHandler},
    {"/api/config",   HTTP_POST, configPostHandler},
    {"/api/wifi",     HTTP_POST, wifiHandler},
    {"/api/reboot",   HTTP_POST, rebootHandler},
};

}  // namespace

String urlDecode(const String& in) {
  String out;
  out.reserve(in.length());
  for (size_t i = 0; i < in.length(); ++i) {
    const char c = in[i];
    if (c == '+') {
      out += ' ';
    } else if (c == '%' && i + 2 < in.length()) {
      const char hex[3] = {in[i + 1], in[i + 2], 0};
      out += static_cast<char>(strtol(hex, nullptr, 16));
      i += 2;
    } else {
      out += c;
    }
  }
  return out;
}

bool webBegin() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = HTTP_UI_PORT;
  cfg.ctrl_port = 32768;
  cfg.max_uri_handlers = sizeof(kRoutes) / sizeof(kRoutes[0]) + 2;
  cfg.uri_match_fn = httpd_uri_match_wildcard;   // for the preflight catch-all
  cfg.lru_purge_enable = true;
  cfg.stack_size = 8192;      // the config handler holds a candidate record and
                              // a kilobyte of request body at the same time
  // The handlers build their replies in half-kilobyte stack buffers and hand
  // them to a many-argument snprintf. The 4 kB default does not survive that:
  // the node reboots on the first request rather than answering it.
  cfg.stack_size = 8192;
  cfg.core_id = 0;                // leave core 1 to the enforcement loop

  if (httpd_start(&g_srv, &cfg) != ESP_OK) return false;

  for (const Route& r : kRoutes) {
    const httpd_uri_t u = {r.uri, r.method, r.fn, nullptr};
    httpd_register_uri_handler(g_srv, &u);
  }
  // One handler answers every CORS preflight, registered last so the exact
  // matches above are tried first.
  const httpd_uri_t opts = {"/*", HTTP_OPTIONS, optionsHandler, nullptr};
  httpd_register_uri_handler(g_srv, &opts);
  return true;
}
