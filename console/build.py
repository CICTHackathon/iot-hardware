#!/usr/bin/env python3
"""
Embed console/index.html into both firmwares.

The console is one page for two nodes, and it is written once. This script is
what keeps that true: it emits the same document into

    firmware/gateway-esp32/console_ui.h
    firmware/esp32cam/console_ui.h

as a PROGMEM raw string, so whichever node a browser is pointed at serves the
identical interface. The generated headers are committed, exactly like the
schematic SVGs and classifier_weights.h — the build stays a plain Arduino build
with no preprocessing step, and the generator is run by hand after an edit.

    python console/build.py
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SOURCE = os.path.join(HERE, "index.html")

TARGETS = [
    os.path.join(ROOT, "firmware", "gateway-esp32", "console_ui.h"),
    os.path.join(ROOT, "firmware", "esp32cam", "console_ui.h"),
]

DELIM = "HTML"

BANNER = """// Operator console, served from flash as a single self-contained page.
//
// GENERATED FROM console/index.html BY console/build.py - DO NOT EDIT.
// Edit the source, then re-run:  python console/build.py
//
// Both edge nodes embed this same file, so the interface a browser gets does
// not depend on which node it was pointed at. Nothing is fetched from a CDN:
// an access-control node belongs on an isolated VLAN, and a door that cannot be
// operated because a stylesheet failed to load is a door that is broken.
#pragma once

#include <pgmspace.h>

static const char CONSOLE_HTML[] PROGMEM = R"%s(
%s)%s";
"""


def squeeze(html):
    """Drop the source-only comment block and the blank lines. Nothing else.

    Indentation stays and line breaks stay. Joining JavaScript lines makes
    automatic semicolon insertion a way to change behaviour rather than size,
    and de-indenting would rewrite the inside of every template literal — both
    to save a page against four megabytes of flash.
    """
    out = []
    in_comment = False
    for line in html.splitlines():
        stripped = line.strip()
        if not in_comment and stripped.startswith("<!--") and "-->" not in stripped:
            in_comment = True
            continue
        if in_comment:
            if "-->" in stripped:
                in_comment = False
            continue
        if not stripped:
            continue
        out.append(line.rstrip())
    return "\n".join(out) + "\n"


def main():
    with open(SOURCE, encoding="utf-8") as f:
        html = f.read()

    # A raw string ends at the first occurrence of the delimiter, so a page
    # containing one would truncate the firmware's copy at that point and
    # produce a compile error a long way from the cause.
    if ')%s"' % DELIM in html:
        sys.exit("index.html contains the raw-string terminator )%s\"" % DELIM)

    body = squeeze(html)
    header = BANNER % (DELIM, body, DELIM)

    for path in TARGETS:
        if not os.path.isdir(os.path.dirname(path)):
            sys.exit("no such firmware directory: " + os.path.dirname(path))
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(header)
        print("%-46s %6d bytes" % (os.path.relpath(path, ROOT), len(header)))

    print("source %d bytes, embedded %d" % (len(html), len(body)))


if __name__ == "__main__":
    main()
