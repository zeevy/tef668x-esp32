"""Turn Roboto Condensed, Noto Sans SC and Material Symbols into the LVGL
fonts the screens use.

The type scale has six sizes of Roboto Condensed and one of symbols, each
subset to the glyphs that size actually shows. Subsetting is not a nicety.
The 52px frequency face needs eleven glyphs, ten digits and a point, and the
whole face at that size is about 180 KB against under 20 KB subset.

Every language's letters past ASCII come from two more faces per text size,
which the Roboto face names as its fallback. LVGL takes a character a face
does not carry from its fallback, and lays the line out by the first face
alone, so English draws the same with or without them. Merging them into the
Roboto face would not be: the converter takes the line height from every
glyph, and Chinese makes a 21px face 23 high where Roboto alone is 22.

- `ext_<face>`: Roboto Condensed at the same size and weight, holding every
  accented Latin, Cyrillic and Greek letter and every sign any language
  file uses, so an accent matches the letter it sits on.
- `zh_<size>`: Noto Sans SC, holding every Chinese character the Chinese
  texts use, at 13, 17, 20 and 24px. The 15px face takes its Chinese at
  13px. The 13px face is Bold, because at Medium small Chinese comes out thin
  and grey beside the Latin.

They are complete by construction: this reads src/lang/en.h and every other
language file there and puts every character of a script into every face of
that script. Each file records a checksum of its character set, and
`--check` works the sets out again from the language files and fails on any
difference. A character no face can hold stops the tool, naming the text.

The two kinds of extra face are stored packed, LVGL's own run length coding,
which saves about a quarter of their size. The Roboto faces are drawn all the
time and stay unpacked.

Roboto Condensed and Noto Sans SC are under the SIL Open Font License 1.1 and
Material Symbols under the Apache License 2.0, which lets the converted
bitmaps ship in this GPLv3 repository. The font files themselves are not kept
here, because they are large, nobody edits them and they are one download
away.

    python3 tools/make_lvgl_font.py --font "RobotoCondensed[wght].ttf" \
        --icons "MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf" \
        --cjk "NotoSansSC[wght].ttf"

Get them from Google. Roboto Condensed and Noto Sans SC are pinned to a
commit, and the tool stops when a file is not the one it was made from:

    https://github.com/google/fonts/raw/ae50998cbcab558ae1f047ad5c724ec7261980ca/ofl/robotocondensed/RobotoCondensed%5Bwght%5D.ttf
    https://github.com/google/fonts/raw/2894aab31764f10f29c421bdfd2340d3b382d384/ofl/notosanssc/NotoSansSC%5Bwght%5D.ttf
    https://github.com/google/material-design-icons/raw/master/variablefont/MaterialSymbolsOutlined%5BFILL,GRAD,opsz,wght%5D.ttf

They are variable fonts, so this instantiates a static instance at each
weight first with fontTools and then runs `lv_font_conv` over that. Handing
the variable font straight to the converter gives whatever the default
instance is, which is Regular for Roboto and Thin for Noto, and every weight
in the scale would come out the same. Noto Sans SC is cut down to the
characters first: instancing all 17 MB of it takes minutes.

Needs `fonttools` and Node:

    python3 -m pip install --break-system-packages fonttools==4.65.0
    npx lv_font_conv@1.5.3 --version

Rerun it when the scale or a language file changes, run clang-format over
the result, and commit the headers. The format gate covers generated files
like any other, and the fonts gate runs `--check`.
"""
import argparse
import functools
import glob
import hashlib
import os
import re
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
    ("remove", 0xE15B, "the value editor's minus button, with Touch On"),
    ("add", 0xE145, "the value editor's plus button, with Touch On"),
    ("density_small", 0xEBA8, "the menu symbol, four lines, at the top left of"
                              " the radio screen, with Touch On"),
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
    ("computer", 0xE30A, "a PC connected over the PC Link, in the radio"
     " screen's header"),
    ("graphic_eq", 0xE1B8, "the station's decoder bits, DI, stereo or mono"),
    ("waves", 0xE176, "the squelch tile on the radio screen"),
    ("height", 0xEA16, "the filter width tile on the radio screen"),
    ("volume_down", 0xE04D, "the volume tile on the radio screen"),
    ("no_sound", 0xE710, "the volume tile while the radio is muted"),
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
    "Noto Sans SC": "Noto Sans SC: Copyright 2014-2021 Adobe "
                    "(http://www.adobe.com/), with Reserved Font Name "
                    "'Source', SIL Open Font License 1.1.",
}

