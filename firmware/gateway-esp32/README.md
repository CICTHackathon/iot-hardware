# ESP32 enforcement node

Edge-tier door controller for the Smart Gateway: RC522 credential capture, 4×4 PIN entry, the access state machine of the root, servo actuation, and the HTTP API the web application drives.

## Files

| File | Contents |
|---|---|
| `gateway-esp32.ino` | Bring-up, WiFi/NVS, config persistence and validation |
| `app_config.h` | Pin map, timings, `RuntimeConfig` |
| `access.cpp/.h` | The FSM, PIN hashing, PDP client task, audit ring, cached ACL |
| `panel.cpp/.h` | HD44780-over-PCF8574 driver, keypad scan, LEDs, buzzer |
| `latch.cpp/.h` | Servo sweep and hold timer |
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

Arduino IDE 2.x or `arduino-cli`, ESP32 board support 2.0.x or 3.x. Verified against core **3.3.11** and `arduino-cli` 1.2.0.

| Setting | Value |
|---|---|
| Board | DOIT ESP32 DEVKIT V1 |
| Upload Speed | 921600 |
| Monitor | 115200 |

Two libraries are not in the core and must be installed once:

```bash
arduino-cli lib install "MFRC522" "ESP32Servo"
```

> [!NOTE]
> **No `arduino-cli` on `PATH`?** Arduino IDE 2.x bundles one, and it already shares the IDE's cores and libraries — no second toolchain to install. On Windows:
>
> ```powershell
> $cli = "$env:LOCALAPPDATA\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
> ```
>
> Then substitute `& $cli` for `arduino-cli` in every command below.

```bash
arduino-cli compile --fqbn esp32:esp32:esp32doit-devkit-v1 firmware/gateway-esp32
arduino-cli upload -p COM7 --fqbn esp32:esp32:esp32doit-devkit-v1 firmware/gateway-esp32
arduino-cli monitor -p COM7 -c baudrate=115200
```

Replace `COM7` with the port the DevKit actually enumerates on. `arduino-cli board list` reports a bare USB-serial bridge with no board identity, so it cannot name the port for you; with more than one board plugged in, the reliable test is to ask each port what silicon is behind it:

```bash
esptool --port COM7 --connect-attempts 1 chip_id
```

An ESP32 answers — even a refusal naming a boot mode is an answer, and identifies the chip. Anything else on the bench replies with a sync error or nothing at all. `esptool.exe` ships with the core, under `packages/esp32/tools/esptool_py/`.

The sketch currently occupies **89% of the default 1.31 MB app partition**. It fits and it flashes, but there is no room for an OTA slot and not much for growth; anything substantial added here wants the *Minimal SPIFFS (1.9MB APP)* partition scheme.

### When the board will not enter download mode

A healthy DevKit is reset into the bootloader by the DTR/RTS pair on the USB bridge, and `upload` just works. A board whose auto-reset path is faulty — a missing capacitor on `EN`, or the transistor pair not doing its job — fails in one of two ways, and the wording tells you which:

| esptool says | Means |
|---|---|
| `Wrong boot mode detected (0x13)` | The chip answered, but GPIO 0 was high at reset — it booted the app instead of the bootloader |
| `No serial data received` | The chip never answered — usually GPIO 0 held low with no reset after it, so the strapping pin was never re-read |

Both are cured by latching download mode by hand, **before** starting the upload:

1. Press and hold **BOOT** (`IO0`)
2. Tap **EN** (`RST`) once, still holding BOOT
3. Release **EN**, then release **BOOT**

The board now sits in download mode until its next reset, so there is no window to hit and nothing to hold while the upload runs — issue the `upload` command normally. Holding BOOT *during* the upload without ever tapping EN does nothing: the strapping pin is only sampled as the chip comes out of reset.

> [!TIP]
> If this is needed on every flash, the auto-reset path is the fault to chase rather than a habit to acquire. Confirm first that nothing on the bench is loading `EN` or `GPIO 0` — §4.2 of the root README leaves both free for exactly this reason.

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
| GET | `/api/status` | State, door, transaction in flight, counters, link, clock |
| GET | `/api/events?since=&limit=` | Audit ring, newest first |
| POST | `/api/decision` | Answer the transaction the node is blocked on |
| POST | `/api/unlock`, `/api/lock` | Operator override — audited as `remote` |
| GET | `/api/acl` | Cached authorisation set |
| POST | `/api/acl` | `{uid,pin,ttl}`, or `{remove}`, or `{clear}` |
| POST | `/api/enrol?ms=` | Arm the reader to report the next card |
| GET | `/api/enrol` | Collect the enrolled UID, once |
| GET/POST | `/api/config` | Read all settings / write `{key,value}` or a body naming settings directly |
| POST | `/api/wifi`, `/api/reboot` | Provisioning |

