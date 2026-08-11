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


def circle(cx, cy, r, fill, stroke="none", sw=1):
    add(f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{fill}" '
        f'stroke="{stroke}" stroke-width="{sw}"/>')


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
    (C["w_33"],   "+3.3 V (RC522 only)"),
    (C["w_spi"],  "SPI - RC522"),
    (C["w_i2c"],  "I2C - LCD and ESP32-CAM"),
    (C["w_kp"],   "Keypad matrix"),
    (C["w_sig"],  "Discrete signals"),
    (C["w_trig"], "REC_TRIG to recorder"),
]


# Breadboard view: physical layout of the enforcement node and peripherals, with pin headers and wiring.
def breadboard_view(path):
    W, H = 1560, 1020
    svg_open(W, H, "Smart Gateway - breadboard view")
    frame(W, H, "Smart Gateway - Breadboard View",
          "Arduino UNO R3 enforcement node with RC522, 1602 I2C LCD, 4x4 keypad, "
          "SG90 servo and ESP32-CAM recorder")

    # ---- Arduino UNO ----
    ux, uy, uw, uh = 470, 330, 470, 340
    panel(ux, uy, uw, uh, C["uno"], C["uno_dark"], rx=14, sw=2)
    rect(ux + 14, uy + 250, 120, 46, "#3b3f46", rx=4)      # MCU
    text(ux + 74, uy + 278, "ATmega328P", 9.5, "#e6e8ec", anchor="middle")
    rect(ux - 4, uy + 60, 46, 60, "#9aa1ab", rx=3)          # USB
    text(ux + 19, uy + 96, "USB", 9, "#2b2f36", anchor="middle")
    rect(ux - 4, uy + 240, 40, 40, "#23262b", rx=6)         # barrel jack
    text(ux + uw / 2, uy + 180, "Arduino UNO R3", 20, "#ffffff",
         anchor="middle", weight="bold")
    text(ux + uw / 2, uy + 202, "Policy Enforcement Point", 12, "#cfeced",
         anchor="middle")

    # digital header (top edge) - D13 .. D0, right to left
    dig = {}
    x0, pitch = 900, 26
    for i, name in enumerate(["D13", "D12", "D11", "D10", "D9", "D8"]):
        dig[name] = (x0 - i * pitch, uy)
    x1 = x0 - 6 * pitch - 12
    for i, name in enumerate(["D7", "D6", "D5", "D4", "D3", "D2", "D1", "D0"]):
        dig[name] = (x1 - i * pitch, uy)
    header(dig["D8"][0], uy, 6, pitch)
    header(dig["D0"][0], uy, 8, pitch)
    for name, (px, py) in dig.items():
        text(px, py - 16, name, 9.5, C["muted"], anchor="middle")

    # power + analog header (bottom edge)
    ana = {}
    for i, name in enumerate(["3V3", "5V", "GND", "GND2", "VIN"]):
        ana[name] = (560 + i * pitch, uy + uh)
    for i, name in enumerate(["A0", "A1", "A2", "A3", "A4", "A5"]):
        ana[name] = (760 + i * pitch, uy + uh)
    header(ana["3V3"][0], uy + uh, 5, pitch)
    header(ana["A0"][0], uy + uh, 6, pitch)
    for name, (px, py) in ana.items():
        text(px, py + 26, name.replace("GND2", "GND"), 9.5, C["muted"],
             anchor="middle")

    # ---- LCD 1602 (top centre) ----
    lx, ly, lw, lh = 390, 92, 380, 118
    panel(lx, ly, lw, lh, C["lcd"], "#14532a", rx=6)
    rect(lx + 30, ly + 16, 320, 68, C["lcd_scr"], "#6f9a24", rx=3, sw=1.5)
    text(lx + 46, ly + 44, "PRESENT BADGE", 13, "#2c3a12", family="Courier New, monospace")
    text(lx + 46, ly + 68, "GW-LAB-01", 13, "#2c3a12", family="Courier New, monospace")
    rect(lx + 8, ly + lh - 26, 96, 22, "#0f3d22", rx=3)
    text(lx + 56, ly + lh - 11, "PCF8574", 9, "#cfe8d6", anchor="middle")
    text(lx + lw / 2, ly - 12, "LCD 1602 + I2C backpack (addr 0x27)", 12.5,
         C["ink"], anchor="middle", weight="bold")
    lcd_pins = {}
    for i, name in enumerate(["GND", "VCC", "SDA", "SCL"]):
        px = lx + 130 + i * 28
        lcd_pins[name] = (px, ly + lh)
        circle(px, ly + lh, 4.5, "#c9ccd1", "#7d8189", 1)
        text(px, ly + lh + 18, name, 9, C["muted"], anchor="middle")

    # ---- 4x4 keypad (top right) ----
    kx, ky, kw, kh = 1150, 92, 300, 300
    panel(kx, ky, kw, kh, C["keypad"], "#15181c", rx=10)
    labels = [["1", "2", "3", "A"], ["4", "5", "6", "B"],
              ["7", "8", "9", "C"], ["*", "0", "#", "D"]]
    for r in range(4):
        for c in range(4):
            bx, by = kx + 22 + c * 66, ky + 26 + r * 66
            unscanned = (c == 3)
            fill = "#4a4f57" if unscanned else (C["key_alt"] if labels[r][c] in "*#" else C["key"])
            rect(bx, by, 54, 50, fill, "#1a1d21", rx=7, sw=1.5)
            text(bx + 27, by + 33, labels[r][c], 19, "#ffffff",
                 anchor="middle", weight="bold")
    text(kx + kw / 2, ky - 12, "4x4 matrix keypad", 12.5, C["ink"],
         anchor="middle", weight="bold")
    text(kx + kw / 2, ky + kh + 22, "column D unscanned - pin budget exhausted",
         10.5, C["muted"], anchor="middle", style="italic")
    kp = {}
    for i, name in enumerate(["R1", "R2", "R3", "R4", "C1", "C2", "C3"]):
        py = ky + 34 + i * 36
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
    text(rx_ + rw / 2, ry - 12, "RC522 reader (3.3 V)", 12.5, C["ink"],
         anchor="middle", weight="bold")
    rcp = {}
    for i, name in enumerate(["SDA", "SCK", "MOSI", "MISO", "GND", "RST", "3V3"]):
        py = ry + 26 + i * 38
        rcp[name] = (rx_ + rw, py)
        circle(rx_ + rw, py, 4.5, "#c9ccd1", "#7d8189", 1)
        text(rx_ + rw + 12, py + 4, name, 9.5, C["muted"])

    # ---- breadboard (bottom left) ----
    bx, by, bw, bh = 60, 730, 620, 200
    panel(bx, by, bw, bh, C["bb"], C["bb_line"], rx=6, sw=1.5)
    for ry_ in (by + 14, by + bh - 26):
        line(bx + 16, ry_, bx + bw - 16, ry_, C["rail_r"], 1.5)
        line(bx + 16, ry_ + 12, bx + bw - 16, ry_ + 12, C["rail_b"], 1.5)
    for col in range(int((bw - 40) / 14)):
        for row in range(5):
            circle(bx + 26 + col * 14, by + 52 + row * 13, 1.9, "#b9bab6")
        for row in range(5):
            circle(bx + 26 + col * 14, by + 128 + row * 13, 1.9, "#b9bab6")
    line(bx + 16, by + 118, bx + bw - 16, by + 118, "#dcdcd8", 6)
    text(bx + bw / 2, by - 12, "Annunciation and actuation harness", 12.5,
         C["ink"], anchor="middle", weight="bold")

    # LEDs, resistors, buzzer on the breadboard
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

    # ---- servo (bottom right) ----
    sx, sy = 1180, 800
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

    # ---- ESP32-CAM (right of UNO) ----
    ex, ey, ew, eh = 990, 470, 220, 210
    panel(ex, ey, ew, eh, C["cam"], "#15181c", rx=8)
    rect(ex + 20, ey + 20, 84, 84, "#3a3f47", rx=6)
    circle(ex + 62, ey + 62, 28, "#0d0f12", "#5a6069", 3)
    circle(ex + 62, ey + 62, 14, "#1d3a5c")
    circle(ex + 54, ey + 54, 5, "#8fb8e8", op=0.8)
    rect(ex + 124, ey + 24, 74, 52, "#8f949c", rx=4)
    text(ex + 161, ey + 54, "microSD", 8.5, "#23262b", anchor="middle")
    text(ex + ew / 2, ey + 136, "ESP32-CAM", 14, "#ffffff", anchor="middle",
         weight="bold")
    text(ex + ew / 2, ey + 154, "MJPEG2SD recorder", 10, "#a9b0ba",
         anchor="middle")
    text(ex + ew / 2, ey + 172, "AI-Thinker / OV2640", 9.5, "#7f868f",
         anchor="middle")
    text(ex + ew / 2, ey - 12, "Movement recorder node", 12.5, C["ink"],
         anchor="middle", weight="bold")
    cam = {}
    for i, name in enumerate(["G13", "G12", "G16", "5V", "GND"]):
        px = ex + 22 + i * 44
        cam[name] = (px, ey + eh)
        circle(px, ey + eh, 4.5, "#c9ccd1", "#7d8189", 1)
        text(px, ey + eh + 18, name, 8.5, C["muted"], anchor="middle")

    # ---- level shifter between UNO and ESP32-CAM ----
    lsx, lsy = 960, 700
    panel(lsx, lsy, 190, 56, "#e8d9b0", "#b9a678", rx=5, sw=1.5)
    text(lsx + 95, lsy + 24, "BSS138 level shifter", 10.5, "#4a4033",
         anchor="middle", weight="bold")
    text(lsx + 95, lsy + 42, "5 V  <->  3.3 V", 10, "#6b6252", anchor="middle")

    # ================================================================ wires ==
    # RC522 SPI -> UNO
    wire([rcp["SDA"], (330, 356), (330, 300), (dig["D10"][0], 300),
          dig["D10"]], C["w_spi"])
    wire([rcp["SCK"], (346, 394), (346, 288), (dig["D13"][0], 288),
          dig["D13"]], C["w_spi"])
    wire([rcp["MOSI"], (362, 432), (362, 276), (dig["D11"][0], 276),
          dig["D11"]], C["w_spi"])
    wire([rcp["MISO"], (378, 470), (378, 264), (dig["D12"][0], 264),
          dig["D12"]], C["w_spi"])
    wire([rcp["RST"], (394, 546), (394, 252), (dig["D9"][0], 252),
          dig["D9"]], C["w_spi"])
    # RC522 power
    wire([rcp["3V3"], (410, 584), (410, 700), (ana["3V3"][0], 700),
          ana["3V3"]], C["w_33"])
    wire([rcp["GND"], (300, 508), (300, 690), (ana["GND"][0], 690),
          ana["GND"]], C["w_gnd"])

    # LCD I2C + power
    wire([lcd_pins["SDA"], (lcd_pins["SDA"][0], 250), (430, 250), (430, 712),
          (ana["A4"][0], 712), ana["A4"]], C["w_i2c"])
    wire([lcd_pins["SCL"], (lcd_pins["SCL"][0], 240), (420, 240), (420, 724),
          (ana["A5"][0], 724), ana["A5"]], C["w_i2c"])
    wire([lcd_pins["VCC"], (lcd_pins["VCC"][0], 262), (444, 262), (444, 700),
          (ana["5V"][0], 700), ana["5V"]], C["w_5v"])
    wire([lcd_pins["GND"], (lcd_pins["GND"][0], 274), (456, 274), (456, 688),
          (ana["GND2"][0], 688), ana["GND2"]], C["w_gnd"])

    # keypad rows -> D5..D2, columns -> A3..A1
    for i, (pin, dpin) in enumerate((("R1", "D5"), ("R2", "D4"),
                                     ("R3", "D3"), ("R4", "D2"))):
        yb = 236 - i * 12
        wire([kp[pin], (1090 - i * 14, kp[pin][1]), (1090 - i * 14, yb),
              (dig[dpin][0], yb), dig[dpin]], C["w_kp"])
    for i, (pin, apin) in enumerate((("C1", "A3"), ("C2", "A2"), ("C3", "A1"))):
        yb = 736 + i * 12
        wire([kp[pin], (1000 + i * 16, kp[pin][1]), (1000 + i * 16, yb),
              (ana[apin][0], yb), ana[apin]], C["w_kp"])

    # servo
    wire([svp["SIG"], (1150, svp["SIG"][1]), (1150, 772), (ana["A0"][0], 772),
          ana["A0"]], C["w_sig"])
    wire([svp["5V"], (1120, svp["5V"][1]), (1120, 960), (bx + 560, 960),
          (bx + 560, by + bh - 26)], C["w_5v"])
    wire([svp["GND"], (1100, svp["GND"][1]), (1100, 976), (bx + 520, 976),
          (bx + 520, by + bh - 14)], C["w_gnd"])

    # breadboard annunciators -> UNO
    wire([(led_r[0], led_r[1] - 13), (led_r[0], 700), (dig["D6"][0], 700),
          (dig["D6"][0], uy + uh + 40), (dig["D6"][0], uy)], C["w_sig"])
    wire([(led_g[0], led_g[1] - 13), (led_g[0], 688), (dig["D7"][0], 688),
          (dig["D7"][0], uy)], C["w_sig"])
    wire([(buzz[0], buzz[1] - 24), (buzz[0], 676), (dig["D8"][0], 676),
          dig["D8"]], C["w_sig"])
    # LED cathodes -> breadboard ground rail
    for cx, cy in (led_r, led_g):
        wire([(cx, cy + 70), (cx, by + bh - 14)], C["w_gnd"], 2.5)
    wire([(buzz[0], buzz[1] + 24), (buzz[0], by + bh - 14)], C["w_gnd"], 2.5)
    # UNO ground -> breadboard rail
    wire([ana["GND"], (ana["GND"][0], 960), (bx + 400, 960),
          (bx + 400, by + bh - 14)], C["w_gnd"])

    # ESP32-CAM I2C via level shifter, trigger, power
    wire([cam["G13"], (cam["G13"][0], 700), (lsx + 40, 700)], C["w_i2c"])
    wire([cam["G12"], (cam["G12"][0], 690), (lsx + 80, 690),
          (lsx + 80, lsy)], C["w_i2c"])
    wire([(lsx, lsy + 28), (930, lsy + 28), (930, 736), (ana["A4"][0], 736),
          ana["A4"]], C["w_i2c"])
    wire([(lsx, lsy + 44), (944, lsy + 44), (944, 748), (ana["A5"][0], 748),
          ana["A5"]], C["w_i2c"])
    wire([cam["G16"], (cam["G16"][0], 760), (960, 760), (960, 244),
          (dig["D8"][0], 244), dig["D8"]], C["w_trig"])
    wire([cam["5V"], (cam["5V"][0], 940), (bx + 600, 940),
          (bx + 600, by + bh - 26)], C["w_5v"])
    wire([cam["GND"], (cam["GND"][0], 952), (bx + 580, 952),
          (bx + 580, by + bh - 14)], C["w_gnd"])

    # external supply
    psx, psy = 700, 880
    panel(psx, psy, 200, 62, "#ffffff", "#d7dbe2", rx=6, sw=1.5)
    text(psx + 100, psy + 26, "External 5 V / 2 A", 12, C["ink"],
         anchor="middle", weight="bold")
    text(psx + 100, psy + 46, "grounds commoned to UNO", 10, C["muted"],
         anchor="middle")
    wire([(psx, psy + 20), (bx + bw - 30, psy + 20),
          (bx + bw - 30, by + bh - 26)], C["w_5v"])
    wire([(psx, psy + 44), (bx + bw - 60, psy + 44),
          (bx + bw - 60, by + bh - 14)], C["w_gnd"])

    legend(950, 800, WIRE_KEY[:4] + WIRE_KEY[4:], cols=1)
    text(28, H - 18,
         "Generated by hardware/schematic/svggen.py - do not edit by hand. "
         "Pin assignment authoritative in README.md section 3.2.",
         10.5, C["muted"], style="italic")
    svg_close(path)