# The font files these faces are made from, by sha256, so a file that moved
# on upstream cannot change the faces without anyone seeing it.
SHA256 = {
    "roboto": "dace262afcee68a5276f200d8026c57221735c0118ab5fda8c2c0d3dc409a8d0",
    "cjk": "a3041811a78c361b1de50f953c805e0244951c21c5bd412f7232ef0d899af0da",
}

# The extra letters' faces, one per text face, at its size and weight, and
# the Chinese face each one falls back to in turn.
EXT_FACES = (
    ("label", 13, 500, "zh_13"),
    ("small", 15, 500, "zh_13"),
    ("text", 17, 500, "zh_17"),
    ("title", 20, 600, "zh_20"),
    ("value", 24, 600, "zh_24"),
)

# The Chinese faces: size and weight. Bold at 13px, where Medium Chinese
# looks thin and grey beside the Latin; the others at their text face's
# weight. 15px text takes its Chinese at 13px, which saves a whole face.
ZH_FACES = (
    ("13", 13, 700),
    ("17", 17, 500),
    ("20", 20, 600),
    ("24", 24, 600),
)

LANG_DIR = os.path.join("src", "lang")

# Where Chinese starts, U+2E80, the CJK radicals, which takes in the full
# width punctuation too. The private use area is where the icon symbols are,
# and a text that holds one draws it in the icon face.
CJK_FROM = 0x2E80
PRIVATE_USE = range(0xE000, 0xF900)

