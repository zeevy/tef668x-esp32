"""Record the raw RDS groups a station is sending.

Not a test. This produces the data the RDS decoder is checked against, so it
has to be a real broadcast rather than groups somebody made up. Real stations
send damaged blocks, repeat themselves, scroll their names and pad their text,
and none of that appears in invented data.

    python3 tools/rdscap.py <khz> <seconds> <out>
    python3 tools/rdscap.py 101900 120 fm-101900.log

It tunes the radio, waits for the tuner to settle and its decoder to lock,
then polls GET /api/rds/raw and writes every group it has not already seen.
The radio is left on the frequency that was captured.

A capture is only worth having if it is unbroken, and two different things can
break one.

This script can be too slow. The radio holds the last 128 groups, and a station
sends about 11.4 a second, so that is about eleven seconds against a poll every
one. A group lost that way leaves a jump in the sequence numbers.

The radio can fail to keep a group, when the web task holds the lock as one
arrives. That group never takes a sequence number, so it leaves no jump and
the only sign of it is the radio's own `lost` count going up.

Both are watched, and both are written into the file as a `# missed` line.
tools/make_rds.py then refuses any capture carrying one, so a broken capture
cannot quietly become a fixture that reads as one continuous broadcast.

Output goes in test/fixtures/rds/ when it is worth keeping. See the README
there for the format and for what the first captures showed.
"""
import json
import sys
import time
import urllib.parse
import urllib.request

RADIO = "http://192.168.30.152"
PIN = "000000"

# How often to ask. The radio holds about eleven seconds of groups, so this is
# well inside what it can keep.
POLL_SECONDS = 1.0

# How long to give the tuner and its RDS decoder after a retune. The decoder
# needs a few groups to lock, and a capture that starts before it has is a
# capture of nothing.
SETTLE_SECONDS = 2.0


def open_session():
    jar = urllib.request.HTTPCookieProcessor()
    op = urllib.request.build_opener(jar)
    op.open(RADIO + "/auth", urllib.parse.urlencode({"pin": PIN}).encode())
    return op


def post(op, path, **args):
    return op.open(RADIO + path, urllib.parse.urlencode(args).encode()).read()


def state(op):
    return json.loads(op.open(RADIO + "/api/state").read())


# Header fields the radio prints as hex rather than as a decimal count.
HEX_FIELDS = ("stat",)


def head_value(key, text):
    """One header field, or None where the radio could not say.

    The radio answers `stat=?` and `read=?` when the radio task held the lock,
    which is a real answer and not a number. And `stat` is hex, so reading it
    as decimal is wrong on every word without a letter in it and raises on
    every word with one.
    """
    if text == "?":
        return None
    return int(text, 16 if key in HEX_FIELDS else 10)


def raw(op):
    """One poll. Returns the header fields and the groups, oldest first."""
    body = op.open(RADIO + "/api/rds/raw").read().decode()
    head = {}
    groups = []
    for line in body.splitlines():
        if line.startswith("#"):
            for field in line[1:].split():
                key, _, value = field.partition("=")
                head[key] = head_value(key, value)
            continue
        if not line.strip():
            continue
        parts = line.split()
        groups.append((int(parts[0]), parts[1], parts[2], parts[3], parts[4],
                       parts[5]))
    return head, groups


def main():
    khz = int(sys.argv[1])
    seconds = float(sys.argv[2])
    out = open(sys.argv[3], "w")

    op = open_session()
    post(op, "/api/tune", khz=khz)
    time.sleep(SETTLE_SECONDS)

    now = state(op)
    tuner = now.get("tun", {})
    print("tuned %s %s, signal %s" %
          (tuner.get("f"), tuner.get("unt"), tuner.get("sav")))

    out.write("# khz=%d\n" % khz)
    out.write("# seq A B C D err\n")

    seen = -1
    missed = 0
    written = 0
    lost = 0
    started = time.time()
    while time.time() - started < seconds:
        head, groups = raw(op)

        # A group the radio could not put in its ring leaves no gap in the
        # sequence, because it never took a number. So the check below cannot
        # see it and this is the only thing that can. Without it the file
        # would join two pieces of a broadcast with nothing saying so, and a
        # test written against it would be testing a stream nobody sent.
        now_lost = head.get("lost") or 0
        if now_lost > lost:
            out.write("# missed %d groups here, the radio could not keep "
                      "them\n" % (now_lost - lost))
            missed += now_lost - lost
            lost = now_lost

        for seq, a, b, c, d, err in groups:
            if seq <= seen:
                continue
            if seen >= 0 and seq != seen + 1:
                missed += seq - seen - 1
                out.write("# missed %d groups here, this script was too "
                          "slow\n" % (seq - seen - 1))
            out.write("%d %s %s %s %s %s\n" % (seq, a, b, c, d, err))
            seen = seq
            written += 1
        out.flush()
        time.sleep(POLL_SECONDS)

    final = state(op).get("tun", {}).get("rds", {})
    out.write("# ps=%s pi=%s pty=%s\n" %
              (final.get("ps", ""), final.get("pi", ""), final.get("ptn", "")))
    out.close()

    print("%d groups written, %d missed" % (written, missed))
    print("decoded: ps=%r pi=%s pty=%s rt=%r" %
          (final.get("ps"), final.get("pi"), final.get("ptn"),
           final.get("rt")))


if __name__ == "__main__":
    main()
