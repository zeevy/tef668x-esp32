"""Step across a band and record what every channel reads.

Not a test. This produces the data the seek stop decision is set from, so it
has to be real readings off this radio rather than numbers somebody picked.

    python3 tools/sweep.py <lowKHz> <highKHz> <stepKHz> <settleSeconds> <out>
    python3 tools/sweep.py 87500 108000 100 0.4 fm-sweep.log

The settle time is deliberately far longer than the tuner needs, so that a
reading which has not caught up cannot be mistaken for an empty channel. The
radio is left on the last frequency of the sweep, so tune it back afterwards.

Output goes in test/fixtures/seek/ when it is worth keeping. See the README
there for the format and for what the first sweep showed.
"""
import json
import sys
import time
import urllib.parse
import urllib.request

RADIO = "http://192.168.30.152"
PIN = "000000"


def open_session():
    jar = urllib.request.HTTPCookieProcessor()
    op = urllib.request.build_opener(jar)
    op.open(RADIO + "/auth", urllib.parse.urlencode({"pin": PIN}).encode())
    return op


def post(op, path, **args):
    return op.open(RADIO + path, urllib.parse.urlencode(args).encode()).read()


def state(op):
    return json.loads(op.open(RADIO + "/api/state").read())


def main():
    lo = int(sys.argv[1])
    hi = int(sys.argv[2])
    step = int(sys.argv[3])
    settle = float(sys.argv[4])
    out = open(sys.argv[5], "w")

    op = open_session()
    freq = lo
    while freq <= hi:
        try:
            post(op, "/api/tune", khz=freq)
        except Exception as exc:
            print("tune %d failed: %s" % (freq, exc), file=sys.stderr)
            freq += step
            continue
        time.sleep(settle)
        try:
            t = state(op)["tun"]
        except Exception as exc:
            print("read %d failed: %s" % (freq, exc), file=sys.stderr)
            freq += step
            continue
        line = ("khz=%d sig=%s usn=%s wam=%s offset=%s mod=%s st=%s snr=%s"
                % (freq, t.get("sig"), t.get("usn"), t.get("wam"),
                   t.get("off"), t.get("mod"), t.get("st"), t.get("snr")))
        print(line)
        out.write(line + "\n")
        out.flush()
        freq += step
    out.close()


main()
