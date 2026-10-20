"""Find a text that runs past its box, off the panel, or into another text.

Reads what the panel shows as GET /api/screen gives it, one JSON object a
line: the screenshot tool writes one such file beside each picture, and a
script can save the radio's own answers the same way. A longer language is
where this goes wrong, so a run is compared with English:

    python3 tools/text_fit.py <dir>                 every finding
    python3 tools/text_fit.py <dir> --base <en dir> only what English has not

Each finding names the file, the text and what it ran into. Exits 1 when
there is one.

What counts:
- A text past the panel's edge.
- A text that overlaps another text.
- A text that starts inside a box, a tile or a panel, and runs past its right
  edge. The box is the smallest one drawn behind the start of the text.
- A text the panel had to cut short with an ellipsis to fit.
"""
import argparse
import glob
import json
import os
import sys

W, H = 320, 240


def read(path):
    texts, boxes = [], []
    for line in open(path, encoding="utf-8"):
        line = line.strip()
        if not line:
            continue
        item = json.loads(line)
        if "text" in item:
            if item["text"].strip():
                texts.append(item)
        elif "x" in item and "zone" not in item:
            if item.get("fill") or item.get("border"):
                boxes.append(item)
    return texts, boxes


def overlap(a, b):
    return (a["x"] < b["x"] + b["w"] and b["x"] < a["x"] + a["w"] and
            a["y"] < b["y"] + b["h"] and b["y"] < a["y"] + a["h"])


def findings(path):
    """(key, words): the key is the same for the same fault in any language,
    so a fault English already has is not reported again."""
    texts, boxes = read(path)
    out = []
    for i, t in enumerate(texts):
        right, bottom = t["x"] + t["w"], t["y"] + t["h"]
        if t["text"].endswith("\u2026"):
            out.append((("cut", i), "%r is cut short to fit" % t["text"]))
        if t["x"] < 0 or right > W or bottom > H:
            out.append((("edge", i), "%r runs off the panel (%d..%d)" %
                        (t["text"], t["x"], right)))
        for j in range(i + 1, len(texts)):
            if overlap(t, texts[j]):
                out.append((("overlap", i, j), "%r runs into %r" %
                            (t["text"], texts[j]["text"])))
        cx, cy = t["x"], t["y"] + t["h"] // 2
        behind = [b for b in boxes if b["w"] < W and
                  b["x"] <= cx < b["x"] + b["w"] and
                  b["y"] <= cy < b["y"] + b["h"]]
        if behind:
            box = min(behind, key=lambda b: b["w"] * b["h"])
            if right > box["x"] + box["w"]:
                out.append((("box", i), "%r runs %d px past its box at x %d" %
                            (t["text"], right - box["x"] - box["w"],
                             box["x"] + box["w"])))
    return out


def files(where):
    if os.path.isdir(where):
        return sorted(glob.glob(os.path.join(where, "*.jsonl")))
    return [where]


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("where", help="a .jsonl file, or a folder of them")
    ap.add_argument("--base", help="the same captures in English")
    args = ap.parse_args()
    found = 0
    for path in files(args.where):
        known = set()
        if args.base:
            twin = os.path.join(args.base, os.path.basename(path))
            if os.path.exists(twin):
                known = {k for k, _ in findings(twin)}
        for key, words in findings(path):
            if key not in known:
                print("%s: %s" % (os.path.basename(path), words))
                found += 1
    print("%d findings" % found)
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
