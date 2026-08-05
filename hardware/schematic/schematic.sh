#!/usr/bin/env sh
#
# schematic.sh - generate the breadboard and schematic PNGs.
#
#   ./schematic.sh              regenerate SVG sources, then rasterise to PNG
#   ./schematic.sh --no-svg     rasterise the checked-in SVGs only
#   ./schematic.sh --dpi 300    override the output resolution (default 144)
#
# Rasteriser search order (first one found wins):
#   Fritzing  - only when a .fzz sketch is present; exports the canonical
#               breadboard and schematic views the way upstream kits do
#   rsvg-convert / Inkscape / ImageMagick / Chromium - SVG fallbacks
#
# POSIX sh: runs under Git Bash, WSL, macOS and Linux without modification.

set -eu

DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
DPI=144
GEN_SVG=1

while [ $# -gt 0 ]; do
    case "$1" in
        --no-svg) GEN_SVG=0 ;;
        --dpi)    DPI="${2:?--dpi needs a value}"; shift ;;
        -h|--help) sed -n '2,17p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "schematic.sh: unknown option '$1'" >&2; exit 2 ;;
    esac
    shift
done

BREADBOARD="$DIR/smart-gateway-breadboard"
SCHEMATIC="$DIR/smart-gateway-schematic"

have() { command -v "$1" >/dev/null 2>&1; }

# ---------------------------------------------------------------------------
# 1. Regenerate the SVG sources from the pin assignment.
# ---------------------------------------------------------------------------
if [ "$GEN_SVG" -eq 1 ]; then
    if have python3;  then PY=python3
    elif have python; then PY=python
    else PY=""
    fi

    if [ -n "$PY" ]; then
        echo "==> regenerating SVG sources ($PY)"
        "$PY" "$DIR/svggen.py"
    else
        echo "==> no Python interpreter; using the checked-in SVG sources"
    fi
fi

# ---------------------------------------------------------------------------
# 2. Pick a rasteriser.
# ---------------------------------------------------------------------------
FRITZING=""
for c in fritzing Fritzing \
         "/Applications/Fritzing.app/Contents/MacOS/Fritzing" \
         "/c/Program Files/Fritzing/Fritzing.exe"; do
    if have "$c"; then FRITZING="$c"; break; fi
    if [ -x "$c" ];  then FRITZING="$c"; break; fi
done

FZZ=$(find "$DIR" -maxdepth 1 -name '*.fzz' 2>/dev/null | head -n 1)

# Chromium is the last resort: headless screenshot of the SVG.
CHROME=""
for c in chromium chromium-browser google-chrome chrome msedge \
         "/c/Program Files/Google/Chrome/Application/chrome.exe" \
         "/c/Program Files (x86)/Microsoft/Edge/Application/msedge.exe"; do
    if have "$c"; then CHROME="$c"; break; fi
    if [ -x "$c" ];  then CHROME="$c"; break; fi
done

render() {  # render <svg-in> <png-out>
    _in="$1"; _out="$2"
    if   have rsvg-convert; then
        rsvg-convert --dpi-x "$DPI" --dpi-y "$DPI" -b white -o "$_out" "$_in"
    elif have inkscape; then
        inkscape "$_in" --export-type=png --export-dpi="$DPI" \
                 --export-background=white --export-filename="$_out"
    elif have magick; then
        magick -density "$DPI" -background white "$_in" -flatten "$_out"
    elif have convert && convert -list format 2>/dev/null | grep -qi rsvg; then
        convert -density "$DPI" -background white "$_in" -flatten "$_out"
    elif [ -n "$CHROME" ]; then
        "$CHROME" --headless --disable-gpu --default-background-color=ffffffff \
                  --screenshot="$_out" --window-size=1560,1020 "file://$_in"
    else
        echo "schematic.sh: no rasteriser found." >&2
        echo "  install one of: librsvg (rsvg-convert), Inkscape, ImageMagick," >&2
        echo "  or Fritzing. The SVG sources in $DIR remain usable as-is." >&2
        return 1
    fi
    echo "    $(basename "$_out")"
}

# ---------------------------------------------------------------------------
# 3. Export.
# ---------------------------------------------------------------------------
if [ -n "$FRITZING" ] && [ -n "$FZZ" ]; then
    echo "==> exporting from Fritzing sketch: $(basename "$FZZ")"
    "$FRITZING" --export-image-breadboard "$BREADBOARD.png" "$FZZ" || true
    "$FRITZING" --export-image-schematic  "$SCHEMATIC.png"  "$FZZ" || true
fi

if [ ! -f "$BREADBOARD.png" ] || [ ! -f "$SCHEMATIC.png" ]; then
    echo "==> rasterising SVG at ${DPI} dpi"
    render "$BREADBOARD.svg" "$BREADBOARD.png"
    render "$SCHEMATIC.svg"  "$SCHEMATIC.png"
fi

echo "==> done"
ls -l "$BREADBOARD.png" "$SCHEMATIC.png" 2>/dev/null || true
