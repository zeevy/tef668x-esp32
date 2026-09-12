#!/usr/bin/env python3
"""Turn a TrueType font into a C bitmap font for the ILI9341.

The panel driver has no font engine and does not want one. LVGL brings its own
in phase 4, so the text this phase draws only has to be readable and cheap.

Roboto Condensed is the face the design settled on, and it is Apache 2.0, so
rendered bitmaps can ship in this GPLv3 repository. The font file itself is not
kept here. Point this at a copy:

    https://github.com/googlefonts/roboto-2/releases

    python3 tools/make_font.py --font RobotoCondensed-VF.ttf \\
        --size 16 --name Small --chars ascii \\
        --out src/ui/font_small.h

    python3 tools/make_font.py --font RobotoCondensed-VF.ttf \\
        --size 44 --weight 500 --name Large --chars digits \\
        --out src/ui/font_large.h

Each glyph is one bit per pixel, rows padded to whole bytes, so a row can be
read a byte at a time and pushed straight out. Glyphs keep their real widths
rather than being squared off, because a proportional frequency reads better
and costs nothing to draw.
"""

import argparse
import subprocess
import sys

from PIL import Image, ImageDraw, ImageFont

ASCII = [chr(c) for c in range(32, 127)]
DIGITS = list("0123456789. ")