# name, pixels, weight, glyphs, source, and what it is for.
# The sizes and weights are the screens' type scale.
FONTS = (
    ("freq", 52, 600, NUMERIC, "roboto", "the frequency, and a full screen setting value"),
    ("value", 24, 600, ASCII, "roboto", "a panel value, such as the signal level or the PI, the station name on the RDS station page, and a setting value that is a word"),
    ("title", 20, 600, ASCII + "," + MICRO + "," + ELLIPSIS, "roboto", "a screen title, the station name on the radio screen, the boot screen and other short panel text"),
    # Medium, for the same reason as the label face below.
    ("text", 17, 500, ASCII + "," + ELLIPSIS, "roboto",
     "radio text, the unit, and a menu row"),
    ("small", 15, 500, ASCII + "," + MICRO + "," + ELLIPSIS, "roboto",
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


def instantiate_subset(source, weight, letters, into):
    """A static instance of Noto Sans SC at one weight holding only
    `letters`. Cut down first, since instancing the whole 17 MB file takes
    minutes."""
    from fontTools import subset, ttLib
    from fontTools.varLib import instancer

    font = ttLib.TTFont(source)
    if "fvar" not in font:
        raise SystemExit("%s is not a variable font. Get the one named "
                         "NotoSansSC[wght].ttf." % source)
    cutter = subset.Subsetter(subset.Options())
    cutter.populate(unicodes=sorted(letters))
    cutter.subset(font)
    instancer.instantiateVariableFont(font, {"wght": weight}, inplace=True)
    font.save(into)
    return into


def check_sha(path, which):
    digest = hashlib.sha256(open(path, "rb").read()).hexdigest()
    if digest != SHA256[which]:
        raise SystemExit("%s is not the file these fonts are made from: sha256 "
                         "%s, wanted %s. Get it from the URL above." %
                         (path, digest, SHA256[which]))


def ranges(letters):
    """The converter's --range for a set of code points, runs joined."""
    out = []
    for code in sorted(letters):
        if out and code == out[-1][1] + 1:
            out[-1][1] = code
        else:
            out.append([code, code])
    return ",".join("0x%04X" % a if a == b else "0x%04X-0x%04X" % (a, b)
                    for a, b in out)


LITERAL = re.compile(r'"((?:[^"\\]|\\.)*)"')


def decode(literals):
    """The text C makes of string literals, escapes read. A \\x escape takes
    every hex digit after it, as in C."""
    data = bytearray()
    for lit in literals:
        raw = lit.encode("utf-8")
        k = 0
        while k < len(raw):
            if raw[k:k + 2] == b"\\x":
                m = re.match(rb"\\x([0-9A-Fa-f]+)", raw[k:])
                data.append(int(m.group(1), 16) & 0xFF)
                k += len(m.group(0))
            elif raw[k:k + 1] == b"\\":
                data += {b"n": b"\n", b"\\": b"\\", b'"': b'"'}[raw[k + 1:k + 2]]
                k += 2
            else:
                data.append(raw[k])
                k += 1
    return bytes(data).decode("utf-8")


def texts(path):
    """(id, text) for every line of a language file. A text is the first run
    of literals on its line; en.h has a second, the note on where it is
    used, after a comma."""
    out = []
    for line in open(path, encoding="utf-8"):
        m = re.match(r"\s*X\((\w+),", line)
        if not m:
            continue
        run = []
        last = m.end()
        for lit in LITERAL.finditer(line, m.end()):
            if run and "," in line[last:lit.start()]:
                break
            run.append(lit.group(1))
            last = lit.end()
        out.append((m.group(1), decode(run)))
    return out


def letters():
    """Every character past ASCII the language files hold, split into the
    extra letters and the Chinese, and where each was first seen."""
    ext, zh, where = set(), set(), {}
    for path in sorted(glob.glob(os.path.join(LANG_DIR, "*.h"))):
        for ident, text in texts(path):
            for ch in text:
                code = ord(ch)
                if code <= 0x7E or code in PRIVATE_USE:
                    continue
                (zh if code >= CJK_FROM else ext).add(code)
                where.setdefault(code, "%s %s" % (os.path.basename(path), ident))
    return ext, zh, where


def letters_line(codes):
    """The checksum a face records of its character set, so the fonts gate
    can tell a face cut before a language file changed."""
    digest = hashlib.sha256(",".join("%04X" % c for c in sorted(codes))
                            .encode()).hexdigest()[:16]
    return "%d characters, sha256 %s" % (len(codes), digest)


def tidy(path, weight, glyphs, faces, purpose, letters=None):
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
            if letters:
                head.append(" * Letters: %s\n" % letters)
            head.append(" *\n")
            head.append(" * %s, for %s.\n" % (" and ".join(faces), purpose))
            head.append(" * Generated by tools/make_lvgl_font.py. Do not edit.\n")
            head.append(" *\n")
            for face in faces:
                head.append(" * %s\n" % NOTICES[face])
    open(path, "w").write("".join(head))


def convert(source, name, pixels, glyphs, extra=None, fallback=None,
            packed=False):
    """One LVGL font, `name` in font_<name>.c, or roboto_<name> for a face of
    the scale. `extra` is a second (source, glyphs) cut into the same face at
    the same size, which is how the filled symbol rides in the icon font: the
    converter takes any number of --font/--range pairs. `fallback` is the
    font LVGL takes a character from when this one does not carry it."""
    font_name = name if name.startswith(("ext_", "zh_")) else "roboto_" + name
    out = os.path.join(OUT_DIR, "font_%s.c" % name)
    fonts = ["--font", source, "--range", glyphs]
    if extra:
        fonts += ["--font", extra[0], "--range", extra[1]]
    if fallback:
        fonts += ["--lv-fallback", fallback]
    # Unpacked unless asked. lv_font_conv packs 4 bpp glyphs by default, and
    # each packed glyph is unpacked on every draw, which the Roboto faces,
    # drawn all the time, are spared. LVGL only draws a packed face when
    # LV_USE_FONT_COMPRESSED is on, and the fonts gate checks the two agree.
    subprocess.run(
        ["npx", "--yes", LV_FONT_CONV] + fonts + [
         "--size", str(pixels),
         "--bpp", str(BPP),
         "--format", "lvgl",
         "--lv-include", "lvgl.h",
         "--force-fast-kern-format"] +
        ([] if packed else ["--no-compress"]) + [
         "--lv-font-name", font_name,
         "-o", out],
        check=True)
    return out


def check():
    """Fail when a face's characters are not what the language files need
    now, or a face does not name the fallback it should."""
    ext, zh, _ = letters()
    bad = []
    faces = [("ext_" + n, ext, fb) for n, _, _, fb in EXT_FACES]
    faces += [("zh_" + n, zh, None) for n, _, _ in ZH_FACES]
    for name, codes, fallback in faces:
        path = os.path.join(OUT_DIR, "font_%s.c" % name)
        if not os.path.exists(path):
            bad.append("%s is missing" % path)
            continue
        text = open(path).read()
        if " * Letters: %s\n" % letters_line(codes) not in text:
            bad.append("%s holds other characters than the language files "
                       "use" % path)
        if fallback and ".fallback = &%s," % fallback not in text:
            bad.append("%s does not fall back to %s" % (path, fallback))
    for name, _, _, _ in EXT_FACES:
        path = os.path.join(OUT_DIR, "font_%s.c" % name)
        if ".fallback = &ext_%s," % name not in open(path).read():
            bad.append("%s does not fall back to ext_%s" % (path, name))
    for line in bad:
        print("   " + line)
    if bad:
        print("   run tools/make_lvgl_font.py, then clang-format over "
              "src/ui/fonts/")
    return 1 if bad else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--font", help="RobotoCondensed[wght].ttf")
    ap.add_argument("--icons",
                    help="MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf")
    ap.add_argument("--cjk", help="NotoSansSC[wght].ttf")
    ap.add_argument("--only", help="font names, such as title,ext_title,zh_20, "
                                   "for a quick rebuild")
    ap.add_argument("--check", action="store_true",
                    help="check the faces against the language files, and "
                         "make nothing")
    args = ap.parse_args()
    if args.check:
        return check()

    ext, zh, where = letters()
    only = set(args.only.split(",")) if args.only else None
    wanted = lambda name: only is None or name in only  # noqa: E731
    need = {
        "roboto": any(wanted(r[0]) for r in FONTS if r[4] == "roboto") or
        any(wanted("ext_" + f[0]) for f in EXT_FACES),
        "icons": any(wanted(r[0]) for r in FONTS if r[4] == "icons"),
        "cjk": any(wanted("zh_" + f[0]) for f in ZH_FACES),
    }
    sources = {"roboto": args.font, "icons": args.icons,
               "icons-filled": args.icons, "cjk": args.cjk}
    for which, needed in need.items():
        if needed and not sources[which]:
            raise SystemExit("needs --%s" % {"roboto": "font", "icons": "icons",
                                             "cjk": "cjk"}[which])
        if needed and which in SHA256:
            check_sha(sources[which], which)

    os.makedirs(OUT_DIR, exist_ok=True)
    made = []
    with tempfile.TemporaryDirectory() as tmp:
        faces = {"roboto": "Roboto Condensed",
                 "icons": "Material Symbols Outlined",
                 "icons-filled": "Material Symbols Outlined"}
        @functools.cache
        def cut_font(source, weight):
            return instantiate(
                sources[source], weight,
                os.path.join(tmp, "%s-%d.ttf" % (source, weight)),
                filled=source == "icons-filled")

        @functools.cache
        def cut_cjk(weight):
            return instantiate_subset(sources["cjk"], weight, zh,
                                      os.path.join(tmp, "cjk-%d.ttf" % weight))

        for which, codes, family in (("roboto", ext, "Roboto Condensed"),
                                     ("cjk", zh, "Noto Sans SC")):
            if not need[which]:
                continue
            from fontTools import ttLib
            carried = set(ttLib.TTFont(sources[which]).getBestCmap())
            for code in sorted(codes - carried):
                raise SystemExit("U+%04X, in %s, is in no face: %s does not "
                                 "carry it" % (code, where[code], family))

        fallbacks = {n: "ext_" + n for n, _, _, _ in EXT_FACES}
        for row in FONTS:
            name, pixels, weight, glyphs, source, purpose = row[:6]
            extra = row[6] if len(row) > 6 else None
            if not wanted(name):
                continue
            if extra:
                extra = (cut_font(extra[0], weight), extra[1])
                glyphs = glyphs + "," + extra[1]
            out = convert(cut_font(source, weight), name, pixels, row[3], extra,
                          fallback=fallbacks.get(name))
            used = [faces[source]]
            if row[6:] and faces[row[6][0]] not in used:
                used.append(faces[row[6][0]])
            tidy(out, weight, glyphs, used, purpose)
            made.append((out, os.path.getsize(out)))
            print("%-8s %3dpx w%d -> %s" % (name, pixels, weight, out))

        for name, pixels, weight, fallback in EXT_FACES:
            if not wanted("ext_" + name):
                continue
            out = convert(cut_font("roboto", weight), "ext_" + name, pixels,
                          ranges(ext), fallback=fallback, packed=True)
            tidy(out, weight, "every letter past ASCII the language files use",
                 ["Roboto Condensed"],
                 "the letters the %s face does not carry" % name,
                 letters_line(ext))
            made.append((out, os.path.getsize(out)))
            print("%-8s %3dpx w%d -> %s" % ("ext_" + name, pixels, weight, out))

        for name, pixels, weight in ZH_FACES:
            if not wanted("zh_" + name):
                continue
            out = convert(cut_cjk(weight), "zh_" + name, pixels, ranges(zh),
                          packed=True)
            tidy(out, weight, "every Chinese character the language files use",
                 ["Noto Sans SC"],
                 "Chinese at %dpx" % pixels, letters_line(zh))
            made.append((out, os.path.getsize(out)))
            print("%-8s %3dpx w%d -> %s" % ("zh_" + name, pixels, weight, out))

    total = sum(size for _, size in made)
    print("%d fonts, %.1f KB of C source" % (len(made), total / 1024.0))
    return 0


if __name__ == "__main__":
    sys.exit(main())
