#!/usr/bin/env bash
# Requires ImageMagick 7 and DejaVu Sans Bold. Run with bash from any directory.
set -euo pipefail

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
logo="$root/src/qt/res/images/splash.png"
output="${1:-$root/share/pixmaps}"

# NSIS Modern UI uses 24-bit Windows BMPs at these exact dimensions.
magick -size 150x57 xc:white \
    \( "$logo" -trim +repage -resize 34x34 \) -geometry +6+11 -composite \
    -font DejaVu-Sans-Bold -pointsize 15 -fill '#4b3426' -annotate +47+34 'AGRARIAN' \
    -alpha off -type TrueColor "BMP3:$output/nsis-header.bmp"

magick -size 164x314 'xc:#f4ebdd' \
    \( "$logo" -trim +repage -resize 124x124 \) -geometry +20+76 -composite \
    -font DejaVu-Sans-Bold -pointsize 17 -fill '#4b3426' \
    -gravity north -annotate +0+218 'AGRARIAN' \
    -alpha off -type TrueColor "BMP3:$output/nsis-wizard.bmp"

magick "$logo" -trim +repage -background none \
    -define icon:auto-resize=16,32,48,96 "$output/agrarian.ico"
