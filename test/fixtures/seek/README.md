# Seek captures

Real readings taken off air from the ATS-125 on 13 September 2026, in Hyderabad, over the HTTP control API. They are what the seek stop decision in `core/seek.c` is set from, so that its thresholds are measured rather than picked.

## Format

`fm-sweep-2026-09-13.log` has one line per channel, in the order they were visited.

```
khz=87500 sig=-72 usn=189 wam=333 offset=8 mod=20 st=False snr=6
```

| Field | Meaning |
|---|---|
| `khz` | The frequency that was tuned |
| `sig` | Signal level in tenths of a dBuV, so 475 is 47.5 dBuV |
| `usn` | Ultrasonic noise, tenths of a per cent |
| `wam` | Multipath, tenths of a per cent. FM only |
| `offset` | How far off centre the carrier reads, tenths of a kHz |
| `mod` | Modulation depth, per cent |
| `st` | The chip saw a stereo pilot |
| `snr` | Signal to noise in dB, worked out by `core/signal.c`, not read from the chip |

## The files

| File | What it is |
|---|---|
| `fm-sweep-2026-09-13.log` | Every 100 kHz channel from 87.5 to 108.0 MHz, 206 of them, tuned in turn with 400 ms to settle before reading |
| `settle-2026-09-13.log` | Eight channels probed at three park distances and three settle times, four readings a probe. 1728 readings. Taken the same night for ticket 32. A different format, described below |

The settle log has a line per reading rather than per channel:

```
# khz park ms probe read sig usn wam off snr what
106400 50 50 0 0 434 36 49 76 29 station
```

`park` is how far away the dial was before the retune, `ms` how long it waited before the first reading, `probe` which visit to the channel this was and `read` which reading within it. `what` is what the channel actually is, established by ear and by the stereo flag rather than by the gates being tested.

## What was on air

Six stations, confirmed by ear and by the stereo flag:

| kHz | sig | usn | wam | offset | snr |
|---|---|---|---|---|---|
| 93500 | 330 | 14 | 35 | +59 | 25 |
| 95000 | 317 | 14 | 36 | +57 | 24 |
| 98300 | 466 | 14 | 30 | +65 | 31 |
| 101900 | 475 | 6 | 29 | +58 | 31 |
| 104000 | 267 | 31 | 31 | +55 | 22 |
| 106400 | 367 | 23 | 32 | +84 | 26 |

Four frequencies on the known station list in `docs/test-checklist.md` were not transmitting that afternoon: 91100, 92700, 94300 and 102800. They read as noise here and the tests treat them as noise, which is what they were.

## What it shows

**Ultrasonic noise and multipath separate a station from an empty channel, and signal level does not.** Real stations read `usn` 6 to 31 and `wam` 29 to 36. The next best thing anywhere on the band reads 52 and 228. There is a wide empty gap between the two, and nothing sits in it.

Signal level is the weakest of the four. The shoulders either side of a strong station reach 204 and 218, which is well above the 267 of a real but quieter station. A seek that looks at level alone stops on the edges of the station it just left.

**The reported carrier offset is not centred on zero.** All six stations read between +42 and +84, which is the 50 ppm crystal error already recorded in HARDWARE.md. The reference firmware's seek uses an offset window of plus or minus 80, and on this radio that has almost no margin left: 106.4 read +84 in this sweep and would have been skipped. The window in `core/seek.c` is wide for that reason, and the discrimination is left to noise and multipath, which the numbers above show can carry it.

## What a second day of measuring changed

The sweep above is one afternoon. Two of the three thresholds in `core/seek.c` were changed after running the seek on the radio the next day, and both changes are worth recording because the sweep alone would have left them wrong.

**A level floor was added.** The sweep says signal level is the weakest of the four measurements, and it is, for telling a station from an empty channel. It is the best of them for telling a station from the shoulder of another station. Running the first version of the seek, it stopped on 102.0 MHz, which sits beside 101.9, the strongest station on the band. Measured there, 102.0 reads an ultrasonic noise of 25 to 87 and a multipath under 130, both comfortably inside the limits, because there is real signal present: the sidebands of the station next door. What it does not have is level, at -1.8 dBuV against 19.6 for the weakest real station.

