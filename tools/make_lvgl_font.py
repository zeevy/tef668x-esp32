"""Turn Roboto Condensed into the LVGL fonts the screens use.

The type scale has eight sizes, each subset to the glyphs that size actually
shows. Subsetting is not a nicety. The 52px frequency face needs eleven glyphs,
ten digits and a point, and the whole face at that size is about 180 KB against
under 20 KB subset.

Roboto Condensed is under the SIL Open Font License 1.1 and Material Symbols
under the Apache License 2.0, which lets the converted bitmaps ship in this
GPLv3 repository. The font files themselves are not kept here, because they
are large, nobody edits them and they are one download away.

    python3 tools/make_lvgl_font.py --font "RobotoCondensed[wght].ttf" \
        --icons "MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf"

Get the two faces from Google:

    https://github.com/google/fonts/raw/main/ofl/robotocondensed/RobotoCondensed%5Bwght%5D.ttf
    https://github.com/google/material-design-icons/raw/master/variablefont/MaterialSymbolsOutlined%5BFILL,GRAD,opsz,wght%5D.ttf

It is a variable font, so this instantiates a static instance at each weight
first with fontTools and then runs `lv_font_conv` over that. Handing the
variable font straight to the converter gives whatever the default instance
is, which is Regular, and every weight in the scale would come out the same.

Needs `fonttools` and Node:

    python3 -m pip install --break-system-packages fonttools==4.65.0
    npx lv_font_conv@1.5.3 --version

Rerun it when the scale changes, run clang-format over the result, and commit
the headers. The format gate covers generated files like any other.
"""
import argparse
import functools
import os
import subprocess
import sys
import tempfile

# Pinned to an exact version, so two regenerations produce the same bytes
# rather than whatever the registry serves that day.
LV_FONT_CONV = "lv_font_conv@1.5.3"

# Four bits of antialiasing. One bit makes small labels look ragged; eight
# doubles the size for a difference nobody can see on a panel this dim.
BPP = 4

# Everything a screen can print. Latin only, because the panel shows English
# only.
ASCII = "0x20-0x7E"

# Glyphs past ASCII, each only in the faces that draw it.
# The micro sign is the "u" of "dBuV" written as the unit is written. The
# ellipsis ends a name or a label cut short in one glyph where three dots
# took three. The middle dot parts two short values, as in "FM \u00b7 kHz". The times
# sign counts a DX catch, "\u00d73", where "x3" would read as a word.
MICRO = "0xB5"
MIDDOT = "0xB7"
ELLIPSIS = "0x2026"
TIMES = "0xD7"

# The status symbols, out of Material Symbols Outlined.
#
# Named here rather than left as bare hex, because six codepoints in a row say
# nothing about what the header will show. The set is small on purpose: an
# icon is used only when it is quicker to read than the word, and `RDS` and
# `AF` are acronyms with no picture, so those stay as text.
ICON_CODEPOINTS = (
    ("wifi", 0xE63E, "joined a network, and the outline drawn under every"
                     " bar count so the symbol never loses its shape as the"
                     " signal drops"),
    ("wifi_2_bar", 0xE4D9, "joined, two of three bars"),
    ("wifi_1_bar", 0xE4CA, "joined, one of three bars"),
    ("wifi_find", 0xEB31, "trying to join one"),
    ("wifi_tethering", 0xE1E2, "serving its own access point"),
    ("check", 0xE668, "a self test that passed, and a saved menu choice"),
    ("close", 0xE5CD, "a self test that failed"),
    ("wifi_off", 0xE648, "not joined, and not trying"),
    ("chevron_right", 0xE5CC, "a menu group row, which opens another list,"
                              " and the Scope page's cursor button to the"
                              " right"),
    ("chevron_left", 0xE5CB, "the Scope page's cursor button to the left"),
    ("backspace", 0xE14A, "the frequency keypad's backspace key"),
    ("check_circle", 0xE86C, "a PI confirmed on this channel, on the DX page"),
    ("help", 0xE887, "a PI heard with a digit in doubt, or a country not"
                     " yet sure, on the DX page"),
    ("schedule", 0xE8B5, "a PI heard once and waiting for a second hearing"),
    ("block", 0xE14B, "a station that sends 0000, which is no PI"),
    ("new_releases", 0xE031, "a PI caught for the first time, on the DX"
                             " Catches page, and the update offer's title."
                             " Name checked in the font"),
    ("inbox", 0xE156, "the Catches page with nothing caught yet"),
    ("play_arrow", 0xE037, "the DX scanner running, and the Scope page's"
                           " Sweep button"),
    ("pause", 0xE034, "the DX scanner stopped"),
    ("traffic", 0xE565, "TP, a station that carries traffic news, on the"
                        " RDS Station page"),
    ("campaign", 0xEF49, "TA, a traffic announcement on air, on the RDS"
                         " Station and Networks pages"),
    ("music_note", 0xE405, "the station says it is playing music"),
    ("record_voice_over", 0xE91F, "the station says it is speech"),
    ("graphic_eq", 0xE1B8, "the station's decoder bits, DI, stereo or mono"),
)
ICONS = ",".join("0x%04X" % c for _, c, _ in ICON_CODEPOINTS)

