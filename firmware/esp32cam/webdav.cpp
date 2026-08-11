// WebDAV class 1/2 export of the recording store, mounted at WEBDAV_ROOT.

#include "app_config.h"

#if ENABLE_WEBDAV

#include <FS.h>
#include <esp_heap_caps.h>

#include "storage.h"
#include "web.h"

// Largest file served in one send with a Content-Length rather than as chunked
// transfer encoding. See the note in davGet().
#define DAV_WHOLE_FILE_MAX (512 * 1024)

namespace {

const char* kAllow =
    "OPTIONS, GET, HEAD, PUT, DELETE, PROPFIND, PROPPATCH, MKCOL, MOVE, COPY, LOCK, UNLOCK";
const char* kLockToken = "opaquelocktoken:0123456789012345";

void davHeaders(httpd_req_t* req) {
  httpd_resp_set_hdr(req, "DAV", "1, 2");
  httpd_resp_set_hdr(req, "MS-Author-Via", "DAV");   // Windows mini-redirector
  httpd_resp_set_hdr(req, "Allow", kAllow);
}

// Maps a request URI onto the store. Returns "/" for the collection root.
String davPath(const char* uri) {
  String p(uri);
  const int q = p.indexOf('?');
  if (q >= 0) p = p.substring(0, q);
  if (p.startsWith(WEBDAV_ROOT)) p = p.substring(strlen(WEBDAV_ROOT));
  p = urlDecode(p);
  while (p.length() > 1 && p.endsWith("/")) p.remove(p.length() - 1);
  if (!p.startsWith("/")) p = "/" + p;
  // Refuse to walk out of the store, whatever the client claims to want.
  if (p.indexOf("..") >= 0) return String("/");
  return p;
}

String hrefFor(const String& path) {
  String out = String(WEBDAV_ROOT);
  for (size_t i = 0; i < path.length(); ++i) {
    const char c = path[i];
    const bool safe = isalnum(static_cast<unsigned char>(c)) || strchr("/-_.~", c) != nullptr;
    if (safe) {
      out += c;
    } else {
      char esc[4];
      snprintf(esc, sizeof(esc), "%%%02X", static_cast<unsigned char>(c));
      out += esc;
    }
  }
  return out;
}

const char* contentTypeOf(const String& name) {
  if (name.endsWith(".jpg") || name.endsWith(".jpeg")) return "image/jpeg";
  if (name.endsWith(".json")) return "application/json";
  if (name.endsWith(".csv")) return "text/csv";
  if (name.endsWith(".txt")) return "text/plain";
  return "application/octet-stream";
}

void httpDate(time_t t, char* out, size_t len) {
  struct tm tmv;
  gmtime_r(&t, &tmv);
  strftime(out, len, "%a, %d %b %Y %H:%M:%S GMT", &tmv);
}

void isoDate(time_t t, char* out, size_t len) {
  struct tm tmv;
  gmtime_r(&t, &tmv);
  strftime(out, len, "%Y-%m-%dT%H:%M:%SZ", &tmv);
}

// esp_http_server has no httpd_err_code_t for 409, so conflicts are answered
// by hand rather than downgraded to a status the client will misread.
esp_err_t fail(httpd_req_t* req, const char* status, const char* msg) {
  davHeaders(req);
  httpd_resp_set_status(req, status);
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_sendstr(req, msg);
  return ESP_FAIL;
}

fs::FS* store(httpd_req_t* req) {
  fs::FS* fs = storageFs();
  if (!fs) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no storage mounted");
    return nullptr;
  }
  return fs;
}

// Reads and discards a request body. Clients send XML on PROPFIND and
// PROPPATCH; this implementation reports a fixed property set regardless, so
// the body is drained only to keep the connection in sync.
void drainBody(httpd_req_t* req) {
  char sink[256];
  int remaining = req->content_len;
  while (remaining > 0) {
    const int got = httpd_req_recv(req, sink, min(remaining, static_cast<int>(sizeof(sink))));
    if (got <= 0) break;
    remaining -= got;
  }
}

