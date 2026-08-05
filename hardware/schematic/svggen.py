#!/usr/bin/env python3
"""
Generate the breadboard SVG source for the smart gateway bench wiring.

Only the breadboard view is generated here; the schematic is maintained by hand
in EAGLE as smart-gateway.sch. Re-run after any change to the pin assignment
in README.md section 4.2. Dependency-free: emits SVG text directly.
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


# Every wire end must land on a declared pin or on a breadboard rail. These
# registries let validate_wires() catch jumpers that stop in mid-air.
ANCHORS = []          # (x, y) pin positions
RAILS = []            # (y, x_from, x_to) horizontal breadboard rails
WIRE_ENDS = []        # (x, y) endpoints to verify


def anchor(x, y):
    """Register a connectable pin position and return it."""
    ANCHORS.append((x, y))
    return (x, y)


def rail(y, x0, x1):
    RAILS.append((y, x0, x1))


def validate_wires(tol=4):
    bad = []
    for (x, y) in WIRE_ENDS:
        if any(abs(x - ax) <= tol and abs(y - ay) <= tol for ax, ay in ANCHORS):
            continue
        if any(abs(y - ry) <= tol and x0 - tol <= x <= x1 + tol
               for ry, x0, x1 in RAILS):
            continue
        bad.append((x, y))
    if bad:
        raise SystemExit(
            "svggen: %d wire end(s) connect to nothing: %s"
            % (len(bad), ", ".join(f"({x},{y})" for x, y in bad)))
    print(f"  validated {len(WIRE_ENDS)} wire ends against "
          f"{len(ANCHORS)} pins and {len(RAILS)} rails")


def wire(pts, stroke, sw=3):
    """Orthogonal-ish wire through a point list, drawn with rounded joins."""
    WIRE_ENDS.extend([tuple(pts[0]), tuple(pts[-1])])
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


def svg_open(w, h, title, view=None):
    _out.clear()
    vx, vy, vw, vh = view or (0, 0, w, h)
    add(f'<svg xmlns="http://www.w3.org/2000/svg" width="{vw}" height="{vh}" '
        f'viewBox="{vx} {vy} {vw} {vh}">')
    add(f'<title>{esc(title)}</title>')
    rect(vx, vy, vw, vh, C["bg"])


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


# ========================================================= BREADBOARD VIEW ===
def breadboard_view(path):
    W, H = 1700, 1200
    svg_open(W, H, "Smart Gateway - breadboard view",
             view=(40, 70, 1590, 1120))

    # ---- Arduino UNO -------------------------------------------------------
    ux, uy, uw, uh = 470, 330, 470, 340
    panel(ux, uy, uw, uh, C["uno"], C["uno_dark"], rx=14, sw=2)
    rect(ux + 16, uy + 236, 122, 46, "#3b3f46", rx=4)       # MCU
    text(ux + 77, uy + 264, "ATmega328P", 9.5, "#e6e8ec", anchor="middle")
    rect(ux - 4, uy + 60, 46, 60, "#9aa1ab", rx=3)          # USB
    text(ux + 19, uy + 96, "USB", 9, "#2b2f36", anchor="middle")
    rect(ux - 4, uy + 236, 40, 40, "#23262b", rx=6)         # barrel jack
    text(ux + uw / 2, uy + 160, "Arduino UNO R3", 21, "#ffffff",
         anchor="middle", weight="bold")
    text(ux + uw / 2, uy + 184, "Policy Enforcement Point", 12, "#cfeced",
         anchor="middle")

    # digital header (top edge): D13 at the left, as on the physical board
    pitch = 26
    dig = {}
    for i, name in enumerate(["D13", "D12", "D11", "D10", "D9", "D8"]):
        dig[name] = (560 + i * pitch, uy)
    for i, name in enumerate(["D7", "D6", "D5", "D4", "D3", "D2", "D1", "D0"]):
        dig[name] = (728 + i * pitch, uy)
    header(dig["D13"][0], uy, 6, pitch)
    header(dig["D7"][0], uy, 8, pitch)
    # labels sit inside the PCB so that jumper runs never cross them
    for name, (px, py) in dig.items():
        text(px, py + 26, name, 9.5, "#d6f0f1", anchor="middle")

    # power + analog header (bottom edge)
    ana = {}
    for i, name in enumerate(["3V3", "5V", "GND", "GND2", "VIN"]):
        ana[name] = (560 + i * pitch, uy + uh)
    for i, name in enumerate(["A0", "A1", "A2", "A3", "A4", "A5"]):
        ana[name] = (760 + i * pitch, uy + uh)
    header(ana["3V3"][0], uy + uh, 5, pitch)
    header(ana["A0"][0], uy + uh, 6, pitch)
    for name, (px, py) in ana.items():
        text(px, py - 18, name.replace("GND2", "GND"), 9.5, "#d6f0f1",
             anchor="middle")

    # ---- LCD 1602 (top centre) --------------------------------------------
    lx, ly, lw, lh = 390, 92, 380, 118
    panel(lx, ly, lw, lh, C["lcd"], "#14532a", rx=6)
    rect(lx + 30, ly + 14, 320, 64, C["lcd_scr"], "#6f9a24", rx=3, sw=1.5)
    text(lx + 46, ly + 40, "PRESENT BADGE", 13, "#2c3a12",
         family="Courier New, monospace")
    text(lx + 46, ly + 64, "GW-LAB-01", 13, "#2c3a12",
         family="Courier New, monospace")
    text(lx + lw - 14, ly + lh - 13, "LCD 1602", 11, "#cfe8d6",
         anchor="end", weight="bold")
    rect(lx + 8, ly + lh - 30, 96, 22, "#0f3d22", rx=3)
    text(lx + 56, ly + lh - 15, "PCF8574", 9, "#cfe8d6", anchor="middle")
    lcd = {}
    for i, name in enumerate(["GND", "VCC", "SDA", "SCL"]):
        px = lx + 130 + i * 28
        lcd[name] = (px, ly + lh)
        circle(px, ly + lh, 4.5, "#c9ccd1", "#7d8189", 1)
        text(px, ly + lh - 12, name, 9, "#cfe8d6", anchor="middle")

    # ---- 4x4 keypad (top right) -------------------------------------------
    kx, ky, kw, kh = 1300, 92, 300, 300
    panel(kx, ky, kw, kh, C["keypad"], "#15181c", rx=10)
    labels = [["1", "2", "3", "A"], ["4", "5", "6", "B"],
              ["7", "8", "9", "C"], ["*", "0", "#", "D"]]
    for r in range(4):
        for c in range(4):
            bx_, by_ = kx + 22 + c * 66, ky + 26 + r * 66
            fill = ("#4a4f57" if c == 3
                    else (C["key_alt"] if labels[r][c] in "*#" else C["key"]))
            rect(bx_, by_, 54, 50, fill, "#1a1d21", rx=7, sw=1.5)
            text(bx_ + 27, by_ + 33, labels[r][c], 19, "#ffffff",
                 anchor="middle", weight="bold")
    kp = {}
    for i, name in enumerate(["R1", "R2", "R3", "R4", "C1", "C2", "C3"]):
        py = ky + 34 + i * 36
        kp[name] = (kx, py)
        circle(kx, py, 4.5, "#c9ccd1", "#7d8189", 1)
        text(kx + 12, py + 4, name, 9.5, "#c9ccd1")

    # ---- RC522 (left) ------------------------------------------------------
    rx_, ry, rw, rh = 60, 380, 210, 300
    panel(rx_, ry, rw, rh, C["rc522"], "#0f4a80", rx=8)
    for i in range(4):
        rect(rx_ + 26, ry + 34 + i * 24, 158, 11, "#3f8ddb", rx=6, op=0.55)
    circle(rx_ + rw / 2, ry + 190, 46, "none", "#7fb6ea", 3)
    circle(rx_ + rw / 2, ry + 190, 30, "none", "#7fb6ea", 3)
    text(rx_ + rw / 2, ry + 258, "MFRC522", 14, "#ffffff", anchor="middle",
         weight="bold")
    text(rx_ + rw / 2, ry + 277, "13.56 MHz", 10.5, "#bcd9f5", anchor="middle")
    rc = {}
    for i, name in enumerate(["SDA", "SCK", "MOSI", "MISO", "GND", "RST", "3V3"]):
        py = ry + 26 + i * 38
        rc[name] = (rx_ + rw, py)
        circle(rx_ + rw, py, 4.5, "#c9ccd1", "#7d8189", 1)
        text(rx_ + rw - 12, py + 4, name, 9.5, "#cfe4f7", anchor="end")

    # ---- breadboard (bottom left) -----------------------------------------
    bx, by, bw, bh = 60, 830, 620, 180
    panel(bx, by, bw, bh, C["bb"], C["bb_line"], rx=6, sw=1.5)
    rail_top_r, rail_top_b = by + 14, by + 26
    rail_bot_r, rail_bot_b = by + bh - 26, by + bh - 14
    for yr, yb in ((rail_top_r, rail_top_b), (rail_bot_r, rail_bot_b)):
        line(bx + 16, yr, bx + bw - 16, yr, C["rail_r"], 1.5)
        line(bx + 16, yb, bx + bw - 16, yb, C["rail_b"], 1.5)
    for col in range(int((bw - 44) / 14)):
        for row in range(5):
            circle(bx + 28 + col * 14, by + 48 + row * 12, 1.9, "#b9bab6")
            circle(bx + 28 + col * 14, by + 116 + row * 12, 1.9, "#b9bab6")
    line(bx + 16, by + 108, bx + bw - 16, by + 108, "#dcdcd8", 6)

    led_r = (bx + 130, by + 78)
    led_g = (bx + 230, by + 78)
    buzz = (bx + 380, by + 84)
    for (cx, cy), col, lab in ((led_r, C["rail_r"], "DENY"),
                               (led_g, "#2fa84f", "GRANT")):
        circle(cx, cy, 13, col, "#00000033", 1.5)
        circle(cx - 4, cy - 4, 4, "#ffffff", op=0.6)
        text(cx + 22, cy + 4, lab, 9.5, C["muted"])
        rect(cx - 26, cy + 34, 52, 12, "#d8c9a3", "#9a8b66", rx=3, sw=1)
        for i, bc in enumerate(("#8b1a1a", "#c02020", "#3b2b1a")):
            rect(cx - 16 + i * 9, cy + 34, 5, 12, bc)
        text(cx + 32, cy + 44, "220R", 9, C["muted"])
    circle(buzz[0], buzz[1], 22, "#1b1f27", "#43474e", 2)
    circle(buzz[0], buzz[1], 5, "#43474e")
    text(buzz[0] + 30, buzz[1] + 4, "BUZZER", 9.5, C["muted"])

    # ---- ESP32-CAM ---------------------------------------------------------
    ex, ey, ew, eh = 1010, 470, 220, 210
    panel(ex, ey, ew, eh, C["cam"], "#15181c", rx=8)
    rect(ex + 20, ey + 20, 84, 84, "#3a3f47", rx=6)
    circle(ex + 62, ey + 62, 28, "#0d0f12", "#5a6069", 3)
    circle(ex + 62, ey + 62, 14, "#1d3a5c")
    circle(ex + 54, ey + 54, 5, "#8fb8e8", op=0.8)
    rect(ex + 124, ey + 24, 74, 52, "#8f949c", rx=4)
    text(ex + 161, ey + 54, "microSD", 8.5, "#23262b", anchor="middle")
    text(ex + ew / 2, ey + 138, "ESP32-CAM", 14, "#ffffff", anchor="middle",
         weight="bold")
    text(ex + ew / 2, ey + 156, "MJPEG2SD recorder", 10, "#a9b0ba",
         anchor="middle")
    cam = {}
    for i, name in enumerate(["GND", "5V", "G13", "G12", "G16"]):
        px = ex + 22 + i * 44
        cam[name] = (px, ey + eh)
        circle(px, ey + eh, 4.5, "#c9ccd1", "#7d8189", 1)
        text(px, ey + eh - 12, name, 8.5, "#c9ccd1", anchor="middle")

    lsx, lsy = 1090, 730
    panel(lsx, lsy, 150, 60, "#e8d9b0", "#b9a678", rx=5, sw=1.5)
    text(lsx + 75, lsy + 26, "BSS138", 11, "#4a4033", anchor="middle",
         weight="bold")
    text(lsx + 75, lsy + 44, "5 V <-> 3.3 V", 9.5, "#6b6252", anchor="middle")

    # ---- servo -------------------------------------------------------------
    sx, sy = 1420, 620
    panel(sx, sy, 170, 110, C["servo"], "#1c5a9e", rx=6)
    rect(sx + 118, sy + 14, 44, 34, "#7fb6ea", rx=4)
    circle(sx + 140, sy + 62, 22, "#e8eef5", "#9fb4c9", 2)
    line(sx + 140, sy + 62, sx + 140, sy + 26, "#9fb4c9", 5)
    text(sx + 56, sy + 68, "SG90", 15, "#ffffff", weight="bold")
    sv = {}
    for i, name in enumerate(["SIG", "5V", "GND"]):
        py = sy + 26 + i * 30
        sv[name] = (sx, py)
        circle(sx, py, 4.5, "#c9ccd1", "#7d8189", 1)
        text(sx + 12, py + 4, name, 9.5, "#dceaf7")

    # ---- external supply ---------------------------------------------------
    psx, psy = 730, 1040
    panel(psx, psy, 210, 66, "#ffffff", "#d7dbe2", rx=6, sw=1.5)
    text(psx + 105, psy + 26, "External 5 V / 2 A", 12, C["ink"],
         anchor="middle", weight="bold")
    text(psx + 105, psy + 48, "grounds commoned to UNO", 10, C["muted"],
         anchor="middle")

    # ================================================================ wires ==
    # register every connectable point before routing
    for d in (dig, ana, lcd, kp, rc, cam, sv):
        for pt in d.values():
            anchor(*pt)
    for (cx, cy) in (led_r, led_g):
        anchor(cx, cy - 13)          # anode
        anchor(cx, cy + 46)          # resistor tail
    anchor(buzz[0], buzz[1] - 22)
    anchor(buzz[0], buzz[1] + 22)
    anchor(lsx, lsy + 20)            # level shifter HV side
    anchor(lsx, lsy + 44)
    anchor(cam["G13"][0], lsy)       # level shifter LV side
    anchor(cam["G12"][0], lsy)
    anchor(psx, psy + 20)            # external supply terminals
    anchor(psx, psy + 44)
    for ry in (rail_top_r, rail_top_b, rail_bot_r, rail_bot_b):
        rail(ry, bx + 16, bx + bw - 16)

    # RC522 SPI -> UNO digital header
    for (pin, dpin, vx, band) in (("SCK",  "D13", 300, 288),
                                  ("MISO", "D12", 316, 264),
                                  ("MOSI", "D11", 332, 276),
                                  ("SDA",  "D10", 348, 300),
                                  ("RST",  "D9",  364, 252)):
        wire([rc[pin], (vx, rc[pin][1]), (vx, band), (dig[dpin][0], band),
              dig[dpin]], C["w_spi"])
    wire([rc["3V3"], (380, rc["3V3"][1]), (380, 678), (ana["3V3"][0], 678),
          ana["3V3"]], C["w_33"])
    wire([rc["GND"], (396, rc["GND"][1]), (396, 690), (ana["GND"][0], 690),
          ana["GND"]], C["w_gnd"])

    # LCD: down the left of the UNO, round to the analog header
    for (pin, apin, band_t, vx, band_b, col) in (
            ("GND", "GND2", 250, 404, 760, C["w_gnd"]),
            ("VCC", "5V",   262, 416, 748, C["w_5v"]),
            ("SDA", "A4",   274, 428, 700, C["w_i2c"]),
            ("SCL", "A5",   286, 440, 712, C["w_i2c"])):
        wire([lcd[pin], (lcd[pin][0], band_t), (vx, band_t), (vx, band_b),
              (ana[apin][0], band_b), ana[apin]], col)

    # keypad rows -> D5..D2, columns -> A3..A1
    for i, (pin, dpin) in enumerate((("R1", "D5"), ("R2", "D4"),
                                     ("R3", "D3"), ("R4", "D2"))):
        vx, band = 1270 - i * 14, 228 - i * 12
        wire([kp[pin], (vx, kp[pin][1]), (vx, band), (dig[dpin][0], band),
              dig[dpin]], C["w_kp"])
    for i, (pin, apin) in enumerate((("C1", "A3"), ("C2", "A2"), ("C3", "A1"))):
        vx, band = 1290 - i * 14, 826 + i * 12
        wire([kp[pin], (vx, kp[pin][1]), (vx, band), (ana[apin][0], band),
              ana[apin]], C["w_kp"])

    # servo
    wire([sv["SIG"], (1400, sv["SIG"][1]), (1400, 808), (ana["A0"][0], 808),
          ana["A0"]], C["w_sig"])
    wire([sv["5V"], (1350, sv["5V"][1]), (1350, 1160), (640, 1160),
          (640, rail_bot_r)], C["w_5v"])
    wire([sv["GND"], (1336, sv["GND"][1]), (1336, 1174), (620, 1174),
          (620, rail_bot_b)], C["w_gnd"])

    # ESP32-CAM: I2C through the level shifter, trigger, power
    wire([cam["G13"], (cam["G13"][0], lsy)], C["w_i2c"])
    wire([cam["G12"], (cam["G12"][0], lsy)], C["w_i2c"])
    wire([(lsx, lsy + 20), (975, lsy + 20), (975, 724), (ana["A4"][0], 724),
          ana["A4"]], C["w_i2c"])
    wire([(lsx, lsy + 44), (960, lsy + 44), (960, 736), (ana["A5"][0], 736),
          ana["A5"]], C["w_i2c"])
    wire([cam["G16"], (cam["G16"][0], 700), (1250, 700), (1250, 180),
          (dig["D8"][0], 180), dig["D8"]], C["w_trig"])
    wire([cam["5V"], (cam["5V"][0], 1146), (660, 1146), (660, rail_bot_r)],
         C["w_5v"])
    wire([cam["GND"], (cam["GND"][0], 1132), (600, 1132), (600, rail_bot_b)],
         C["w_gnd"])

    # annunciators -> UNO
    for (cx, cy), dpin, band in ((led_r, "D6", 796), (led_g, "D7", 784)):
        wire([(cx, cy - 13), (cx, band), (dig[dpin][0], band), dig[dpin]],
             C["w_sig"])
        wire([(cx, cy + 46), (cx, rail_bot_b)], C["w_gnd"], 2.5)
    wire([(buzz[0], buzz[1] - 22), (buzz[0], 772), (dig["D8"][0], 772),
          dig["D8"]], C["w_sig"])
    wire([(buzz[0], buzz[1] + 22), (buzz[0], rail_bot_b)], C["w_gnd"], 2.5)

    # UNO power tie to the breadboard rails
    wire([ana["5V"], (ana["5V"][0], rail_top_r)], C["w_5v"])
    wire([ana["GND2"], (ana["GND2"][0], rail_top_b)], C["w_gnd"])

    # external supply -> breadboard rails
    wire([(psx, psy + 20), (650, psy + 20), (650, rail_bot_r)], C["w_5v"])
    wire([(psx, psy + 44), (580, psy + 44), (580, rail_bot_b)], C["w_gnd"])

    validate_wires()
    svg_close(path)


if __name__ == "__main__":
    print("Generating breadboard SVG...")
    breadboard_view(os.path.join(HERE, "smart-gateway-breadboard.svg"))
    print("Done.")
