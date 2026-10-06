"""Make the pictures for the README and the wiki, in assets/.

The screens come from tools/screenshot.sh, which runs the firmware's own
drawing code on this machine and writes the same pixels the panel gets. So a
picture made here shows exactly what the radio draws, nothing made up.

Each screen is drawn at twice its size inside a dark rounded frame, with a
clear background so it sits well on a light or a dark page. Three pictures
put several screens side by side for the README.

Standard library only, so it needs nothing installed.

    tools/screenshot.sh && python3 tools/make_assets.py
"""
import os
import struct
import sys
import zlib

IN_DIR = os.path.join(".pio", "shots")
OUT_DIR = "assets"

FRAME = (0x1C, 0x1D, 0x20)

# One picture per screen, for the wiki, by the name the screenshot tool gives.
SCREENS = [
    # The radio screen.
    "cap-fm-106400", "cap-fm-93500", "fm", "fm-memory", "fm-typing",
    "fm-squelched", "fm-logged", "fm-checking-updates", "mw", "sw", "oirt",
    "touch-radio",
    # The RDS pages.
    "rds-station", "touch-rds", "rds-text", "rds-networks", "rds-decoder",
    # DX mode.
    "dx-station", "touch-dx", "dx-scope", "touch-scope", "dx-scan-run",
    "dx-catches",
    # The menu.
    "menu-groups", "touch-menu", "menu-sub", "menu-value", "touch-value",
    "menu-theme", "touch-picker", "menu-presets",
    "menu-station-log", "menu-network-info", "menu-diagnostics",
    "menu-about", "menu-typed-choice", "menu-update-offer",
    # The bandwidth page.
    "bw-fm", "touch-bw", "bw-mw",
    # Start up, update, sleep and recovery.
    "boot", "veil", "update-failed", "sleeping", "recovery",
    # Touch calibration.
    "touch-cal-mark2", "touch-cal-kept",
]

THEMES = [
    "nightwatch", "clearDay", "phosphor", "redNight", "lcd", "paper",
    "ember", "violet", "slate", "mono", "hiFi", "highContrast", "daylight",
    "clear", "blossom",
]

# The pictures the README shows: name, the screens in it, how many across,
# and the scale.
SHEETS = [
    ("hero", ["fm", "rds-station", "dx-scope"], 3, 2),
    ("tour", ["rds-text", "dx-catches", "menu-groups", "bw-fm",
              "dx-scan-run", "mw"], 3, 1),
    ("themes", ["theme-%s-fm" % t for t in THEMES], 5, 1),
]


def read_bmp(path):
    """Width, height and the pixels as RGB bytes, top row first."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:2] != b"BM":
        raise SystemExit("%s is not a BMP" % path)
    offset = struct.unpack_from("<I", data, 10)[0]
    width, height = struct.unpack_from("<ii", data, 18)
    bits = struct.unpack_from("<H", data, 28)[0]
    if bits != 24:
        raise SystemExit("%s is %d bits, this reads 24" % (path, bits))
    # A positive height means the rows are stored bottom up.
    bottom_up = height > 0
    height = abs(height)
    stride = (width * 3 + 3) & ~3
    rows = []
    for y in range(height):
        start = offset + y * stride
        line = bytearray(data[start:start + width * 3])
        # BMP stores blue, green, red.
        line[0::3], line[2::3] = line[2::3], line[0::3]
        rows.append(bytes(line))
    if bottom_up:
        rows.reverse()
    return width, height, rows


def scale(image, times):
    """Each pixel made `times` pixels wide and tall, with no blur."""
    width, height, rows = image
    if times == 1:
        return image
    out = []
    for row in rows:
        wide = bytearray()
        for x in range(width):
            wide += row[x * 3:x * 3 + 3] * times
        out.extend([bytes(wide)] * times)
    return width * times, height * times, out


def framed(image, pad, radius):
    """The screen inside a dark frame with round outer corners, as RGBA.

    The corners are smoothed by how much of each pixel falls inside the
    curve, so they do not look stepped.
    """
    width, height, rows = image
    w, h = width + 2 * pad, height + 2 * pad
    out = bytearray(w * h * 4)
    solid = bytes(FRAME + (255,))
    for y in range(h):
        line = bytearray(solid * w)
        dy = max(radius - y, y - (h - 1 - radius), 0)
        if dy > 0:
            for x in range(radius):
                dx = radius - x
                dist = ((dx - 0.5) ** 2 + (dy - 0.5) ** 2) ** 0.5
                alpha = int(max(0.0, min(1.0, radius - dist)) * 255)
                line[x * 4 + 3] = alpha
                line[(w - 1 - x) * 4 + 3] = alpha
        out[y * w * 4:(y + 1) * w * 4] = line
    for y, row in enumerate(rows):
        rgba = bytearray(width * 4)
        rgba[0::4] = row[0::3]
        rgba[1::4] = row[1::3]
        rgba[2::4] = row[2::3]
        rgba[3::4] = b"\xff" * width
        at = ((y + pad) * w + pad) * 4
        out[at:at + width * 4] = rgba
    return w, h, out


def sheet(images, across, gap):
    """Framed pictures in rows, `across` to a row, on a clear background."""
    w, h = images[0][0], images[0][1]
    down = (len(images) + across - 1) // across
    width = across * w + (across - 1) * gap
    height = down * h + (down - 1) * gap
    out = bytearray(width * height * 4)
    for i, (_, _, px) in enumerate(images):
        left = (i % across) * (w + gap)
        top = (i // across) * (h + gap)
        for y in range(h):
            at = ((top + y) * width + left) * 4
            out[at:at + w * 4] = px[y * w * 4:(y + 1) * w * 4]
    return width, height, out


def write_png(path, image):
    width, height, px = image
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        raw += px[y * width * 4:(y + 1) * width * 4]

    def chunk(kind, data):
        body = kind + data
        return (struct.pack(">I", len(data)) + body +
                struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF))

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR",
                      struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
        f.write(chunk(b"IEND", b""))


def load(name):
    path = os.path.join(IN_DIR, name + ".bmp")
    if not os.path.exists(path):
        return None
    return read_bmp(path)


def main():
    if not os.path.isdir(IN_DIR):
        raise SystemExit("no %s yet. Run tools/screenshot.sh first" % IN_DIR)
    os.makedirs(os.path.join(OUT_DIR, "screens"), exist_ok=True)
    os.makedirs(os.path.join(OUT_DIR, "themes"), exist_ok=True)
    missing = []

    singles = [(n, os.path.join("screens", n)) for n in SCREENS]
    singles += [("theme-%s-fm" % t, os.path.join("themes", t)) for t in THEMES]
    for name, out in singles:
        image = load(name)
        if image is None:
            missing.append(name)
            continue
        path = os.path.join(OUT_DIR, out + ".png")
        write_png(path, framed(scale(image, 2), 20, 28))
        print("  " + path)

    for name, screens, across, times in SHEETS:
        images = [load(s) for s in screens]
        if any(i is None for i in images):
            missing.append(name)
            continue
        pad, radius, gap = 10 * times, 14 * times, 16 * times
        frames = [framed(scale(i, times), pad, radius) for i in images]
        path = os.path.join(OUT_DIR, name + ".png")
        write_png(path, sheet(frames, across, gap))
        print("  " + path)

    if missing:
        print("left out, not drawn: " + ", ".join(missing))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