void sendPropResponse(httpd_req_t* req, const String& path, bool isDir, size_t size, time_t mtime) {
  char last[48], created[32], item[640];
  httpDate(mtime, last, sizeof(last));
  isoDate(mtime, created, sizeof(created));

  String name = path.substring(path.lastIndexOf('/') + 1);
  if (name.isEmpty()) name = "dav";

  if (isDir) {
    snprintf(item, sizeof(item),
             "<D:response><D:href>%s/</D:href><D:propstat><D:prop>"
             "<D:displayname>%s</D:displayname>"
             "<D:resourcetype><D:collection/></D:resourcetype>"
             "<D:getlastmodified>%s</D:getlastmodified>"
             "<D:creationdate>%s</D:creationdate>"
             "</D:prop><D:status>HTTP/1.1 200 OK</D:status></D:propstat></D:response>",
             hrefFor(path).c_str(), name.c_str(), last, created);
  } else {
    snprintf(item, sizeof(item),
             "<D:response><D:href>%s</D:href><D:propstat><D:prop>"
             "<D:displayname>%s</D:displayname>"
             "<D:resourcetype/>"
             "<D:getcontentlength>%u</D:getcontentlength>"
             "<D:getcontenttype>%s</D:getcontenttype>"
             "<D:getlastmodified>%s</D:getlastmodified>"
             "<D:creationdate>%s</D:creationdate>"
             "</D:prop><D:status>HTTP/1.1 200 OK</D:status></D:propstat></D:response>",
             hrefFor(path).c_str(), name.c_str(), static_cast<unsigned>(size),
             contentTypeOf(name), last, created);
  }
  httpd_resp_sendstr_chunk(req, item);
}

// Methods

esp_err_t davOptions(httpd_req_t* req) {
  davHeaders(req);
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, nullptr, 0);
}

esp_err_t davPropfind(httpd_req_t* req) {
  fs::FS* fs = store(req);
  if (!fs) return ESP_FAIL;
  drainBody(req);

  const String path = davPath(req->uri);
  File f = fs->open(path);
  if (!f) {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
    return ESP_FAIL;
  }

  char depth[8] = "1";
  httpd_req_get_hdr_value_str(req, "Depth", depth, sizeof(depth));

  davHeaders(req);
  httpd_resp_set_status(req, "207 Multi-Status");
  httpd_resp_set_type(req, "application/xml; charset=utf-8");
  httpd_resp_sendstr_chunk(req,
      "<?xml version=\"1.0\" encoding=\"utf-8\"?><D:multistatus xmlns:D=\"DAV:\">");

  const bool isDir = f.isDirectory();
  sendPropResponse(req, path, isDir, f.size(), f.getLastWrite());

  if (isDir && depth[0] != '0') {
    const String base = (path == "/") ? String("") : path;
    for (File c = f.openNextFile(); c; c = f.openNextFile()) {
      String n = c.name();
      const int slash = n.lastIndexOf('/');
      if (slash >= 0) n = n.substring(slash + 1);
      sendPropResponse(req, base + "/" + n, c.isDirectory(), c.size(), c.getLastWrite());
      c.close();
    }
  }
  f.close();

  httpd_resp_sendstr_chunk(req, "</D:multistatus>");
  return httpd_resp_sendstr_chunk(req, nullptr);
}

// Reads up to `cap` bytes of the request body into a String, discarding the
// rest. Property update documents are small; anything larger is a client we do
// not need to satisfy exactly.
String readBody(httpd_req_t* req, size_t cap) {
  String body;
  int remaining = req->content_len;
  char chunk[257];
  while (remaining > 0) {
    const int want = min(remaining, static_cast<int>(sizeof(chunk) - 1));
    const int got = httpd_req_recv(req, chunk, want);
    if (got <= 0) break;
    chunk[got] = '\0';
    if (body.length() < cap) body += chunk;
    remaining -= got;
  }
  return body;
}

