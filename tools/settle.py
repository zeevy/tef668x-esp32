"""Measure how long a reading takes to settle after a retune.

Not a test. This produces the data the seek settle time is set from.

The seek thresholds in `core/seek.c` were fitted to a sweep taken with a 400 ms
settle, chosen at the time to take settling out of the question. The seek
itself decides on a reading taken 50 ms after its own retune. Nobody had
checked whether the reading means the same thing at 50 ms as at 400, and this
is what checks it.

    python3 tools/settle.py <out>
    python3 tools/settle.py test/fixtures/seek/settle-2026-09-13.log

For each channel and each settle time it parks the dial one step away, tunes
back, waits, and takes four fresh readings through `POST /api/seek/settle`,
which is the same sequence a seek runs. Parking matters: a probe of the frequency the
radio is already on has no retune to settle from and hands back a fully
settled reading wearing whatever settle time was asked for. The endpoint says
`mvd` for that reason and a reading without it is thrown away here.

The radio is left on the last frequency probed, so tune it back afterwards.
"""
import json
import sys
import time
import urllib.parse
import urllib.request

RADIO = "http://192.168.30.152"
PIN = "000000"

# How far away the dial is parked before each probe, in kHz.
#
# This is not a detail. A reading settles faster after a small retune than
# after a large one, and a seek steps one channel at a time. Parking far away
# measures a jump the seek never makes, and a fixture taken that way says
# channels are less settled than they really are during a seek. The first
# version of this tool parked at 87.5 MHz, produced exactly that error, and
# the change fitted to it made the seek worse on the radio.
#
# 50 kHz is one FM step, which is what a seek does. The larger distances are
# kept because the difference between them is the thing that was missed.
PARK_OFFSETS_KHZ = (50, 200, 1000)

SETTLE_MS = (50, 100, 200)

# Readings per probe, and how far apart. This is the other half of the
# question: whether a channel that looks like a station on one reading still
# looks like one on the next. The shoulder of a strong station drifts with
# that station's modulation, so two readings close together may not be
# independent, and if they are not then confirming a candidate buys nothing.
READS = 4
GAP_MS = 50

REPEATS = 6

# What is being measured, and why each one is here.
CHANNELS = (
    (106400, "station"),
    (95000, "station"),
    (98300, "station"),
    (93500, "station"),
    (93600, "shoulder"),
    (101800, "shoulder"),
    (99500, "empty"),
    (94500, "quiet_band"),
)


def session():
    op = urllib.request.build_opener(urllib.request.HTTPCookieProcessor())
    op.open(RADIO + "/auth", urllib.parse.urlencode({"pin": PIN}).encode())
    return op


def post(op, path, **args):
    return op.open(RADIO + path, urllib.parse.urlencode(args).encode()).read()


def probe(op, khz, ms):
    body = post(op, "/api/seek/settle", khz=khz, ms=ms, n=READS, gap=GAP_MS)
    return json.loads(body)


def main():
    out = open(sys.argv[1], "w")
    op = session()

    out.write("# how a reading settles after a retune, and whether a second\n")
    out.write("# reading agrees with the first\n")
    out.write("# parks=%s repeats=%d reads=%d gap=%d\n"
              % (",".join(str(p) for p in PARK_OFFSETS_KHZ), REPEATS, READS,
                 GAP_MS))
    out.write("# khz park ms probe read sig usn wam off snr what\n")

    for khz, what in CHANNELS:
      for park in PARK_OFFSETS_KHZ:
        for ms in SETTLE_MS:
            for n in range(REPEATS):
                # Park first, or there is no retune and nothing settles.
                post(op, "/api/tune", khz=khz - park)
                time.sleep(0.25)
                r = probe(op, khz, ms)
                if not r["mvd"]:
                    raise SystemExit(
                        "the dial did not move for %d kHz, so this would not "
                        "be a settle measurement" % khz)
                for i, one in enumerate(r["r"]):
                    out.write("%d %d %d %d %d %d %d %d %d %d %s\n" %
                              (khz, park, ms, n, i, one["sig"], one["usn"],
                               one["wam"], one["off"], one["snr"],
                               what.replace(" ", "_")))
            out.flush()
            print("%6d kHz  park %4d  %4d ms  done" % (khz, park, ms))

    out.close()
    print("written to %s" % sys.argv[1])


if __name__ == "__main__":
    main()