# Cut from the filled style into the same face: the outline of a person in bed
# at 16px is a thin box that does not read as one, so it is drawn solid.
ICON_FILLED_CODEPOINTS = (
    ("hotel", 0xE53A, "auto off is on, and in the main colour the radio sleeps"
                      " within five minutes, in the header"),
)
ICONS_FILLED = ",".join("0x%04X" % c for _, c, _ in ICON_FILLED_CODEPOINTS)

# Ten digits, the point, the plus, the minus, the space and the colon. What
# a frequency and a setting value are made of, and nothing else. The colon
# is not decoration: a UTC offset such as -05:00 is a setting value, which
# is this same 52px face, and a table without a colon glyph draws nothing
# where it belongs. The plus is there for the same reason, for an offset
# east of Greenwich such as +05:30.
NUMERIC = "0x20,0x2B,0x2D-0x2E,0x30-0x3A"

# The copyright and licence line written into each generated file, per face.
NOTICES = {
    "Roboto Condensed": "Roboto Condensed: Copyright 2011 The Roboto Project "
                        "Authors, SIL Open Font License 1.1.",
    "Material Symbols Outlined": "Material Symbols: Copyright Google LLC, "
                                 "Apache License 2.0.",
}

# name, pixels, weight, glyphs, source, and what it is for.
# The sizes and weights are the screens' type scale.
FONTS = (
    ("freq", 52, 600, NUMERIC, "roboto", "the frequency, and a full screen setting value"),
    ("name", 26, 600, ASCII, "roboto", "the station name on the RDS station page, and a setting value that is a word"),
    ("value", 24, 500, ASCII, "roboto", "a panel value, such as the signal level or the PI"),
    ("menu", 21, 500, ASCII + "," + ELLIPSIS, "roboto", "the station name on the radio screen, and other short panel text"),
    ("title", 20, 600, ASCII, "roboto", "a screen title, and the boot screen"),
    # Medium, for the same reason as the label face below.
    ("text", 17, 500, ASCII + "," + ELLIPSIS, "roboto",
     "radio text, the unit, and a menu row"),
    ("small", 15, 500, ASCII + "," + ELLIPSIS, "roboto",
     "the clock, the date, tile and menu values"),
    # Medium, like every other face on the panel. Measured on the real font
    # file, Regular at 13px puts a 1.17px stem on the screen, and a stem near
    # one pixel is carried almost entirely by anti aliasing, which is what
    # makes small text wash out on a dim TFT. This face is every label, unit
    # and cell on the panel, so it is the one place where that matters most.
    ("label", 13, 500,
     ASCII + "," + MICRO + "," + MIDDOT + "," + ELLIPSIS + "," + TIMES,
     "roboto",
     "a label, a unit, a hint or a scale marking"),
    # 16, and placed by their middle rather than their baseline. A Material
    # Symbol fills its em box while a capital fills about three quarters of
    # one, so an icon set to the same size as the words beside it is visibly
    # taller than they are, and 18 was two clear pixels over the capitals.
    # 16 puts the ink between the height of the capitals and the height of
    # the drawn battery beside them, which is the compromise the two of them
    # need: matched to the words it was smaller than the battery, and matched
    # to the battery it was taller than the words.
    ("icons", 16, 400, ICONS, "icons", "the status and page symbols",
     ("icons-filled", ICONS_FILLED)),
)

OUT_DIR = os.path.join("src", "ui", "fonts")