So the three gates each reject something the other two cannot, and none of them is enough alone.

**The multipath limit was loosened, from 200 to 320 at the default.** In this sweep nothing on the whole band read a multipath above 36, so any limit above that looked safe. The next day 95.0 MHz read 204 to 279 across nine samples and was plainly a station, stereo and audible: a weaker one with reflections. A limit fitted to the sweep alone skipped it every time, and the reference firmware's fixed 230 would have skipped it about half the time.

The lesson is not that the sweep was wrong. It is that one sweep measures one afternoon, and a threshold sitting in the empty gap between two clusters is only as wide as the day that produced them.

## The settle capture, and what it ruled out

`settle-2026-09-13.log` was taken on the night of 13 September 2026 for ticket 32, after a seek was seen stopping on the shoulder of 93.5 and reporting a find while the auto squelch held the audio shut.

It was taken with `tools/settle.py` through `POST /api/seek/settle`, which tunes, waits a controlled time and takes fresh readings off the tuner. That endpoint exists because nothing else could see what a seek decides on: `GET /api/state` serves the last polled reading, which is up to a poll interval old and often describes the channel before this one, and a sweep driven over HTTP measures that same stale path.

Each probe is one retune, a wait, then four readings 50 ms apart. Three park distances, three settle times, eight channels, six repeats, which is 1728 readings.

**It ruled out two explanations that looked right and were not.** Both are recorded because each produced a change that was written before it was checked.

*The settle time is not the problem.* The seek decides 50 ms after its own retune while the sweep above used 400 ms, so the thresholds appeared to have been fitted to readings the seek never sees. They were not: across every probe, all four stations on air passed every gate on every reading at 50, 100 and 200 ms alike. A station is settled by 50 ms.

*The distance the dial moved is not the problem either.* A reading might reasonably settle more slowly after a large jump than a small one, and the first version of `tools/settle.py` parked at 87.5 MHz, which is a 7 MHz jump where a seek steps 50 kHz. Measured across parks of 50, 200 and 1000 kHz, the pass rate at 50 ms is the same. The tool now parks one step away regardless, because that is what a seek does.

**What was really happening was intermittent interference.** Around 10pm the whole 94 MHz region read 10 to 15 dBuV with no stereo pilot and no RDS, and the seek stopped in it seven times out of eight. Ninety minutes later the same channels read -5 to -8 and the seek found 95.0 six times out of six. Three stations, 91.1, 94.3 and 102.8, also left the air during the same evening, dropping from 43, 45 and 37 dBuV to about -5.

So the defect is real and its cause is outside the radio. That matters for anyone retaking these numbers: **the band here is not the same from hour to hour**, and a threshold fitted in one sitting is worth what the sweep note above already says it is worth.

## The gap that did not need the interference

What the same session did settle, and without needing the band to misbehave, is that the seek and the auto squelch were free to disagree.

| Gate | Seek allowed | Squelch allowed |
|---|---|---|
| Level | 10.0 dBuV | 15.0 dBuV |
| Multipath | 320 | 230 |
| Carrier off centre | 20 kHz | 10 kHz |

A channel inside any of those three gaps is one the seek stops on and the squelch immediately mutes, on any night. That is where `found=True` with the audio shut came from, and it is a contradiction inside the radio rather than a threshold fitted to weather.

Closing only the level was tried and was not enough: a seek then stopped on 93.45 at 20.0 dBuV, above the floor, and the squelch shut on the offset. The seek now asks the squelch instead of keeping its own copy of any of it. Decision 33.

## How it was taken

`tools/sweep.py`, tuning each channel over `POST /api/tune`, waiting 400 ms, then reading `GET /api/state`. The 400 ms is far longer than the tuner needs and was chosen to take the settle time out of the question. The reference firmware uses 50 ms in its own seek loop.

A sweep is worth retaking from a different place or at a different time of day. A rule that only works on six strong local stations on one afternoon is not yet known to be a good rule.
