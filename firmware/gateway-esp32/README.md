# ESP32 enforcement node

Edge-tier door controller for the Smart Gateway: RC522 credential capture, 4×4 PIN entry, the access state machine of the root, servo actuation, and the HTTP API the web application drives.

## Files

| File | Contents |
|---|---|
| `gateway-esp32.ino` | Bring-up, WiFi/NVS, config persistence and validation |
| `app_config.h` | Pin map, timings, `RuntimeConfig` |
| `access.cpp/.h` | The FSM, PIN hashing, PDP client task, audit ring, cached ACL |
| `panel.cpp/.h` | HD44780-over-PCF8574 driver, keypad scan, LEDs, buzzer |
| `latch.cpp/.h` | Servo sweep, hold timer, `REC_TRIG` to the recorder |
| `web.cpp/.h` | Console + JSON API on :80 |
| `console_ui.h` | The shared console, embedded in flash — **generated**, see below |

The page it serves at `/` is not written here. It is
[`console/index.html`](../../console/index.html), the same document the recorder
serves, embedded into both firmwares by `console/build.py`. Editing
`console_ui.h` by hand loses the edit the next time anyone regenerates it:

```bash
python console/build.py
```

## Build and flash

Arduino IDE 2.x or `arduino-cli`, ESP32 board support 2.0.x or 3.x.

| Setting | Value |
|---|---|
| Board | DOIT ESP32 DEVKIT V1 |
| Upload Speed | 921600 |
| Monitor | 115200 |

```bash
arduino-cli compile --fqbn esp32:esp32:esp32doit-devkit-v1 firmware/gateway-esp32
```

## First boot

1. The node mints an **API token** and prints it once on the serial console: `[api] token generated: 3f9c…`. 
2. With no stored credentials it raises the access point `gateway-setup` (passphrase `attestation`) and serves the page at **http://192.168.4.1**. Paste the token into the field in the header, then set the SSID.
3. It restarts onto the LAN and prints its address. The LCD header line shows `NET` once the station link is up and `---` while it is not.

> [!IMPORTANT]
> Clearing the token (`apiToken` set to an empty string) puts the API in bench mode, where the unlock endpoint answers anyone who can reach port 80. That is a wiring-desk convenience, not a configuration.

## HTTP interface

Port 80. Every route takes the token, either as the `X-Api-Token` header or as `?token=`. Replies carry permissive CORS headers and `OPTIONS` is answered for any path, so the web application can be served from anywhere; the token, not the origin, is what gates the door.

| Method | Path | Purpose |
|---|---|---|
| GET | `/` | Operator page |
| GET | `/api/status` | State, door, transaction in flight, counters, link |
| GET | `/api/events?since=&limit=` | Audit ring, newest first |
| POST | `/api/decision` | Answer the transaction the node is blocked on |
| POST | `/api/unlock`, `/api/lock` | Operator override — audited as `remote` |
| GET | `/api/acl` | Cached authorisation set |
| POST | `/api/acl` | `{uid,pin,ttl}`, or `{remove}`, or `{clear}` |
| POST | `/api/enrol?ms=` | Arm the reader to report the next card |
| GET | `/api/enrol` | Collect the enrolled UID, once |
| GET/POST | `/api/config` | Read all settings / set one `{key,value}` |
| POST | `/api/wifi`, `/api/reboot` | Provisioning |