// PROPPATCH is answered but never acts: nothing in the store has user-settable
// properties. It cannot simply be refused, though. Windows Explorer sends one
// immediately after every PUT, to write the Win32 timestamps and file
// attributes it carries in the urn:schemas-microsoft-com namespace, and a
// client that gets an error there reports the copy as failed even though the
// file arrived intact.
//
// So each requested property name is echoed back with a 200 status: the
// response says "recorded", the store says nothing. That is a real limitation
// of this node - copy a file onto the share and its Windows-side creation time
// will not survive the round trip.
esp_err_t davProppatch(httpd_req_t* req) {
  const String body = readBody(req, 2048);
  const String path = davPath(req->uri);

  // Carry the client's own namespace declarations onto our response, so the
  // prefixed names echoed below stay well-formed.
  String ns;
  for (int i = body.indexOf("xmlns:"); i >= 0; i = body.indexOf("xmlns:", i + 6)) {
    const int q1 = body.indexOf('"', i);
    if (q1 < 0) break;
    const int q2 = body.indexOf('"', q1 + 1);
    if (q2 < 0) break;
    const String decl = body.substring(i, q2 + 1);
    if (!decl.startsWith("xmlns:D=") && ns.indexOf(decl) < 0) ns += " " + decl;
    i = q2;
  }

  // Element names inside the <prop> container, kept verbatim with their prefix.
  String props;
  const int propStart = body.indexOf("prop>");
  const int propEnd = body.lastIndexOf("</");
  for (int i = propStart; i > 0 && i < propEnd;) {
    const int lt = body.indexOf('<', i);
    if (lt < 0 || lt >= propEnd) break;
    const int stop = body.indexOf('>', lt);
    if (stop < 0) break;
    String name = body.substring(lt + 1, stop);
    name.trim();
    if (name.endsWith("/")) name.remove(name.length() - 1);
    const int sp = name.indexOf(' ');
    if (sp >= 0) name = name.substring(0, sp);
    if (!name.startsWith("/") && !name.endsWith("prop") && !name.endsWith("set") &&
        !name.endsWith("remove") && !name.endsWith("propertyupdate") && name.length()) {
      props += "<" + name + "/>";
    }
    i = stop + 1;
  }

  davHeaders(req);
  httpd_resp_set_status(req, "207 Multi-Status");
  httpd_resp_set_type(req, "application/xml; charset=utf-8");

  String out = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
               "<D:multistatus xmlns:D=\"DAV:\"";
  out += ns;
  out += "><D:response><D:href>";
  out += hrefFor(path);
  out += "</D:href><D:propstat><D:prop>";
  out += props;
  out += "</D:prop><D:status>HTTP/1.1 200 OK</D:status>"
         "</D:propstat></D:response></D:multistatus>";
  return httpd_resp_sendstr(req, out.c_str());
}

