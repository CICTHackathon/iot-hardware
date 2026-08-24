#!/usr/bin/env python3
"""
Generate the breadboard and schematic SVG sources for the smart gateway.
"""

import os

FONT = "Arial, Helvetica, DejaVu Sans, sans-serif"
HERE = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------- palette ---
C = {
    "bg":       "#f7f8fa",
    "grid":     "#e3e7ee",
    "ink":      "#1b1f27",
    "muted":    "#5b6472",
    "uno":      "#00979d",
    "uno_dark": "#007b80",
    "header":   "#23262b",
    "lcd":      "#1f6f3f",
    "lcd_scr":  "#9ccc3c",
    "keypad":   "#2b2f36",
    "key":      "#3f8ddb",
    "key_alt":  "#d4483b",
    "rc522":    "#1c6bb8",
    "cam":      "#2a2d33",
    "servo":    "#2f7fd0",
    "bb":       "#f0f0ee",
    "bb_line":  "#c8c9c4",
    "rail_r":   "#d4483b",
    "rail_b":   "#2f6fd0",
    # wire colours
    "w_5v":     "#e02b20",
    "w_gnd":    "#101317",
    "w_33":     "#e8820c",
    "w_spi":    "#7b4fd0",
    "w_i2c":    "#0f9b8e",
    "w_kp":     "#c9a227",
    "w_sig":    "#2f6fd0",
    "w_trig":   "#d63384",
}

# ------------------------------------------------------------- primitives ---
_out = []


def add(s):
    _out.append(s)


def esc(t):
    return (t.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"))


def rect(x, y, w, h, fill, stroke="none", rx=0, sw=1, op=1.0):
    add(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" '
        f'fill="{fill}" stroke="{stroke}" stroke-width="{sw}" opacity="{op}"/>')


def circle(cx, cy, r, fill, stroke="none", sw=1, op=1.0):
    add(f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{fill}" '
        f'stroke="{stroke}" stroke-width="{sw}" opacity="{op}"/>')


def text(x, y, s, size=12, fill=None, anchor="start", weight="normal",
         family=FONT, style="normal"):
    fill = fill or C["ink"]
    add(f'<text x="{x}" y="{y}" font-family="{family}" font-size="{size}" '
        f'font-weight="{weight}" font-style="{style}" fill="{fill}" '
        f'text-anchor="{anchor}">{esc(s)}</text>')


def line(x1, y1, x2, y2, stroke, sw=2, dash=None):
    d = f' stroke-dasharray="{dash}"' if dash else ""
    add(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{stroke}" '
        f'stroke-width="{sw}" stroke-linecap="round"{d}/>')


def wire(pts, stroke, sw=3):
    """Orthogonal-ish wire through a point list, drawn with rounded joins."""
    d = " ".join(f"{x},{y}" for x, y in pts)
    add(f'<polyline points="{d}" fill="none" stroke="{stroke}" '
        f'stroke-width="{sw}" stroke-linecap="round" stroke-linejoin="round"/>')
    circle(pts[0][0], pts[0][1], sw * 0.9, stroke)
    circle(pts[-1][0], pts[-1][1], sw * 0.9, stroke)


def junction(x, y, stroke, r=4):
    circle(x, y, r, stroke)


def panel(x, y, w, h, fill, stroke, rx=8, sw=2):
    rect(x, y, w, h, fill, stroke, rx=rx, sw=sw)


def header(x, y, n, pitch=24, horiz=True, w=13, h=13):
    """Black pin-header strip with square pads."""
    if horiz:
        rect(x - 6, y - 9, n * pitch + 4, 22, C["header"], rx=3)
        for i in range(n):
            rect(x + i * pitch - 1, y - 4, w * 0.62, h * 0.62, "#c9ccd1", rx=1)
    else:
        rect(x - 9, y - 6, 22, n * pitch + 4, C["header"], rx=3)
        for i in range(n):
            rect(x - 4, y + i * pitch - 1, w * 0.62, h * 0.62, "#c9ccd1", rx=1)


def svg_open(w, h, title):
    _out.clear()
    add(f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" '
        f'viewBox="0 0 {w} {h}">')
    add(f'<title>{esc(title)}</title>')
    rect(0, 0, w, h, C["bg"])


def svg_close(path):
    add("</svg>")
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(_out) + "\n")
    print(f"  wrote {os.path.relpath(path, HERE)}")


def frame(w, h, title, subtitle):
    rect(0, 0, w, 62, "#ffffff")
    line(0, 62, w, 62, "#d7dbe2", 2)
    text(28, 30, title, 20, C["ink"], weight="bold")
    text(28, 50, subtitle, 12.5, C["muted"])
    text(w - 28, 30, "iot-hardware", 13, C["muted"], anchor="end", weight="bold")
    text(w - 28, 50, "Smart Gateway - edge tier", 11.5, C["muted"], anchor="end")