def render(font, ch, height, baseline):
    """Draw one character and return its width and its rows of bits.

    The sampled window runs from the pen origin to the advance width. A face
    whose ink hangs outside that, such as a negative left side bearing, would
    lose those columns. Checked for both fonts this project generates and no
    glyph does, but it is worth knowing before pointing this at another face.
    """
    # Wide scratch image, then cropped to the ink. Measuring first and drawing
    # second gets the bearing wrong on some faces, so draw and look.
    pad = height * 2
    img = Image.new("L", (pad * 2, height), 0)
    draw = ImageDraw.Draw(img)
    draw.text((pad // 2, baseline), ch, font=font, fill=255, anchor="ls")

    px = img.load()
    width = int(round(font.getlength(ch)))
    left = pad // 2

    rows = []
    for y in range(height):
        bits = []
        for x in range(left, left + width):
            bits.append(1 if px[x, y] >= 128 else 0)
        rows.append(bits)
    return width, rows


def pack(rows):
    """One bit per pixel, each row padded out to whole bytes."""
    out = []
    for bits in rows:
        byte = 0
        count = 0
        for b in bits:
            byte = (byte << 1) | b
            count += 1
            if count == 8:
                out.append(byte)
                byte = 0
                count = 0
        if count:
            out.append(byte << (8 - count))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--font", required=True)
    ap.add_argument("--size", type=int, required=True)
    ap.add_argument("--weight", type=int, default=400)
    ap.add_argument("--name", required=True)
    ap.add_argument("--chars", choices=["ascii", "digits"], default="ascii")
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    font = ImageFont.truetype(args.font, args.size)
    if args.weight != 400:
        # Loudly, not quietly. Swallowing this wrote a header that claimed a
        # weight the glyphs were never rendered at.
        try:
            font.set_variation_by_axes([args.weight])
        except OSError as err:
            print(
                f"{args.font} is not a variable font, so weight "
                f"{args.weight} cannot be applied: {err}",
                file=sys.stderr,
            )
            return 1

    ascent, descent = font.getmetrics()
    height = ascent + descent
    chars = ASCII if args.chars == "ascii" else DIGITS

    # Render everything first, then crop the blank rows off the top and bottom
    # of the whole font at once. Cropping each glyph to its own ink would put
    # every character on its own baseline, so the crop has to be the same for
    # all of them.
    drawn = [(ch,) + render(font, ch, height, ascent) for ch in chars]

    top = height
    bottom = 0
    for _, _, rows in drawn:
        for y, bits in enumerate(rows):
            if any(bits):
                top = min(top, y)
                bottom = max(bottom, y)
    if bottom < top:
        print("nothing was drawn at all", file=sys.stderr)
        return 1
    height = bottom - top + 1

    glyphs = []
    data = []
    for ch, width, rows in drawn:
        packed = pack(rows[top : bottom + 1])
        # FontGlyph.offset is a uint16_t. Past 65535 it would wrap silently and
        # every glyph after that would be read from the wrong place.
        if len(data) > 0xFFFF:
            print(
                f"the font is too big: glyph '{ch}' starts at byte "
                f"{len(data)}, past what a uint16_t offset can hold",
                file=sys.stderr,
            )
            return 1
        glyphs.append((ch, width, len(data)))
        data.extend(packed)

    lower = args.name.lower()
    guard = f"UI_FONT_{lower.upper()}_H"
    first = ord(chars[0])
    last = ord(chars[-1]) if args.chars == "ascii" else 0

    with open(args.out, "w") as f:
        f.write(f"""/**
 * @file font_{lower}.h
 * @brief Generated bitmap font, {args.size}px. Do not edit by hand.
 *
 * Made by tools/make_font.py from Roboto Condensed at weight {args.weight},
 * which is the face decision 21 settled on for the screen. Roboto is Apache
 * 2.0, so these bitmaps are fine in a GPLv3 project.
 *
 * One bit per pixel. Each row of a glyph is padded out to whole bytes, so a
 * row starts on a byte boundary and can be walked a byte at a time.
 *
 * This font goes away in phase 4. LVGL brings its own text rendering, and the
 * only part of the panel driver that survives is the flush.
 */
#ifndef {guard}
#define {guard}

#include <stdint.h>

#include "font.h"

""")
        f.write(f"/** The pixels of every glyph, one after another. */\n")
        f.write(f"static const uint8_t k{args.name}Bits[] = {{\n")
        for i in range(0, len(data), 16):
            chunk = ", ".join(f"0x{b:02X}" for b in data[i : i + 16])
            f.write(f"    {chunk},\n")
        f.write("};\n\n")

        f.write("/** Where each glyph starts and how wide it is. */\n")
        f.write(f"static const FontGlyph k{args.name}Glyphs[] = {{\n")
        for ch, width, offset in glyphs:
            shown = ch if ch not in ('"', "\\") else "\\" + ch
            f.write(f"    {{{offset}, {width}}}, /* '{shown}' */\n")
        f.write("};\n\n")

        if args.chars == "ascii":
            f.write(f"""/** {args.size}px Roboto Condensed, printable ASCII. */
static const Font k{args.name}Font = {{
    k{args.name}Bits, k{args.name}Glyphs, {height}, {first}, {last},
}};
""")
        else:
            f.write(f"""/**
 * {args.size}px Roboto Condensed, the digits and a full stop only.
 *
 * A frequency is all this size ever draws, so the letters are left out. They
 * would be most of the flash this font costs and nothing would ask for them.
 * Anything outside this set draws as a space. See glyphIndex.
 */
static const Font k{args.name}Font = {{
    k{args.name}Bits, k{args.name}Glyphs, {height}, 0, 0,
}};
""")
        f.write(f"\n#endif /* {guard} */\n")

    # clang-format is a gate, and a generated file has to pass it like any
    # other. Formatting here means a regeneration never turns the gate red.
    for cf in ("clang-format", "/opt/homebrew/bin/clang-format"):
        try:
            subprocess.run([cf, "-i", args.out], check=True)
            break
        except (OSError, subprocess.CalledProcessError):
            continue
    else:
        print("clang-format not found, the output is unformatted", file=sys.stderr)

    total = len(data) + len(glyphs) * 4
    print(
        f"{args.out}: {len(glyphs)} glyphs, {height}px tall, "
        f"{len(data)} bytes of pixels, about {total} bytes in all",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    # The exit code matters. Without it a failure here is invisible to
    # anything calling this.
    sys.exit(main())
