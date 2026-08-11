# ESP32-CAM movement recorder

Edge-tier camera node for the Smart Gateway: motion detection, a two-stage
classifier, an HTTP operator interface, and a WebDAV export of the recordings.
Method follows [`ESP32-CAM_MJPEG2SD`][mjpeg2sd] — frame-differencing on very
small greyscale bitmaps, 1-in-N sampling, an Edge Impulse model as the ML
interface, and a WebDAV view of the store — reimplemented here as a standalone
sketch scoped to this project rather than vendored wholesale.

## The cascade

| Stage | What it does | Runs on | Typical cost |
|---|---|---|---|
| 0 | Frame differencing on a 32×24 cell grid, decoded at 1/8 scale | every sampled frame | 20–60 ms |
| 1 | Logistic regression over the geometry of the changed-cell mask | frames with movement | <1 ms |
| 2 | Edge Impulse image model on a crop of the motion bounding box | what stage 1 accepts | 0.3–2 s |

Stage 1 exists to keep stage 2 rare. A frame-differencing detector in a corridor
is dominated by two false positives — a whole-frame luminance step (lamp, AGC,
sunlight) and a scatter of isolated noisy cells — and both are cheap to
recognise from the mask alone. Running a neural network on them would cost a
second of wall clock to reach the same conclusion.

An event is a transit, not a frame: stages 1 and 2 run once when the event
opens, not on every sample inside it.

## Files

| File | Contents |
|---|---|
| `esp32cam.ino` | Bring-up, camera init, WiFi/NVS, config persistence |
| `app_config.h` | Pin map, build options, `RuntimeConfig` |
| `detect.cpp/.h` | Stages 0 and 1, event ring, capture task |
| `ml.cpp/.h` | Stage 2: wrapper over the Edge Impulse Arduino library |
| `classifier_weights.h` | Stage-1 parameters — generated, see below |
| `storage.cpp/.h` | SD_MMC (1-line) or LittleFS, event files, reclaim |
| `web.cpp/.h` | UI + JSON API on :80, MJPEG stream on :81 |
| `webdav.cpp` | WebDAV methods on `/dav` |
| `web_ui.h` | The operator page, embedded in flash |
| `tools/train_classifier.py` | Refits stage 1 from labelled events |

## Build and flash

The board is an AI-Thinker ESP32-CAM on an **ESP32-CAM-MB** baseboard: connect
the USB-C cable, pick the port, upload. No FTDI adaptor, no IO0 jumper, no reset
dance.

Arduino IDE 2.x, ESP32 board support 2.0.x or 3.x:

| Setting | Value |
|---|---|
| Board | AI Thinker ESP32-CAM |
| PSRAM | Enabled — the node will not start without it |
| Partition Scheme | Huge APP (3MB No OTA) once a model is linked in |
| Upload Speed | 921600 |
| Monitor | 115200 |

## First boot

With no stored credentials the node raises the access point `esp32cam-setup`
(passphrase `attestation`) and serves the interface at **http://192.168.4.1**.
Enter the site SSID and passphrase in the *network* panel; it stores them in NVS
and restarts onto the LAN. Its address is then printed on the serial console and
shown in the page header.

## HTTP interface

Port 80:

| Method | Path | Purpose |
|---|---|---|
| GET | `/` | Operator page |
| GET | `/api/status` | Counters, timings, last scores, changed-cell mask |
| GET/POST | `/api/config` | Read all settings / set one (`?key=value`) |
| GET | `/api/events` | Recent events as JSON |
| POST | `/api/label` | Ground truth for one event (`?id=&label=0\|1`) |
| GET | `/api/dataset.csv` | Labelled events with their feature vectors |
| GET | `/api/mlpreview.bmp` | The crop stage 2 was last given |
| GET | `/capture` | Single JPEG |
| POST | `/api/wifi`, `/api/reboot` | Provisioning |
| * | `/dav/…` | WebDAV |

Port 81: `/stream`, MJPEG multipart, capped at two concurrent clients.

Two servers because one stalled stream client must not take the management
interface with it. The stream sits behind a single-producer frame bus — only the
capture task ever touches the camera driver, so frame rate does not depend on
how many browsers are open.

## Recordings and WebDAV

Events land on the microSD card as `/events/YYYYMMDD/HHMMSS_id_verdict.jpg` with
a JSON sidecar carrying the scores, the bounding box and the feature vector —
the sidecar is what makes a clip joinable against the audit record held by the
decision tier. With no card, the same tree is written to internal flash. When
space runs short the oldest day-folder is dropped.

`SD_MMC` is mounted in **1-line mode**, which is not a tuning choice: it frees
GPIO 12/13 for the I²C link to the enforcement node (root README §4.3).

### Mapping the share on Windows

```bash
net use Z: http://<node>/dav
```