def legend(x, y, items, cols=1, title="Wire colour key"):
    bw = 250
    rows = (len(items) + cols - 1) // cols
    panel(x, y, bw * cols + 20, rows * 22 + 42, "#ffffff", "#d7dbe2", rx=6, sw=1.5)
    text(x + 14, y + 24, title, 12.5, C["ink"], weight="bold")
    for i, (col, label) in enumerate(items):
        cx = x + 14 + (i // rows) * bw
        cy = y + 46 + (i % rows) * 22
        line(cx, cy, cx + 26, cy, col, 4)
        text(cx + 34, cy + 4, label, 11.5, C["muted"])


WIRE_KEY = [
    (C["w_5v"],   "+5 V (external rail)"),
    (C["w_gnd"],  "GND / common return"),
    (C["w_33"],   "+3.3 V (logic rail)"),
    (C["w_spi"],  "SPI - RC522"),
    (C["w_i2c"],  "I2C - LCD and ESP32-CAM"),
    (C["w_kp"],   "Keypad matrix"),
    (C["w_sig"],  "Discrete signals"),
    (C["w_trig"], "REC_TRIG to recorder"),
]


# ------------------------------------------------------------ ESP32 board ---
# DOIT ESP32 DEVKIT V1 / NodeMCU-32S, 30-pin, as silkscreened on the bench
# unit. Column order is transcribed from the board itself; note that the left
# column ends "... D14 D12 D13 GND VIN", with D13 *above* GND.
#
#   left column,  antenna end first: EN VP VN D34 D35 D32 D33 D25 D26 D27
#                                    D14 D12 D13 GND VIN
#   right column, antenna end first: D23 D22 TX0 RX0 D21 D19 D18 D5 TX2 RX2
#                                    D4 D2 D15 GND 3V3
#
# The board is drawn rotated 90 degrees anticlockwise: antenna to the left,
# micro-USB to the right. Under that rotation the right column becomes the top
# edge read left to right, and the left column becomes the bottom edge read
# left to right. Reversing either list on its own would silently mirror the
# board and put every jumper in the wrong hole.
ESP_TOP = ["G23", "G22", "TX0", "RX0", "G21", "G19", "G18", "G5",
           "G17", "G16", "G4", "G2", "G15", "GND", "3V3"]
ESP_BOT = ["EN", "G36", "G39", "G34", "G35", "G32", "G33", "G25",
           "G26", "G27", "G14", "G12", "G13", "GND2", "VIN"]


# Breadboard view: physical layout of the enforcement node and peripherals.
#
# Horizontal routing corridors, top to bottom. Every jumper runs along one of
# these; keeping them declared in one place is what stops two wires sharing a
# line and reading as a junction.
Y_LCD_VCC, Y_LCD_GND = 242, 250   # display subsystem supply
Y_BUS_SDA, Y_BUS_SCL = 338, 348        # translator low side -> ESP32
Y_TRIG = 358                           # recorder trigger -> G4
Y_TAP_SDA, Y_TAP_SCL = 368, 378        # recorder taps onto the I2C pair
Y_SERVO_SIG = 388
Y_SPI = (396, 400, 404, 408, 412, 416, 420)
Y_ANN = (728, 734, 740)                # buzzer, grant LED, deny LED
Y_KEYPAD = (748, 752, 756, 760, 764, 768, 772, 776)
Y_CAM_STUB = (788, 784, 780)           # G13, G12, G16 - clear of the keypad
Y_VIN, Y_GND = 1010, 1016              # ESP32 supply, under the breadboard
Y_SERVO_PWR = (1022, 1030)
Y_CAM_PWR = (1038, 1046)


def breadboard_view(path):
    W, H = 1560, 1210
    svg_open(W, H, "Smart Gateway - breadboard view")
    frame(W, H, "Smart Gateway - Breadboard View",
          "ESP32 DevKit V1 (30-pin) enforcement node with RC522, 4x4 keypad, "
          "SG90 servo, display subsystem and ESP32-CAM recorder")

    # ---- ESP32 DevKit V1, antenna left / micro-USB right ----
    ux, uy, uw, uh = 470, 450, 470, 250
    uy2 = uy + uh
    panel(ux, uy, uw, uh, "#1f232a", "#0e1116", rx=10, sw=2)
    for i in range(7):                                       # PCB antenna
        rect(ux + 18 + i * 12, uy + 22, 7, 30, "#c8a04a", rx=1)
    rect(ux + 18, uy + 46, 82, 7, "#c8a04a")
    rect(ux + 118, uy + 56, 200, 116, "#b9bec7", "#8d939c", rx=4, sw=1.5)
    text(ux + 218, uy + 106, "ESP32-WROOM-32", 12, "#23262b", anchor="middle",
         weight="bold")
    text(ux + 218, uy + 126, "240 MHz  -  WiFi + BT", 9.5, "#4a4f57",
         anchor="middle")
    rect(ux + 340, uy + 104, 26, 26, "#3b3f46", rx=3)        # EN / BOOT
    rect(ux + 374, uy + 104, 26, 26, "#3b3f46", rx=3)
    text(ux + 353, uy + 150, "EN", 8.5, "#8f959e", anchor="middle")
    text(ux + 387, uy + 150, "BOOT", 8.5, "#8f959e", anchor="middle")
    rect(ux + uw - 52, uy + 100, 40, 30, "#9aa1ab", rx=3)    # micro-USB
    text(ux + uw - 32, uy + 150, "USB", 9, "#8f959e", anchor="middle")
    text(ux + uw / 2, uy + 208, "ESP32 DevKit V1", 18, "#ffffff",
         anchor="middle", weight="bold")
    text(ux + uw / 2, uy + 228, "Policy Enforcement Point", 11.5, "#9fb4c9",
         anchor="middle")

    pitch = 28
    top = {n: (508 + i * pitch, uy) for i, n in enumerate(ESP_TOP)}
    bot = {n: (508 + i * pitch, uy2) for i, n in enumerate(ESP_BOT)}
    header(top[ESP_TOP[0]][0], uy, 15, pitch)
    header(bot[ESP_BOT[0]][0], uy2, 15, pitch)
    for name, (px, py) in top.items():
        text(px, py - 16, name, 9, C["muted"], anchor="middle")
    for name, (px, py) in bot.items():
        text(px, py + 22, name.replace("GND2", "GND"), 9, C["muted"],
             anchor="middle")

    # ---- display subsystem (top centre) - detailed on sheet 2 ----
    lx, ly, lw, lh = 380, 92, 380, 130
    panel(lx, ly, lw, lh, C["lcd"], "#14532a", rx=6)
    rect(lx + 24, ly + 20, 200, 62, C["lcd_scr"], "#6f9a24", rx=3, sw=1.5)
    text(lx + 38, ly + 44, "PRESENT BADGE", 11.5, "#2c3a12",
         family="Courier New, monospace")
    text(lx + 38, ly + 64, "GW-LAB-01", 11.5, "#2c3a12",
         family="Courier New, monospace")
    rect(lx + 244, ly + 20, 112, 62, "#0f3d22", "#0a2a17", rx=4, sw=1.5)
    text(lx + 300, ly + 44, "Arduino", 11, "#cfe8d6", anchor="middle",
         weight="bold")
    text(lx + 300, ly + 62, "UNO R3", 11, "#cfe8d6", anchor="middle")
    text(lx + lw / 2, ly - 12,
         "Display subsystem  -  5 V island, see sheet 2", 12.5, C["ink"],
         anchor="middle", weight="bold")
    lcd = {}
    for i, name in enumerate(["SDA", "SCL", "5V", "GND"]):
        px = 470 + i * 50
        lcd[name] = (px, ly + lh)
        circle(px, ly + lh, 4.5, "#c9ccd1", "#7d8189", 1)
        text(px, ly + lh - 8, name, 9, "#cfe8d6", anchor="middle")

    # ---- 4x4 keypad (top right) - all four columns scanned on ESP32 ----
    kx, ky, kw, kh = 1150, 92, 300, 300
    panel(kx, ky, kw, kh, C["keypad"], "#15181c", rx=10)
    labels = [["1", "2", "3", "A"], ["4", "5", "6", "B"],
              ["7", "8", "9", "C"], ["*", "0", "#", "D"]]
    for r in range(4):
        for c in range(4):
            bx_, by_ = kx + 22 + c * 66, ky + 26 + r * 66
            fill = C["key_alt"] if labels[r][c] in "*#" else C["key"]
            rect(bx_, by_, 54, 50, fill, "#1a1d21", rx=7, sw=1.5)
            text(bx_ + 27, by_ + 33, labels[r][c], 19, "#ffffff",
                 anchor="middle", weight="bold")
    text(kx + kw / 2, ky - 12, "4x4 matrix keypad", 12.5, C["ink"],
         anchor="middle", weight="bold")
    text(kx + kw / 2, ky + kh + 24,
         "all sixteen keys scanned - see README section 4.2", 10.5,
         C["muted"], anchor="middle", style="italic")
    kp = {}
    for i, name in enumerate(["R1", "R2", "R3", "R4", "C1", "C2", "C3", "C4"]):
        py = ky + 34 + i * 34
        kp[name] = (kx, py)
        circle(kx, py, 4.5, "#c9ccd1", "#7d8189", 1)
        text(kx - 12, py + 4, name, 9.5, C["muted"], anchor="end")

    # ---- RC522 (left) ----
    rx_, ry, rw, rh = 60, 330, 210, 300
    panel(rx_, ry, rw, rh, C["rc522"], "#0f4a80", rx=8)
    for i in range(4):
        rect(rx_ + 26, ry + 40 + i * 26, 158, 12, "#3f8ddb", rx=6, op=0.55)
    circle(rx_ + rw / 2, ry + 195, 46, "none", "#7fb6ea", 3)
    circle(rx_ + rw / 2, ry + 195, 30, "none", "#7fb6ea", 3)
    text(rx_ + rw / 2, ry + 262, "MFRC522", 14, "#ffffff", anchor="middle",
         weight="bold")
    text(rx_ + rw / 2, ry + 280, "13.56 MHz", 10.5, "#bcd9f5", anchor="middle")
    text(rx_ + rw / 2, ry - 12, "RC522 reader (3.3 V native)", 12.5, C["ink"],
         anchor="middle", weight="bold")
    rcp = {}
    for i, name in enumerate(["SDA", "SCK", "MOSI", "MISO", "GND", "RST",
                              "3V3"]):
        py = ry + 26 + i * 38
        rcp[name] = (rx_ + rw, py)
        circle(rx_ + rw, py, 4.5, "#c9ccd1", "#7d8189", 1)
        # labels sit inside the module, clear of the outgoing jumpers
        text(rx_ + rw - 12, py + 4, name, 9.5, "#cfe4f8", anchor="end")

    # ---- SG90 servo (right) ----
    sx, sy = 1270, 470
    panel(sx, sy, 170, 110, C["servo"], "#1c5a9e", rx=6)
    rect(sx + 118, sy + 14, 44, 34, "#7fb6ea", rx=4)
    circle(sx + 140, sy + 62, 22, "#e8eef5", "#9fb4c9", 2)
    line(sx + 140, sy + 62, sx + 140, sy + 26, "#9fb4c9", 5)
    text(sx + 60, sy + 66, "SG90", 15, "#ffffff", weight="bold")
    text(sx + 85, sy - 12, "Latch actuator", 12.5, C["ink"], anchor="middle",
         weight="bold")
    svp = {}
    for i, (name, col) in enumerate((("SIG", C["w_sig"]), ("5V", C["w_5v"]),
                                     ("GND", C["w_gnd"]))):
        py = sy + 26 + i * 30
        svp[name] = (sx, py)
        circle(sx, py, 4.5, col, "#7d8189", 1)
        text(sx - 12, py + 4, name, 9.5, C["muted"], anchor="end")

    # ---- ESP32-CAM (bottom right) ----
    ex, ey, ew, eh = 1020, 790, 220, 210
    text(ex + ew / 2, ey - 34, "Movement recorder node", 12.5, C["ink"],
         anchor="middle", weight="bold")
    panel(ex, ey, ew, eh, C["cam"], "#15181c", rx=8)
    rect(ex + 20, ey + 40, 84, 84, "#3a3f47", rx=6)
    circle(ex + 62, ey + 82, 28, "#0d0f12", "#5a6069", 3)
    circle(ex + 62, ey + 82, 14, "#1d3a5c")
    circle(ex + 54, ey + 74, 5, "#8fb8e8", op=0.8)
    rect(ex + 124, ey + 44, 74, 52, "#8f949c", rx=4)
    text(ex + 161, ey + 74, "microSD", 8.5, "#23262b", anchor="middle")
    text(ex + ew / 2, ey + 156, "ESP32-CAM", 14, "#ffffff", anchor="middle",
         weight="bold")
    text(ex + ew / 2, ey + 174, "MJPEG2SD recorder", 10, "#a9b0ba",
         anchor="middle")
    text(ex + ew / 2, ey + 192, "AI-Thinker / OV2640", 9.5, "#7f868f",
         anchor="middle")
    cam = {}
    for i, name in enumerate(["G13", "G12", "G16", "5V", "GND"]):
        px = ex + 22 + i * 44
        cam[name] = (px, ey)
        circle(px, ey, 4.5, "#c9ccd1", "#7d8189", 1)
        text(px, ey - 12, name, 8.5, C["muted"], anchor="middle")

    # ---- breadboard (bottom left) ----
    bx, by, bw, bh = 60, 800, 620, 200
    panel(bx, by, bw, bh, C["bb"], C["bb_line"], rx=6, sw=1.5)
    rail_rt, rail_bt = by + 14, by + 26
    rail_rb, rail_bb = by + bh - 26, by + bh - 14
    for yr, yb in ((rail_rt, rail_bt), (rail_rb, rail_bb)):
        line(bx + 16, yr, bx + bw - 16, yr, C["rail_r"], 1.5)
        line(bx + 16, yb, bx + bw - 16, yb, C["rail_b"], 1.5)
    for col in range(int((bw - 40) / 14)):
        for row in range(5):
            circle(bx + 26 + col * 14, by + 52 + row * 13, 1.9, "#b9bab6")
            circle(bx + 26 + col * 14, by + 128 + row * 13, 1.9, "#b9bab6")
    line(bx + 16, by + 118, bx + bw - 16, by + 118, "#dcdcd8", 6)
    text(bx + bw / 2, by - 12, "Annunciation harness", 12.5, C["ink"],
         anchor="middle", weight="bold")
    wire([(100, rail_rt), (100, rail_rb)], C["w_5v"], 2.5)   # rail to rail
    wire([(120, rail_bt), (120, rail_bb)], C["w_gnd"], 2.5)

    led_r = (bx + 120, by + 90)
    led_g = (bx + 200, by + 90)
    buzz = (bx + 330, by + 96)
    for (cx, cy), col, lab in ((led_r, C["rail_r"], "DENY"),
                               (led_g, "#2fa84f", "GRANT")):
        circle(cx, cy, 13, col, "#00000033", 1.5)
        circle(cx - 4, cy - 4, 4, "#ffffff", op=0.6)
        text(cx, cy + 34, lab, 9.5, C["muted"], anchor="middle")
        rect(cx - 26, cy + 44, 52, 12, "#d8c9a3", "#9a8b66", rx=3, sw=1)
        for i, bc in enumerate(("#8b1a1a", "#c02020", "#3b2b1a")):
            rect(cx - 16 + i * 9, cy + 44, 5, 12, bc)
        text(cx, cy + 70, "220R", 9, C["muted"], anchor="middle")
    circle(buzz[0], buzz[1], 24, "#1b1f27", "#43474e", 2)
    circle(buzz[0], buzz[1], 5, "#43474e")
    text(buzz[0], buzz[1] + 44, "BUZZER", 9.5, C["muted"], anchor="middle")

    # ================================================================ wires ==
    # RC522 -> ESP32 top header. Corridors run top to bottom against
    # destinations running right to left, so no drop crosses another corridor.
    for name, riser, corridor, dest in (("3V3", 360, Y_SPI[0], "3V3"),
                                        ("GND", 340, Y_SPI[1], "GND"),
                                        ("RST", 350, Y_SPI[2], "G2"),
                                        ("SDA", 300, Y_SPI[3], "G5"),
                                        ("SCK", 310, Y_SPI[4], "G18"),
                                        ("MISO", 330, Y_SPI[5], "G19"),
                                        ("MOSI", 320, Y_SPI[6], "G23")):
        col = {"GND": C["w_gnd"], "3V3": C["w_33"]}.get(name, C["w_spi"])
        wire([rcp[name], (riser, rcp[name][1]), (riser, corridor),
              (top[dest][0], corridor), top[dest]], col)

    # display subsystem -> ESP32 I2C pins. The subsystem presents 3.3 V here:
    # its BSS138 sits on the far side of these two wires, on sheet 2.
    sda_x, scl_x = top["G21"][0], top["G22"][0]
    wire([lcd["SDA"], (lcd["SDA"][0], Y_BUS_SDA), (sda_x, Y_BUS_SDA),
          top["G21"]], C["w_i2c"])
    wire([lcd["SCL"], (lcd["SCL"][0], Y_BUS_SCL), (scl_x, Y_BUS_SCL),
          top["G22"]], C["w_i2c"])
    # the subsystem is a 5 V island, fed from the rail and not from the ESP32
    wire([lcd["5V"], (lcd["5V"][0], Y_LCD_VCC), (280, Y_LCD_VCC),
          (280, rail_rt)], C["w_5v"])
    wire([lcd["GND"], (lcd["GND"][0], Y_LCD_GND), (290, Y_LCD_GND),
          (290, rail_bt)], C["w_gnd"])

    # keypad rows and columns -> ESP32 bottom header
    for name, riser, corridor, dest in (("R1", 964, Y_KEYPAD[0], "G13"),
                                        ("R2", 970, Y_KEYPAD[1], "G14"),
                                        ("R3", 976, Y_KEYPAD[2], "G27"),
                                        ("R4", 982, Y_KEYPAD[3], "G26"),
                                        ("C2", 988, Y_KEYPAD[4], "G35"),
                                        ("C1", 994, Y_KEYPAD[5], "G34"),
                                        ("C4", 1000, Y_KEYPAD[6], "G39"),
                                        ("C3", 1006, Y_KEYPAD[7], "G36")):
        wire([kp[name], (riser, kp[name][1]), (riser, corridor),
              (bot[dest][0], corridor), bot[dest]], C["w_kp"])

    # annunciators -> ESP32 bottom header; cathodes -> breadboard ground rail
    for (cx, cy), rad, corridor, dest in ((buzz, 24, Y_ANN[0], "G32"),
                                          (led_g, 13, Y_ANN[1], "G25"),
                                          (led_r, 13, Y_ANN[2], "G33")):
        wire([(cx, cy - rad), (cx, corridor), (bot[dest][0], corridor),
              bot[dest]], C["w_sig"])
    for cx, cy in (led_r, led_g):
        wire([(cx, cy + 70), (cx, rail_bb)], C["w_gnd"], 2.5)
    wire([(buzz[0], buzz[1] + 24), (buzz[0], rail_bb)], C["w_gnd"], 2.5)

    # servo: signal to G17, power off the external rail
    wire([svp["SIG"], (1252, svp["SIG"][1]), (1252, Y_SERVO_SIG),
          (top["G17"][0], Y_SERVO_SIG), top["G17"]], C["w_sig"])
    wire([svp["5V"], (1256, svp["5V"][1]), (1256, Y_SERVO_PWR[0]),
          (600, Y_SERVO_PWR[0]), (600, rail_rb)], C["w_5v"])
    wire([svp["GND"], (1262, svp["GND"][1]), (1262, Y_SERVO_PWR[1]),
          (576, Y_SERVO_PWR[1]), (576, rail_bb)], C["w_gnd"])

    # ESP32-CAM: both nodes are 3.3 V, so the recorder taps the bus directly.
    # The risers sit left of the keypad risers, so they cross no keypad wire.
    wire([cam["G13"], (cam["G13"][0], Y_CAM_STUB[0]), (946, Y_CAM_STUB[0]),
          (946, Y_TAP_SDA), (sda_x, Y_TAP_SDA)], C["w_i2c"])
    junction(sda_x, Y_TAP_SDA, C["w_i2c"])
    wire([cam["G12"], (cam["G12"][0], Y_CAM_STUB[1]), (952, Y_CAM_STUB[1]),
          (952, Y_TAP_SCL), (scl_x, Y_TAP_SCL)], C["w_i2c"])
    junction(scl_x, Y_TAP_SCL, C["w_i2c"])
    wire([cam["G16"], (cam["G16"][0], Y_CAM_STUB[2]), (958, Y_CAM_STUB[2]),
          (958, Y_TRIG), (top["G4"][0], Y_TRIG), top["G4"]], C["w_trig"])
    wire([cam["5V"], (cam["5V"][0], 780), (1244, 780), (1244, Y_CAM_PWR[0]),
          (652, Y_CAM_PWR[0]), (652, rail_rb)], C["w_5v"])
    wire([cam["GND"], (cam["GND"][0], 784), (1250, 784), (1250, Y_CAM_PWR[1]),
          (628, Y_CAM_PWR[1]), (628, rail_bb)], C["w_gnd"])

    # ESP32 board supply
    wire([bot["VIN"], (bot["VIN"][0], Y_VIN), (560, Y_VIN), (560, rail_rb)],
         C["w_5v"])
    wire([bot["GND2"], (bot["GND2"][0], Y_GND), (536, Y_GND), (536, rail_bb)],
         C["w_gnd"])

    # external supply
    psx, psy = 700, 1080
    panel(psx, psy, 200, 62, "#ffffff", "#d7dbe2", rx=6, sw=1.5)
    text(psx + 100, psy + 26, "External 5 V / 2 A", 12, C["ink"],
         anchor="middle", weight="bold")
    text(psx + 100, psy + 46, "grounds commoned to the ESP32", 10, C["muted"],
         anchor="middle")
    wire([(psx, psy + 20), (664, psy + 20), (664, rail_rb)], C["w_5v"])
    wire([(psx, psy + 44), (500, psy + 44), (500, rail_bb)], C["w_gnd"])

    legend(1280, 620, WIRE_KEY, cols=1)
    text(28, H - 18,
         "Generated by schematic/svggen.py - do not edit by hand. "
         "Pin assignment authoritative in README.md section 4.2.",
         10.5, C["muted"], style="italic")
    svg_close(path)


# Schematic view: net-level interconnect, pin names and wire colours.
def schematic_view(path):
    W, H = 1560, 1200
    svg_open(W, H, "Smart Gateway - schematic view")
    for gx in range(0, W, 20):
        line(gx, 62, gx, H, C["grid"], 0.5)
    for gy in range(80, H, 20):
        line(0, gy, W, gy, C["grid"], 0.5)
    frame(W, H, "Smart Gateway - Schematic View",
          "Net-level interconnect of the ESP32 enforcement node, peripherals "
          "and movement recorder")

    def block(x, y, w, h, name, sub, accent):
        rect(x + 4, y + 4, w, h, "#00000010", rx=6)
        panel(x, y, w, h, "#ffffff", accent, rx=6, sw=2)
        rect(x, y, w, 30, accent, rx=6)
        rect(x, y + 22, w, 8, accent)
        text(x + w / 2, y + 21, name, 13, "#ffffff", anchor="middle",
             weight="bold")
        text(x + w / 2, y + 48, sub, 10.5, C["muted"], anchor="middle")

    def pin(x, y, name, side="left", col=None):
        col = col or C["ink"]
        d = -14 if side == "left" else 14
        line(x, y, x + d, y, col, 2)
        circle(x + d, y, 3.5, col)
        text(x + (8 if side == "left" else -8), y + 4, name, 10, C["muted"],
             anchor="start" if side == "left" else "end")
        return (x + d, y)

    # ---- MCU block (centre) ----
    mx, my, mw, mh = 560, 180, 420, 620
    block(mx, my, mw, mh, "ESP32-WROOM-32  /  DevKit V1",
          "Policy Enforcement Point - deterministic FSM", C["uno"])

    left, right = {}, {}
    lnames = [("GPIO18 SCK", C["w_spi"]), ("GPIO19 MISO", C["w_spi"]),
              ("GPIO23 MOSI", C["w_spi"]), ("GPIO5 SS", C["w_spi"]),
              ("GPIO2 RST", C["w_spi"]), ("3V3", C["w_33"]), ("VIN 5V", C["w_5v"]),
              ("GND", C["w_gnd"])]
    for i, (n, c) in enumerate(lnames):
        left[n.split()[0]] = pin(mx, my + 90 + i * 46, n, "left", c)
    rnames = [("GPIO4 REC_TRIG", C["w_trig"]), ("GPIO25 LED_G", C["w_sig"]),
              ("GPIO33 LED_R", C["w_sig"]), ("GPIO32 BUZZER", C["w_sig"]),
              ("GPIO13/14/27/26 ROWS", C["w_kp"]),
              ("GPIO34/35/36/39 COLS", C["w_kp"]),
              ("GPIO17 SERVO", C["w_sig"]), ("GPIO21 SDA", C["w_i2c"]),
              ("GPIO22 SCL", C["w_i2c"])]
    for i, (n, c) in enumerate(rnames):
        right[n.split()[0]] = pin(mx + mw, my + 90 + i * 46, n, "right", c)

    # ---- peripherals ----
    block(90, 200, 260, 200, "MFRC522", "ISO/IEC 14443A reader", C["rc522"])
    rc = {}
    for i, (n, c) in enumerate((("SCK", C["w_spi"]), ("MISO", C["w_spi"]),
                                ("MOSI", C["w_spi"]), ("SDA/SS", C["w_spi"]),
                                ("RST", C["w_spi"]), ("3V3", C["w_33"]))):
        rc[n] = pin(350, 240 + i * 26, n, "right", c)
    text(220, 388, "3.3 V logic - native match to the ESP32", 10, "#1f7a4d",
         anchor="middle", weight="bold")

    block(90, 460, 260, 150, "Display subsystem",
          "Arduino UNO R3 + LCD 1602A - sheet 2", C["lcd"])
    lc = {}
    for i, n in enumerate(("SDA", "SCL")):
        lc[n] = pin(350, 540 + i * 30, n, "right", C["w_i2c"])
    text(220, 600, "5 V island - BSS138 on sheet 2", 9.5, C["muted"],
         anchor="middle", style="italic")

    block(90, 680, 260, 190, "Annunciators", "grant / deny / alarm",
          C["muted"])
    an = {}
    for i, (n, c) in enumerate((("LED_R 220R", C["w_sig"]),
                                ("LED_G 220R", C["w_sig"]),
                                ("BUZZER", C["w_sig"]))):
        an[n.split()[0]] = pin(350, 740 + i * 40, n, "right", c)

    block(1190, 180, 280, 250, "4x4 Keypad", "all sixteen keys scanned",
          C["keypad"])
    kp = {}
    for i, n in enumerate(("R1..R4", "C1..C4")):
        kp[n] = pin(1190, 300 + i * 46, n, "left", C["w_kp"])
    text(1330, 410, "columns on input-only pins, 10k pull-ups", 9.5,
         C["muted"], anchor="middle", style="italic")

    block(1190, 500, 280, 130, "SG90 Servo", "latch actuator", C["servo"])
    sv = pin(1190, 570, "SIG", "left", C["w_sig"])

    block(1190, 700, 280, 210, "ESP32-CAM", "MJPEG2SD movement recorder",
          C["cam"])
    cm = {}
    for i, (n, c) in enumerate((("G13 SDA", C["w_i2c"]),
                                ("G12 SCL", C["w_i2c"]),
                                ("G16 REC_TRIG", C["w_trig"]))):
        cm[n.split()[0]] = pin(1190, 780 + i * 40, n, "left", c)

    # ---- nets ----
    for a, b, c in ((rc["SCK"], left["GPIO18"], C["w_spi"]),
                    (rc["MISO"], left["GPIO19"], C["w_spi"]),
                    (rc["MOSI"], left["GPIO23"], C["w_spi"]),
                    (rc["SDA/SS"], left["GPIO5"], C["w_spi"]),
                    (rc["RST"], left["GPIO2"], C["w_spi"])):
        midx = 420 + list(rc.values()).index(a) * 8
        wire([a, (midx, a[1]), (midx, b[1]), b], c, 2.2)
    wire([rc["3V3"], (500, rc["3V3"][1]), (500, left["3V3"][1]),
          left["3V3"]], C["w_33"], 2.2)

    # the display subsystem presents 3.3 V here - its translator is on sheet 2
    wire([lc["SDA"], (390, lc["SDA"][1]), (390, 950), (1035, 950),
          (1035, right["GPIO21"][1]), right["GPIO21"]], C["w_i2c"], 2.2)
    wire([lc["SCL"], (402, lc["SCL"][1]), (402, 968), (1050, 968),
          (1050, right["GPIO22"][1]), right["GPIO22"]], C["w_i2c"], 2.2)

    # the discrete-signal corridor passes *under* the MCU block - routing it
    # across the block would read as a connection to the part it crosses
    for src, dst, drop, corridor, riser in (
            (an["LED_R"],  right["GPIO33"], 444, 838, 1010),
            (an["LED_G"],  right["GPIO25"], 437, 856, 1000),
            (an["BUZZER"], right["GPIO32"], 430, 874,  990)):
        wire([src, (drop, src[1]), (drop, corridor), (riser, corridor),
              (riser, dst[1]), dst], C["w_sig"], 2.2)

    wire([kp["R1..R4"], (1120, kp["R1..R4"][1]),
          (1120, right["GPIO13/14/27/26"][1]), right["GPIO13/14/27/26"]],
         C["w_kp"], 2.6)
    wire([kp["C1..C4"], (1105, kp["C1..C4"][1]),
          (1105, right["GPIO34/35/36/39"][1]), right["GPIO34/35/36/39"]],
         C["w_kp"], 2.6)
    wire([sv, (1090, sv[1]), (1090, right["GPIO17"][1]), right["GPIO17"]],
         C["w_sig"], 2.2)

    # the recorder sits on the same 3.3 V bus - no translation on this branch
    wire([cm["G13"], (1022, cm["G13"][1]), (1022, 950)], C["w_i2c"], 2.2)
    junction(1022, 950, C["w_i2c"])
    wire([cm["G12"], (1034, cm["G12"][1]), (1034, 968)], C["w_i2c"], 2.2)
    junction(1034, 968, C["w_i2c"])
    wire([cm["G16"], (1170, cm["G16"][1]), (1170, 690), (1015, 690),
          (1015, right["GPIO4"][1]), right["GPIO4"]], C["w_trig"], 2.2)

    # power rails
    line(60, 130, W - 60, 130, C["w_5v"], 3)
    text(66, 122, "+5 V", 11, C["w_5v"], weight="bold")
    line(60, 1140, W - 60, 1140, C["w_gnd"], 3)
    text(66, 1132, "GND", 11, C["w_gnd"], weight="bold")
    wire([left["VIN"], (520, left["VIN"][1]), (520, 130)], C["w_5v"], 2.2)
    wire([left["GND"], (505, left["GND"][1]), (505, 1140)], C["w_gnd"], 2.2)
    junction(520, 130, C["w_5v"])
    junction(505, 1140, C["w_gnd"])

    legend(90, 900, WIRE_KEY, cols=1)
    text(28, H - 18,
         "Generated by schematic/svggen.py - do not edit by hand. "
         "Pin assignment authoritative in README.md section 4.2.",
         10.5, C["muted"], style="italic")
    svg_close(path)


# Display subsystem sheet: the bare 1602A and the Arduino that drives it.
#
# This is a 5 V island. It presents exactly four wires to the main sheet -
# SDA, SCL, 5 V, GND - and the BSS138 sits on this side of them, so nothing
# above 3.3 V ever reaches the ESP32.
def display_view(path):
    W, H = 1180, 880
    svg_open(W, H, "Smart Gateway - display subsystem")
    frame(W, H, "Smart Gateway - Display Subsystem (sheet 2)",
          "LCD 1602A + Arduino UNO R3")

    # ---- LCD 1602A, bare 16-way header ----
    lx, ly, lw, lh = 140, 80, 440, 130
    panel(lx, ly, lw, lh, C["lcd"], "#14532a", rx=6)
    rect(lx + 34, ly + 30, 372, 74, C["lcd_scr"], "#6f9a24", rx=3, sw=1.5)
    text(lx + 52, ly + 60, "PRESENT BADGE", 13, "#2c3a12",
         family="Courier New, monospace")
    text(lx + 52, ly + 86, "GW-LAB-01", 13, "#2c3a12",
         family="Courier New, monospace")
    text(lx + lw / 2, ly - 12, "LCD 1602A", 12.5, C["ink"], anchor="middle",
         weight="bold")
    lcd_names = ["GND", "VCC", "Vo", "RS", "RW", "E", "D0", "D1", "D2", "D3",
                 "D4", "D5", "D6", "D7", "A", "K"]
    lcd = {}
    for i, name in enumerate(lcd_names):
        px = 176 + i * 26
        lcd[name] = (px, ly + lh)
        circle(px, ly + lh, 4.5, "#c9ccd1", "#7d8189", 1)
        text(px, ly + lh - 8, name, 8, "#cfe8d6", anchor="middle")
    # ---- contrast trimmer, directly under Vo ----
    px_, py_, pw_, ph_ = 176, 250, 76, 46
    panel(px_, py_, pw_, ph_, "#3b3f46", "#23262b", rx=4, sw=1.5)
    circle(px_ + 38, py_ + 24, 14, "#c9a227", "#8a6f1a", 2)
    line(px_ + 38, py_ + 24, px_ + 48, py_ + 16, "#4a4033", 3)
    text(px_ + pw_ / 2, py_ + ph_ + 16, "10k", 9, C["muted"],
         anchor="middle")
    pot_w = (228, py_)                       # wiper, top edge, under Vo
    pot_5v = (px_, py_ + 12)
    pot_gnd = (px_, py_ + 34)
    for pt in (pot_w, pot_5v, pot_gnd):
        circle(pt[0], pt[1], 4, "#c9ccd1", "#7d8189", 1)

    # ---- Arduino UNO R3 ----
    ax, ay, aw, ah = 140, 420, 440, 200
    ay2 = ay + ah
    panel(ax, ay, aw, ah, "#0f6f78", "#08464c", rx=10, sw=2)
    rect(ax + 20, ay + 66, 40, 46, "#9aa1ab", rx=3)
    rect(ax + 150, ay + 74, 130, 44, "#23262b", rx=4)
    text(ax + aw / 2, ay + 165, "Arduino UNO R3", 16, "#ffffff",
         anchor="middle", weight="bold")

    dig_names = ["D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9",
                 "D10", "D11", "D12", "D13", "GND", "AREF", "SDA", "SCL"]
    dig = {}
    for i, name in enumerate(dig_names):
        dig[name] = (172 + i * 22 + (12 if i >= 8 else 0), ay)
    header(dig["D0"][0], ay, 8, 22)
    header(dig["D8"][0], ay, 10, 22)
    for name, (qx, qy) in dig.items():
        text(qx, qy - 14, name, 7.5, C["muted"], anchor="middle")

    pwr_names = ["3V3", "5V", "GND", "GND2", "VIN"]
    ana_names = ["A0", "A1", "A2", "A3", "A4", "A5"]
    ard = {}
    for i, name in enumerate(pwr_names):
        ard[name] = (172 + i * 22, ay2)
    for i, name in enumerate(ana_names):
        ard[name] = (340 + i * 22, ay2)
    header(ard["3V3"][0], ay2, 5, 22)
    header(ard["A0"][0], ay2, 6, 22)
    for name in pwr_names + ana_names:
        qx, qy = ard[name]
        text(qx, qy + 22, name.replace("GND2", "GND"), 7.5, C["muted"],
             anchor="middle")

    # ---- BSS138 and the connector back to the main sheet ----
    tx_, ty_, tw_, th_ = 680, 660, 190, 56
    panel(tx_, ty_, tw_, th_, "#e8d9b0", "#b9a678", rx=5, sw=1.5)
    text(tx_ + tw_ / 2, ty_ + 34, "BSS138", 11.5, "#4a4033",
         anchor="middle", weight="bold")
    bss_hv_sda, bss_hv_scl = (720, ty_), (760, ty_)
    bss_lv_sda, bss_lv_scl = (tx_ + tw_, 676), (tx_ + tw_, 700)
    bss_lv, bss_hv, bss_gnd = (tx_ + tw_, 668), (700, ty_ + th_), (760, ty_ + th_)

    sx_, sy_, sw_, sh_ = 940, 380, 190, 180
    panel(sx_, sy_, sw_, sh_, "#ffffff", "#d7dbe2", rx=6, sw=1.5)
    text(sx_ + sw_ / 2, sy_ + 32, "ESP32 DevKit V1", 12, C["ink"],
         anchor="middle", weight="bold")
    stub = {}
    for i, (name, col) in enumerate((("3V3", C["w_33"]), ("5V", C["w_5v"]),
                                     ("GND", C["w_gnd"]), ("SDA", C["w_i2c"]),
                                     ("SCL", C["w_i2c"]))):
        qy = sy_ + 62 + i * 22
        stub[name] = (sx_, qy)
        circle(sx_, qy, 4, col, "#7d8189", 1)
        text(sx_ + 12, qy + 4, name, 9.5, C["muted"])

    # ---- supply rails ----
    rail_5v, rail_gnd = 780, 806
    line(60, rail_5v, W - 60, rail_5v, C["rail_r"], 2.5)
    line(60, rail_gnd, W - 60, rail_gnd, C["rail_b"], 2.5)
    # clear of the descending supply wires on the left
    text(300, rail_5v - 8, "+5 V", 10, C["w_5v"], weight="bold")
    text(300, rail_gnd + 18, "GND", 10, C["w_gnd"], weight="bold")
    wire([stub["5V"], (912, stub["5V"][1]), (912, 744), (620, 744),
          (620, rail_5v)], C["w_5v"])
    wire([stub["GND"], (900, stub["GND"][1]), (900, 756), (600, 756),
          (600, rail_gnd)], C["w_gnd"])

    # =============================================================== wires ==
    # LCD supply and R/W run out to the left, nested so none crosses another
    for name, corridor, chan, col, rail in (("GND", 214, 60, C["w_gnd"], rail_gnd),
                                            ("VCC", 222, 76, C["w_5v"], rail_5v),
                                            ("RW", 238, 92, C["w_gnd"], rail_gnd)):
        wire([lcd[name], (lcd[name][0], corridor), (chan, corridor),
              (chan, rail)], col)

    # contrast trimmer: Vo drops straight down, the ends go left to the rails
    wire([lcd["Vo"], pot_w], C["w_sig"])
    wire([pot_5v, (108, pot_5v[1]), (108, rail_5v)], C["w_5v"])
    wire([pot_gnd, (124, pot_gnd[1]), (124, rail_gnd)], C["w_gnd"])

    # backlight: anode through 220R to +5 V, cathode to ground
    wire([lcd["A"], (lcd["A"][0], 214), (912, 214), (912, stub["5V"][1]),
          stub["5V"]], C["w_5v"])
    rect(904, 300, 16, 44, "#d8c9a3", "#9a8b66", rx=3, sw=1)
    for i, bc in enumerate(("#8b1a1a", "#c02020", "#3b2b1a")):
        rect(904, 308 + i * 9, 16, 5, bc)
    text(926, 328, "220R", 8.5, C["muted"])
    wire([lcd["K"], (lcd["K"][0], 222), (896, 222), (896, stub["GND"][1]),
          stub["GND"]], C["w_gnd"])

    # LCD -> Arduino, 4-bit parallel. Corridors are ordered so that no drop
    # crosses another corridor: LiquidCrystal(8, 9, 10, 11, 12, 13).
    for src, dst, corridor in (("E", "D9", 250), ("RS", "D8", 258),
                               ("D4", "D10", 266), ("D5", "D11", 274),
                               ("D6", "D12", 282), ("D7", "D13", 290)):
        wire([lcd[src], (lcd[src][0], corridor), (dig[dst][0], corridor),
              dig[dst]], C["w_spi"])

    # Arduino supply
    wire([ard["5V"], (ard["5V"][0], rail_5v)], C["w_5v"])
    wire([ard["GND"], (ard["GND"][0], rail_gnd)], C["w_gnd"])

    # Arduino I2C -> translator high side
    wire([ard["A4"], (ard["A4"][0], 652), (bss_hv_sda[0], 652), bss_hv_sda],
         C["w_i2c"])
    wire([ard["A5"], (ard["A5"][0], 640), (bss_hv_scl[0], 640), bss_hv_scl],
         C["w_i2c"])
    # translator low side -> the connector, SDA outboard of SCL so they nest
    wire([bss_lv_sda, (908, bss_lv_sda[1]), (908, stub["SDA"][1]),
          stub["SDA"]], C["w_i2c"])
    wire([bss_lv_scl, (926, bss_lv_scl[1]), (926, stub["SCL"][1]),
          stub["SCL"]], C["w_i2c"])
    # translator references: HV off the 5 V rail, LV off the ESP32's 3V3
    wire([bss_hv, (bss_hv[0], rail_5v)], C["w_5v"])
    wire([bss_gnd, (bss_gnd[0], rail_gnd)], C["w_gnd"])
    wire([bss_lv, (936, bss_lv[1]), (936, stub["3V3"][1]), stub["3V3"]],
         C["w_33"])

    legend(620, 400, [(C["w_5v"], "+5 V island rail"),
                     (C["w_gnd"], "GND / common return"),
                     (C["w_33"], "+3.3 V reference from the ESP32"),
                     (C["w_spi"], "LCD 4-bit parallel bus"),
                     (C["w_i2c"], "I2C to the enforcement node"),
                     (C["w_sig"], "contrast (analogue)")], cols=1)
    text(28, H - 18,
         "Generated by schematic/svggen.py - do not edit by hand. "
         "Pin assignment authoritative in README.md section 4.2.",
         10.5, C["muted"], style="italic")
    svg_close(path)


if __name__ == "__main__":
    print("Generating SVG sources...")
    breadboard_view(os.path.join(HERE, "smart-gateway-breadboard.svg"))
    schematic_view(os.path.join(HERE, "smart-gateway-schematic.svg"))
    display_view(os.path.join(HERE, "smart-gateway-display.svg"))
    print("Done.")