# Schematic view: net-level interconnect of the enforcement node and peripherals, with pin names and wire colours.
def schematic_view(path):
    W, H = 1560, 1020
    svg_open(W, H, "Smart Gateway - schematic view")
    # light engineering grid
    for gx in range(0, W, 20):
        line(gx, 62, gx, H, C["grid"], 0.5)
    for gy in range(80, H, 20):
        line(0, gy, W, gy, C["grid"], 0.5)
    frame(W, H, "Smart Gateway - Schematic View",
          "Net-level interconnect of the enforcement node, peripherals and "
          "movement recorder")

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
    block(mx, my, mw, mh, "ATmega328P  /  Arduino UNO R3",
          "Policy Enforcement Point - deterministic FSM", C["uno"])

    left, right = {}, {}
    lnames = [("D13 SCK", C["w_spi"]), ("D12 MISO", C["w_spi"]),
              ("D11 MOSI", C["w_spi"]), ("D10 SS", C["w_spi"]),
              ("D9 RST", C["w_spi"]), ("3V3", C["w_33"]), ("5V", C["w_5v"]),
              ("GND", C["w_gnd"])]
    for i, (n, c) in enumerate(lnames):
        left[n.split()[0] if " " in n else n] = pin(mx, my + 90 + i * 46, n,
                                                    "left", c)
    rnames = [("D8 BUZZ/TRIG", C["w_trig"]), ("D7 LED_G", C["w_sig"]),
              ("D6 LED_R", C["w_sig"]), ("D5..D2 ROWS", C["w_kp"]),
              ("A3..A1 COLS", C["w_kp"]), ("A0 SERVO", C["w_sig"]),
              ("A4 SDA", C["w_i2c"]), ("A5 SCL", C["w_i2c"])]
    for i, (n, c) in enumerate(rnames):
        right[n.split()[0]] = pin(mx + mw, my + 90 + i * 46, n, "right", c)

    # ---- peripherals ----
    block(90, 200, 260, 200, "MFRC522", "ISO/IEC 14443A reader", C["rc522"])
    rc = {}
    for i, (n, c) in enumerate((("SCK", C["w_spi"]), ("MISO", C["w_spi"]),
                                ("MOSI", C["w_spi"]), ("SDA/SS", C["w_spi"]),
                                ("RST", C["w_spi"]), ("3V3", C["w_33"]))):
        rc[n] = pin(350, 240 + i * 26, n, "right", c)
    text(220, 388, "3.3 V logic - not 5 V tolerant", 10, C["key_alt"],
         anchor="middle", weight="bold")

    block(90, 470, 260, 130, "LCD 1602", "PCF8574 I2C backpack - 0x27", C["lcd"])
    lc = {}
    for i, n in enumerate(("SDA", "SCL")):
        lc[n] = pin(350, 530 + i * 30, n, "right", C["w_i2c"])

    block(90, 660, 260, 190, "Annunciators", "grant / deny / alarm", C["muted"])
    an = {}
    for i, (n, c) in enumerate((("LED_R 220R", C["w_sig"]),
                                ("LED_G 220R", C["w_sig"]),
                                ("BUZZER", C["w_sig"]))):
        an[n.split()[0]] = pin(350, 720 + i * 40, n, "right", c)

    block(1190, 180, 280, 250, "4x4 Keypad", "column D unscanned", C["keypad"])
    kp = {}
    for i, n in enumerate(("R1..R4", "C1..C3")):
        kp[n] = pin(1190, 300 + i * 46, n, "left", C["w_kp"])

    block(1190, 500, 280, 130, "SG90 Servo", "latch actuator", C["servo"])
    sv = pin(1190, 570, "SIG", "left", C["w_sig"])

    block(1190, 700, 280, 210, "ESP32-CAM", "MJPEG2SD movement recorder",
          C["cam"])
    cm = {}
    for i, (n, c) in enumerate((("G13 SDA", C["w_i2c"]), ("G12 SCL", C["w_i2c"]),
                                ("G16 REC_TRIG", C["w_trig"]))):
        cm[n.split()[0]] = pin(1190, 780 + i * 40, n, "left", c)

    # level shifter in the I2C path
    panel(1020, 780, 130, 80, "#e8d9b0", "#b9a678", rx=5, sw=1.5)
    text(1085, 812, "BSS138", 11.5, "#4a4033", anchor="middle", weight="bold")
    text(1085, 830, "5V <-> 3V3", 10, "#6b6252", anchor="middle")

    # ---- nets ----
    for a, b, c in ((rc["SCK"], left["D13"], C["w_spi"]),
                    (rc["MISO"], left["D12"], C["w_spi"]),
                    (rc["MOSI"], left["D11"], C["w_spi"]),
                    (rc["SDA/SS"], left["D10"], C["w_spi"]),
                    (rc["RST"], left["D9"], C["w_spi"])):
        midx = 420 + (list(rc.values()).index(a) if a in rc.values() else 0) * 8
        wire([a, (midx, a[1]), (midx, b[1]), b], c, 2.2)
    wire([rc["3V3"], (500, rc["3V3"][1]), (500, left["3V3"][1]),
          left["3V3"]], C["w_33"], 2.2)

    wire([lc["SDA"], (470, lc["SDA"][1]), (470, 940), (1035, 940),
          (1035, right["A4"][1]), right["A4"]], C["w_i2c"], 2.2)
    wire([lc["SCL"], (455, lc["SCL"][1]), (455, 958), (1050, 958),
          (1050, right["A5"][1]), right["A5"]], C["w_i2c"], 2.2)

    wire([an["LED_R"], (430, an["LED_R"][1]), (430, right["D6"][1] + 200),
          (1010, right["D6"][1] + 200), (1010, right["D6"][1]),
          right["D6"]], C["w_sig"], 2.2)
    wire([an["LED_G"], (444, an["LED_G"][1]), (444, right["D7"][1] + 190),
          (1000, right["D7"][1] + 190), (1000, right["D7"][1]),
          right["D7"]], C["w_sig"], 2.2)
    wire([an["BUZZER"], (458, an["BUZZER"][1]), (458, right["D8"][1] + 180),
          (990, right["D8"][1] + 180), (990, right["D8"][1]),
          right["D8"]], C["w_sig"], 2.2)

    wire([kp["R1..R4"], (1120, kp["R1..R4"][1]), (1120, right["D5"][1]),
          right["D5"]], C["w_kp"], 2.6)
    wire([kp["C1..C3"], (1105, kp["C1..C3"][1]), (1105, right["A3"][1]),
          right["A3"]], C["w_kp"], 2.6)
    wire([sv, (1090, sv[1]), (1090, right["A0"][1]), right["A0"]],
         C["w_sig"], 2.2)

    wire([cm["G13"], (1150, cm["G13"][1])], C["w_i2c"], 2.2)
    wire([cm["G12"], (1160, cm["G12"][1]), (1160, 848), (1150, 848)],
         C["w_i2c"], 2.2)
    wire([(1020, 800), (985, 800), (985, 940)], C["w_i2c"], 2.2)
    wire([(1020, 840), (970, 840), (970, 958)], C["w_i2c"], 2.2)
    junction(1035, 940, C["w_i2c"])
    junction(1050, 958, C["w_i2c"])
    wire([cm["G16"], (1170, cm["G16"][1]), (1170, 690), (975, 690),
          (975, right["D8"][1]), right["D8"]], C["w_trig"], 2.2)

    # power rails
    line(60, 130, W - 60, 130, C["w_5v"], 3)
    text(66, 122, "+5 V", 11, C["w_5v"], weight="bold")
    line(60, 990, W - 60, 990, C["w_gnd"], 3)
    text(66, 982, "GND", 11, C["w_gnd"], weight="bold")
    wire([left["5V"], (520, left["5V"][1]), (520, 130)], C["w_5v"], 2.2)
    wire([left["GND"], (505, left["GND"][1]), (505, 990)], C["w_gnd"], 2.2)
    junction(520, 130, C["w_5v"])
    junction(505, 990, C["w_gnd"])

    legend(90, 880, WIRE_KEY, cols=1)
    text(28, H - 18,
         "Generated by hardware/schematic/svggen.py - do not edit by hand. "
         "Pin assignment authoritative in README.md section 3.2.",
         10.5, C["muted"], style="italic")
    svg_close(path)


if __name__ == "__main__":
    print("Generating SVG sources...")
    breadboard_view(os.path.join(HERE, "smart-gateway-breadboard.svg"))
    schematic_view(os.path.join(HERE, "smart-gateway-schematic.svg"))
    print("Done.")
