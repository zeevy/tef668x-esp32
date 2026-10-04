#!/usr/bin/env bash
#
# Draw the real screens on this machine and write them out as images.
#
#   tools/screenshot.sh          into .pio/shots/
#   tools/screenshot.sh <dir>    somewhere else
#
# What this is for is in the comment at the top of tools/screenshot.cpp. In
# short: `ui/` knows about LVGL and nothing else, so the same screen.cpp that
# runs on the radio links against LVGL's software renderer here and draws into
# memory. It checks layout without anybody standing at the radio.
#
# Not part of tools/check.sh. It produces pictures, and a picture has to be
# looked at to mean anything. It mainly fails on a character drawn in a face
# that does not carry it, because that draws as nothing and is easy to miss by
# eye. Every picture is still written, then the script exits 1 and names the
# pictures and the characters. Run `pio run -e ats125` once first, so LVGL is
# downloaded.
set -euo pipefail

cd "$(dirname "$0")/.."
LVGL=.pio/libdeps/ats125/lvgl
OUT="${1:-.pio/shots}"
BIN=.pio/screenshot
LIB=.pio/lvgl-native.a
OBJ=.pio/lvgl-native

if [ ! -d "$LVGL" ]; then
  echo "LVGL is not unpacked yet. Run: pio run -e ats125" >&2
  exit 1
fi

mkdir -p "$OUT" "$OBJ"

# The radio's own lv_conf.h with one line changed.
#
# Everything that decides what the screens look like is kept: the fonts, the
# widgets, the colour depth, the default font. Only LV_MEM_SIZE is raised,
# because this machine is 64 bit and every pointer inside an LVGL object is
# twice the width it is on the ESP32, so the same screens do not fit in the
# same pool. With the radio's 24 KB the pool is down to 776 bytes free before
# anything is drawn and the first label fails to allocate.
#
# The consequence, and it matters: **the memory numbers from this tool say
# nothing about the radio.** Only the layout does. The radio's real figure is
# `pnl.lvp` in GET /api/state.
#
# The file is replaced only when its contents change, so its date says when
# lv_conf.h or LVGL last changed and the rebuild check below can trust it.
# LVGL's version is written into it because LVGL's own files keep the dates
# they had in the release archive, older than any build here, so an upgrade
# would not show up by date alone.
NATIVE_CONF=.pio/lv_conf_native.h
LVGL_VERSION=$(sed -n 's/.*"version": *"\([^"]*\)".*/\1/p' "$LVGL/library.json" | head -1)
: "${LVGL_VERSION:?no version found in $LVGL/library.json}"
{
  echo "/* LVGL $LVGL_VERSION */"
  sed 's|#define LV_MEM_SIZE .*|#define LV_MEM_SIZE (2048 * 1024U)|' \
    src/ui/lv_conf.h
} > "$NATIVE_CONF.new"
if cmp -s "$NATIVE_CONF.new" "$NATIVE_CONF"; then
  rm "$NATIVE_CONF.new"
else
  mv "$NATIVE_CONF.new" "$NATIVE_CONF"
fi

CONF=-DLV_CONF_PATH='"'"$PWD/$NATIVE_CONF"'"'
INC="-I $LVGL -I $LVGL/src -I src -I src/ui"
# The board flag the firmware builds with, so the harness reads the same board
# header the radio does rather than repeating what it says.
INC="$INC -DBOARD_ATS125=1"

# LVGL is C and will not compile as C++, so it is built on its own and kept.
# It is about 460 files and takes a few seconds, and it only changes when
# the library or lv_conf.h does, which is what the timestamp check is for.
if [ ! -f "$LIB" ] || [ "$NATIVE_CONF" -nt "$LIB" ]; then
  echo "building LVGL for this machine, once"
  rm -rf "$OBJ" && mkdir -p "$OBJ"
  n=0
  while IFS= read -r f; do
    cc -O1 -w -c $CONF $INC "$f" -o "$OBJ/$(printf '%04d' $n).o"
    n=$((n + 1))
  done < <(find "$LVGL/src" -name '*.c')
  ar rcs "$LIB" "$OBJ"/*.o
fi

# The fonts are generated C and the theme is C, so they are compiled as C. The
# screens are C++, and so is this harness, because screen.h has no extern "C"
# around it: nothing but C++ has ever needed it.
UI=.pio/ui-native
rm -rf "$UI" && mkdir -p "$UI"
n=0
# All of core/ is here. The panels draw from core/meter.c and core/scale.c, and
# the scenes built from captures run src/screen_state.cpp, the radio's own
# state builder, which uses most of the rest. A panel may use core, so this is
# the renderer needing the same files the firmware links, not a layer being
# crossed.
for f in src/ui/theme.c src/core/*.c src/ui/fonts/*.c; do
  cc -O1 -w -c $CONF $INC "$f" -o "$UI/$(printf '%03d' $n).o"
  n=$((n + 1))
done

c++ -O1 -w -std=c++17 $CONF $INC \
  -o "$BIN" \
  tools/screenshot.cpp src/screen_state.cpp src/screen_dx_state.cpp \
  src/screen_bw_state.cpp src/screen_rds_state.cpp \
  src/ui/screen.cpp src/ui/screen_boot.cpp \
  src/ui/screen_menu.cpp src/ui/screen_rds.cpp src/ui/screen_recovery.cpp \
  src/ui/screen_dx.cpp src/ui/screen_dx_catches.cpp src/ui/screen_dx_scan.cpp src/ui/screen_dx_scope.cpp src/ui/screen_bw.cpp \
  src/ui/draw.cpp \
  src/ui/layout_320x240.cpp src/ui/panels/*.cpp \
  "$UI"/*.o "$LIB" -lm

# Only this run's screens are left for make_assets.py to convert, so a
# screen the tool no longer draws cannot come back as a stale picture.
rm -f "$OUT"/*.bmp
echo "drawing into $OUT"
"$BIN" "$OUT"