esp_err_t davGet(httpd_req_t* req) {
  fs::FS* fs = store(req);
  if (!fs) return ESP_FAIL;

  const String path = davPath(req->uri);
  File f = fs->open(path);
  if (!f) {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
    return ESP_FAIL;
  }
  davHeaders(req);

  if (f.isDirectory()) {
    // Not part of WebDAV, but a browser pointed at the share should show
    // something. Machine clients use PROPFIND and never reach this.
    httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr_chunk(req, "<!doctype html><meta charset=utf-8>"
        "<style>body{font:14px ui-monospace,monospace;background:#0e1116;color:#d7dde5;padding:16px}"
        "a{color:#58a6ff;text-decoration:none}li{margin:2px 0}</style><ul>");
    const String base = (path == "/") ? String("") : path;
    char row[320];
    for (File c = f.openNextFile(); c; c = f.openNextFile()) {
      String n = c.name();
      const int slash = n.lastIndexOf('/');
      if (slash >= 0) n = n.substring(slash + 1);
      snprintf(row, sizeof(row), "<li><a href=\"%s\">%s%s</a> <small>%u B</small></li>",
               hrefFor(base + "/" + n).c_str(), n.c_str(), c.isDirectory() ? "/" : "",
               static_cast<unsigned>(c.size()));
      c.close();
      httpd_resp_sendstr_chunk(req, row);
    }
    f.close();
    httpd_resp_sendstr_chunk(req, "</ul>");
    return httpd_resp_sendstr_chunk(req, nullptr);
  }

  char last[48];
  httpDate(f.getLastWrite(), last, sizeof(last));
  httpd_resp_set_type(req, contentTypeOf(path));
  httpd_resp_set_hdr(req, "Last-Modified", last);

  // HEAD is answered by the same handler; esp_http_server suppresses the body
  // for it, but reading the file anyway would be a pointless SD traffic burst.
  if (req->method == HTTP_HEAD) {
    f.close();
    return httpd_resp_send(req, nullptr, 0);
  }

  // Prefer a single send with a real Content-Length. The Windows WebClient
  // redirector copes badly with chunked transfer encoding on a mapped drive -
  // files arrive zero-length or the copy stalls - and an event JPEG is small
  // enough to stage in PSRAM. Anything larger falls back to chunked, which
  // every other client handles.
  const size_t size = f.size();
  if (size && size <= DAV_WHOLE_FILE_MAX) {
    uint8_t* whole = static_cast<uint8_t*>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM));
    if (!whole) whole = static_cast<uint8_t*>(malloc(size));
    if (whole) {
      const size_t got = f.read(whole, size);
      f.close();
      const esp_err_t res = httpd_resp_send(req, reinterpret_cast<const char*>(whole), got);
      free(whole);
      return res;
    }
  }

  const size_t chunk = 4096;
  uint8_t* buf = static_cast<uint8_t*>(malloc(chunk));
  if (!buf) {
    f.close();
    return httpd_resp_send_500(req);
  }
  esp_err_t res = ESP_OK;
  while (res == ESP_OK) {
    const size_t got = f.read(buf, chunk);
    if (!got) break;
    res = httpd_resp_send_chunk(req, reinterpret_cast<const char*>(buf), got);
  }
  free(buf);
  f.close();
  httpd_resp_send_chunk(req, nullptr, 0);
  return res;
}

esp_err_t davPut(httpd_req_t* req) {
  fs::FS* fs = store(req);
  if (!fs) return ESP_FAIL;

  const String path = davPath(req->uri);
  const bool existed = fs->exists(path);
  File f = fs->open(path, FILE_WRITE);
  if (!f) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "cannot create");
    return ESP_FAIL;
  }

  char buf[1024];
  int remaining = req->content_len;
  bool ok = true;
  while (remaining > 0) {
    const int got = httpd_req_recv(req, buf, min(remaining, static_cast<int>(sizeof(buf))));
    if (got <= 0) { ok = false; break; }
    if (f.write(reinterpret_cast<uint8_t*>(buf), got) != static_cast<size_t>(got)) {
      ok = false;
      break;
    }
    remaining -= got;
  }
  f.close();
  if (!ok) {
    fs->remove(path);
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "write failed");
    return ESP_FAIL;
  }

  davHeaders(req);
  httpd_resp_set_status(req, existed ? "204 No Content" : "201 Created");
  return httpd_resp_send(req, nullptr, 0);
}

esp_err_t davDelete(httpd_req_t* req) {
  fs::FS* fs = store(req);
  if (!fs) return ESP_FAIL;

  const String path = davPath(req->uri);
  File f = fs->open(path);
  if (!f) {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
    return ESP_FAIL;
  }
  const bool isDir = f.isDirectory();
  f.close();

  // Depth on DELETE is infinity by definition, so a collection goes with its
  // contents rather than failing on "directory not empty".
  bool ok;
  if (isDir) {
    storageRemoveTree(path);
    ok = !fs->exists(path);
  } else {
    ok = fs->remove(path);
  }
  if (!ok) return fail(req, "409 Conflict", "not removed");
  davHeaders(req);
  httpd_resp_set_status(req, "204 No Content");
  return httpd_resp_send(req, nullptr, 0);
}

