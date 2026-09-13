# Volume AGC captures

Real recordings taken off air from the ATS-125 on 12 September 2026, in
Hyderabad, on local FM stations. They come from the volume AGC built on the
PE5PVB firmware, which is where the AGC design in this project comes from.

Use them as test fixtures for the AGC in `core/`. They are not invented data, so
they carry the awkward cases that made up numbers do not: a signal that swings
between 10 and 38 dBuV, modulation readings above 120 percent, and the unsigned
wrap that turns a negative reading into a number in the thousands.

## Format

One sample per line, about ten samples a second, printed once a second.

```
AGC on=50 freq=10400 sig=26 usn=2 mod=64 avg=64.0 ticks=1 want=-2 gain=1 base=-8
```

| Field | Meaning |
|---|---|
| `on` | AGC target in modulation percent. 0 means off |
| `freq` | Frequency in units of 10 kHz. 10400 is 104.0 MHz |
| `sig` | Signal level in dBuV |
| `usn` | Ultrasonic noise, tenths |
| `mod` | Modulation depth in percent, as the tuner reported it |
| `avg` | The AGC running average of `mod` |
| `ticks` | Usable samples since the last tune, capped at 20 |
| `want` | Gain the AGC wants, in dB |
| `gain` | Gain actually applied, in dB |
| `base` | Volume the user asked for, before the AGC |

## The files

| File | What it shows |
|---|---|
| `01-target50-signal-gate-too-high.log` | The signal gate set at 20 dBuV. On 104.0 the signal sat at 12 to 18, so the average froze at 77.2 and the gain stayed at 0 for 24 seconds. A guessed threshold that silently switched the feature off |
| `02-target50-measure-apply-split.log` | After separating measuring from applying, and lowering the signal gate to 8 dBuV |
| `03-target50-review-fixes.log` | After the code review fixes: idle release, a freshness check on the reading, and the noise gate made FM only |
| `04-target50-boost-off.log` | Cut only, no boost. Shows loud stations coming down and quiet ones staying where they are |
| `05-target50-boost6-final.log` | Target 50 with up to 6 dB of boost. Ten stations across FM, the final tuning |

## What the last capture measured

Ten stations, settled readings only, target 50 and boost 6 dB.

| Station | Average modulation | Gain | Effective level |
|---|---|---|---|
| 104.0 | 114.4 | -7.1 dB | 50.4 |
| 92.7 | 91.8 | -4.5 dB | 54.9 |
| 93.5 | 73.9 | -2.9 dB | 53.2 |
| 91.1 | 70.7 | -2.0 dB | 56.1 |
| 98.3 | 64.9 | -2.0 dB | 51.5 |
| 94.3 | 64.1 | -2.5 dB | 48.1 |
| 95.0 | 54.3 | +0.2 dB | 55.9 |
| 102.8 | 47.4 | +0.5 dB | 50.3 |
| 101.9 | 44.3 | +1.0 dB | 49.8 |
| 106.4 | 44.1 | +0.5 dB | 46.9 |

The spread went from about 9 dB to 1.6 dB. A regression test that replays this
file and checks the settled gain per station should reproduce these numbers.

## What these captures can and cannot be used for

**One line is not one AGC update.** The prototype ran ten times a second and
printed once a second, which the `ticks` column shows: it climbs by up to ten
between consecutive lines on the same station. So these rows cannot be fed
through a running average one at a time to reproduce the `avg` column. An
attempt to do that reproduced 7 rows of 179.

What they are good for is the pairs that sit on one line together. The `avg`
and `want` columns are the important one: `want` is the gain that average
asked for, worked out in the prototype with `log10f`. `core/agc.c` works the
same thing out with a table of boundaries and no floating point, and the test
replays every row that had a settled average, 168 of them, and requires the
same answer on all of them.

They are also the source of the ranges the guards have to cope with, which is
what the section below is about.

## Cases worth writing a test around

- **Modulation above 120.** 104.0 reports 134, 146, 150 and 153. These are real
  over modulation, not errors. A limit of 120 throws away the loudest samples
  and under corrects the loudest station.
- **Unsigned wrap.** The tuner hands modulation back unsigned, so a negative raw
  reading arrives as 3276 or more. Anything above 200 is garbage and has to be
  dropped, not clamped. **No capture here contains one.** The highest reading
  anywhere in these five files is 153. That guard therefore stands on what the
  chip does, which `drivers/tef668x.h` records, and not on this evidence. It is
  also second in line rather than first: this firmware's driver reads the word
  as signed, so a negative reading arrives as a small negative and is rejected
  by the silence test instead. A test asserts that no capture holds such a
  reading, so that nobody later believes these files prove it.
- **Nothing about an empty channel.** All five were taken on real stations, so
  they say nothing about where the noise gate should sit. That number is
  measured from `test/fixtures/squelch/shoulder-2026-09-13.log` instead, which
  holds stations, the channel beside a strong station and an empty channel all
  sampled the same way. The comment on `AGC_MAX_NOISE_TENTHS` has the working.
  The highest noise reading in these five AGC captures is 57 tenths, in file
  02.

- **A signal that will not sit still.** On 104.0 in file 01 the level swings
  between 10 and 38 dBuV within seconds. Any gate against a fixed level has to
  cope with that.
- **Repeated readings.** On a weak FM signal the tuner is only read every
  300 ms while the AGC ticks every 100 ms, so the same sample appears three
  times. Counting it three times makes a settling guard useless.