Or *This PC → Map network drive → Connect to a Web site…* and enter
`http://<node>/dav`. Explorer then browses the recordings like any other drive.
Browsing to `/dav/` in a browser gives a plain listing instead.

Three Windows-specific things this firmware does deliberately, because the
WebClient redirector is stricter than every other client:

- **The site root answers `OPTIONS /` and `PROPFIND /`.** Windows probes the
  root before it will mount a subpath, and reports a 404 there as *"The folder
  you entered does not appear to be valid"*. `GET /` still serves the operator
  page — only the WebDAV methods are treated as discovery.
- **Files up to 512 KB are sent with a real `Content-Length`,** not chunked.
  The redirector copes badly with chunked transfer encoding on a mapped drive.
- **`PROPPATCH` echoes back every property it was asked to set.** Explorer sends
  one after every `PUT`, carrying `Win32CreationTime` and friends; a client that
  gets an error there reports the copy as failed even though the file arrived.
  Nothing is actually stored, so a file copied onto the share loses its
  Windows-side timestamps.

If `net use` fails immediately, start the WebClient service
(`sc start WebClient`) — it is Manual by default and does not always start on
demand. Windows also caps WebDAV transfers at 50 MB unless
`HKLM\SYSTEM\CurrentControlSet\Services\WebClient\Parameters\FileSizeLimitInBytes`
is raised; event JPEGs are nowhere near that.

Other clients work without any of the above: `rclone`, Cyberduck, `davfs2`,
macOS Finder.

Locking is a formality — `LOCK` returns a fixed token and `UNLOCK` accepts
anything. Windows refuses to write to a share that will not answer `LOCK`, and a
single-writer embedded node has nothing to arbitrate. `COPY` of a collection
returns 501.

## Stage 2: bringing your own model

The interface expects a model packaged as an **Arduino library by Edge
Impulse**, exactly as upstream does. Nothing about a particular impulse is baked
in: input geometry, colour depth, labels, and whether it is a classifier or a
FOMO object detector are read from the library's own macros.

1. Train an impulse on 96×96 greyscale or RGB images (transfer learning works
   well for this).
2. *Deployment → Arduino library*, then *Sketch → Include Library → Add .ZIP
   Library*.
3. In `app_config.h`:

   ```c
   #define INCLUDE_TINYML true
   #define TINY_ML_LIB "your-project_inferencing.h"
   ```

4. Rebuild with the Huge APP partition scheme.

If the library is absent the sketch still builds and runs stages 0 and 1; the
interface reports why stage 2 is unavailable rather than silently reporting
motion as a classification.

Inference on a plain ESP32 costs hundreds of milliseconds to seconds per frame —
upstream restricts ML to the ESP32-S3 for this reason. The cascade makes it
usable here by paying that cost only on events, not on frames, but an S3 is the
comfortable target if you intend to run stage 2 continuously.

## Stage 1: refitting on site

`classifier_weights.h` as shipped holds hand-set priors, and the interface says
so (`priors (unfitted)`). Priors are not a model. To fit real ones:

1. Let the node run on the real doorway.
2. Label events *yes*/*no* in the events table.
3. `curl -o dataset.csv http://<node>/api/dataset.csv`
4. `python tools/train_classifier.py dataset.csv > classifier_weights.h`
5. Reflash. The interface now shows the fit date and sample count.

Unlabelled events are not exported: an unexamined event is not a negative
example. The accuracy the trainer prints is in-sample — it says the fit
converged, not that it generalises.

## Tuning

Every parameter is live on the page and persisted to NVS.

| Parameter | Effect |
|---|---|
| `cellDelta` | Luminance change that makes a cell "changed". Raise it if noise triggers events at night. |
| `motionPercent` | Fraction of cells that declares movement. |
| `sampleIdle` / `sampleActive` | 1-in-N sampling while idle / during an event. Raising `sampleActive` protects stream frame rate. |
| `minFrames` | Consecutive positive samples before an event opens. The main defence against a single noisy frame. |
| `clearFrames` | Quiet samples before it closes. |
| `classifierPercent` | Stage-1 gate. |
| `mlProbability` | Stage-2 gate (the upstream `mlProbability`, in percent). |

## Known limits

- Motion detection is unavailable above UXGA: 1/8 is the coarsest scale the
  JPEG decoder offers, and the scratch buffer is sized for 200×150. Same ceiling
  as upstream.
- Detection is frame-to-frame differencing, not background subtraction, so a
  subject that stops moving stops being detected. `clearFrames` sets how long an
  event survives that.
- The lamp on GPIO 4 shares a pull-up with the SD card's DATA1 line; expect it
  to flicker during card writes on some modules.
- Everything served is unauthenticated plaintext HTTP. This node belongs on an
  isolated VLAN, not on a network anyone can reach.

[mjpeg2sd]: https://github.com/s60sc/ESP32-CAM_MJPEG2SD