esp_err_t davMkcol(httpd_req_t* req) {
  fs::FS* fs = store(req);
  if (!fs) return ESP_FAIL;
  const String path = davPath(req->uri);
  if (fs->exists(path)) {
    httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "already exists");
    return ESP_FAIL;
  }
  if (!fs->mkdir(path)) return fail(req, "409 Conflict", "cannot create collection");
  davHeaders(req);
  httpd_resp_set_status(req, "201 Created");
  return httpd_resp_send(req, nullptr, 0);
}

// Destination is an absolute URL; only its path is meaningful to us.
bool destinationPath(httpd_req_t* req, String* out) {
  char dest[256] = {0};
  if (httpd_req_get_hdr_value_str(req, "Destination", dest, sizeof(dest)) != ESP_OK) return false;
  String d(dest);
  const int scheme = d.indexOf("://");
  if (scheme >= 0) {
    const int slash = d.indexOf('/', scheme + 3);
    d = (slash >= 0) ? d.substring(slash) : String("/");
  }
  *out = davPath(d.c_str());
  return true;
}

esp_err_t davMove(httpd_req_t* req) {
  fs::FS* fs = store(req);
  if (!fs) return ESP_FAIL;
  String dst;
  if (!destinationPath(req, &dst)) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Destination required");
    return ESP_FAIL;
  }
  const String src = davPath(req->uri);
  const bool existed = fs->exists(dst);
  if (existed) fs->remove(dst);          // Overwrite defaults to T
  if (!fs->rename(src, dst)) return fail(req, "409 Conflict", "move failed");
  davHeaders(req);
  httpd_resp_set_status(req, existed ? "204 No Content" : "201 Created");
  return httpd_resp_send(req, nullptr, 0);
}

esp_err_t davCopy(httpd_req_t* req) {
  fs::FS* fs = store(req);
  if (!fs) return ESP_FAIL;
  String dst;
  if (!destinationPath(req, &dst)) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Destination required");
    return ESP_FAIL;
  }
  File in = fs->open(davPath(req->uri));
  if (!in) {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
    return ESP_FAIL;
  }
  if (in.isDirectory()) {
    // Recursive collection copy on a device with one SD controller and 4 KB of
    // spare stack is not worth the failure modes; upstream leaves it out too.
    in.close();
    httpd_resp_send_err(req, HTTPD_501_METHOD_NOT_IMPLEMENTED, "collection copy");
    return ESP_FAIL;
  }
  const bool existed = fs->exists(dst);
  File out = fs->open(dst, FILE_WRITE);
  if (!out) {
    in.close();
    return fail(req, "409 Conflict", "cannot create destination");
  }
  uint8_t buf[1024];
  size_t got;
  while ((got = in.read(buf, sizeof(buf))) > 0) out.write(buf, got);
  in.close();
  out.close();

  davHeaders(req);
  httpd_resp_set_status(req, existed ? "204 No Content" : "201 Created");
  return httpd_resp_send(req, nullptr, 0);
}

esp_err_t davLock(httpd_req_t* req) {
  drainBody(req);
  davHeaders(req);
  static const char kLockHeader[] = "<opaquelocktoken:0123456789012345>";
  httpd_resp_set_hdr(req, "Lock-Token", kLockHeader);
  httpd_resp_set_type(req, "application/xml; charset=utf-8");

  String body = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                "<D:prop xmlns:D=\"DAV:\"><D:lockdiscovery><D:activelock>"
                "<D:locktype><D:write/></D:locktype>"
                "<D:lockscope><D:exclusive/></D:lockscope>"
                "<D:depth>infinity</D:depth><D:timeout>Second-3600</D:timeout>"
                "<D:locktoken><D:href>";
  body += kLockToken;
  body += "</D:href></D:locktoken></D:activelock></D:lockdiscovery></D:prop>";
  return httpd_resp_sendstr(req, body.c_str());
}