def instantiate(source, weight, into, filled=False):
    """One static instance of the variable font at a single weight, and with
    `filled` the solid style of Material Symbols rather than the outline."""
    from fontTools import ttLib
    from fontTools.varLib import instancer

    font = ttLib.TTFont(source)
    if "fvar" not in font:
        raise SystemExit(
            "%s is not a variable font, so a weight cannot be picked from it. "
            "Get the one named RobotoCondensed[wght].ttf." % source)
    axes = {a.axisTag for a in font["fvar"].axes}
    if "wght" not in axes:
        raise SystemExit("%s has no weight axis, only %s"
                         % (source, ", ".join(sorted(axes))))
    pins = {"wght": weight}
    if filled:
        pins["FILL"] = 1
    instancer.instantiateVariableFont(font, pins, inplace=True)
    font.save(into)
    return into


def tidy(path, weight, glyphs, faces, purpose):
    """Make the generated file say something true and the same every time.

    The converter writes its own command line into the header, temporary
    directory and all, so two runs of this tool produce files that differ in a
    line nobody reads. That would show up as a change in every diff that
    touched a font and hide the ones that matter.
    """
    lines = open(path).read().splitlines(True)
    head = []
    for line in lines:
        if line.startswith(" * Opts:"):
            continue
        head.append(line)
        if line.startswith(" * Bpp:"):
            head.append(" * Weight: %d\n" % weight)
            head.append(" * Glyphs: %s\n" % glyphs)
            head.append(" *\n")
            head.append(" * %s, for %s.\n" % (" and ".join(faces), purpose))
            head.append(" * Generated by tools/make_lvgl_font.py. Do not edit.\n")
            head.append(" *\n")
            for face in faces:
                head.append(" * %s\n" % NOTICES[face])
    open(path, "w").write("".join(head))


def convert(source, name, pixels, glyphs, extra=None):
    """One LVGL font. `extra` is a second (source, glyphs) cut into the same
    face at the same size, which is how the hint symbols ride in the label
    font: the converter takes any number of --font/--range pairs."""
    out = os.path.join(OUT_DIR, "font_%s.c" % name)
    fonts = ["--font", source, "--range", glyphs]
    if extra:
        fonts += ["--font", extra[0], "--range", extra[1]]
    subprocess.run(
        ["npx", "--yes", LV_FONT_CONV] + fonts + [
         "--size", str(pixels),
         "--bpp", str(BPP),
         "--format", "lvgl",
         "--lv-include", "lvgl.h",
         "--force-fast-kern-format",
         # Uncompressed. lv_font_conv compresses 4 bpp glyphs by default, and
         # LVGL only decodes that when LV_USE_FONT_COMPRESSED is on, which
         # costs code and a decode of every glyph on every redraw. Without it
         # nothing at all is drawn: the rectangles appear and the text does
         # not, with no error anywhere. Compression would save about a third
         # of the glyph data, which is not worth a decode on every redraw.
         "--no-compress",
         "--lv-font-name", "roboto_%s" % name,
         "-o", out],
        check=True)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--font", required=True,
                    help="RobotoCondensed[wght].ttf")
    ap.add_argument("--icons", required=True,
                    help="MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf")
    ap.add_argument("--only", help="one font name, for a quick rebuild")
    args = ap.parse_args()

    os.makedirs(OUT_DIR, exist_ok=True)
    made = []
    with tempfile.TemporaryDirectory() as tmp:
        sources = {"roboto": args.font, "icons": args.icons,
                   "icons-filled": args.icons}
        faces = {"roboto": "Roboto Condensed",
                 "icons": "Material Symbols Outlined",
                 "icons-filled": "Material Symbols Outlined"}
        @functools.cache
        def cut_font(source, weight):
            return instantiate(
                sources[source], weight,
                os.path.join(tmp, "%s-%d.ttf" % (source, weight)),
                filled=source == "icons-filled")

        for row in FONTS:
            name, pixels, weight, glyphs, source, purpose = row[:6]
            extra = row[6] if len(row) > 6 else None
            if args.only and name != args.only:
                continue
            if extra:
                extra = (cut_font(extra[0], weight), extra[1])
                glyphs = glyphs + "," + extra[1]
            out = convert(cut_font(source, weight), name, pixels, row[3], extra)
            used = [faces[source]]
            if row[6:] and faces[row[6][0]] not in used:
                used.append(faces[row[6][0]])
            tidy(out, weight, glyphs, used, purpose)
            made.append((out, os.path.getsize(out)))
            print("%-6s %3dpx w%d -> %s" % (name, pixels, weight, out))

    total = sum(size for _, size in made)
    print("%d fonts, %.1f KB of C source" % (len(made), total / 1024.0))


if __name__ == "__main__":
    sys.exit(main())