esp_err_t davUnlock(httpd_req_t* req) {
  drainBody(req);
  davHeaders(req);
  httpd_resp_set_status(req, "204 No Content");
  return httpd_resp_send(req, nullptr, 0);
}

// The Windows WebClient redirector does not go straight to the URL you typed.
// Before it will mount \\<node>\dav it probes the site root - OPTIONS / to see
// whether the server speaks WebDAV at all, then PROPFIND / - and a 404 to
// either is reported to the user as "the folder you entered does not appear to
// be valid". So the root answers both, describing itself as a collection whose
// only member is the store. GET / is untouched and still serves the operator
// page; a browser and a mapped drive see different things at the same URL,
// which is exactly what the method distinction is for.
esp_err_t davRootPropfind(httpd_req_t* req) {
  drainBody(req);

  char depth[8] = "1";
  httpd_req_get_hdr_value_str(req, "Depth", depth, sizeof(depth));

  davHeaders(req);
  httpd_resp_set_status(req, "207 Multi-Status");
  httpd_resp_set_type(req, "application/xml; charset=utf-8");

  String out = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
               "<D:multistatus xmlns:D=\"DAV:\">"
               "<D:response><D:href>/</D:href><D:propstat><D:prop>"
               "<D:displayname>esp32cam</D:displayname>"
               "<D:resourcetype><D:collection/></D:resourcetype>"
               "</D:prop><D:status>HTTP/1.1 200 OK</D:status></D:propstat></D:response>";
  if (depth[0] != '0') {
    out += "<D:response><D:href>" WEBDAV_ROOT "/</D:href><D:propstat><D:prop>"
           "<D:displayname>dav</D:displayname>"
           "<D:resourcetype><D:collection/></D:resourcetype>"
           "</D:prop><D:status>HTTP/1.1 200 OK</D:status></D:propstat></D:response>";
  }
  out += "</D:multistatus>";
  return httpd_resp_sendstr(req, out.c_str());
}

void reg(httpd_handle_t s, httpd_method_t m, esp_err_t (*h)(httpd_req_t*)) {
  httpd_uri_t u = {};
  u.uri = WEBDAV_ROOT "*";     // needs httpd_uri_match_wildcard, set in webBegin()
  u.method = m;
  u.handler = h;
  httpd_register_uri_handler(s, &u);
}

}  // namespace

esp_err_t webdavRegister(httpd_handle_t server) {
  // Site-root discovery, for the sake of the Windows redirector.
  httpd_uri_t root = {};
  root.uri = "/";
  root.method = HTTP_OPTIONS;
  root.handler = davOptions;
  httpd_register_uri_handler(server, &root);
  root.method = HTTP_PROPFIND;
  root.handler = davRootPropfind;
  httpd_register_uri_handler(server, &root);

  reg(server, HTTP_OPTIONS,   davOptions);
  reg(server, HTTP_PROPFIND,  davPropfind);
  reg(server, HTTP_PROPPATCH, davProppatch);
  reg(server, HTTP_GET,       davGet);
  reg(server, HTTP_HEAD,      davGet);
  reg(server, HTTP_PUT,       davPut);
  reg(server, HTTP_DELETE,    davDelete);
  reg(server, HTTP_MKCOL,     davMkcol);
  reg(server, HTTP_MOVE,      davMove);
  reg(server, HTTP_COPY,      davCopy);
  reg(server, HTTP_LOCK,      davLock);
  reg(server, HTTP_UNLOCK,    davUnlock);
  return ESP_OK;
}

#endif  // ENABLE_WEBDAV
