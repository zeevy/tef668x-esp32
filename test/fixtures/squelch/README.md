# Squelch captures

Real readings taken off air from the ATS-125 on 13 September 2026, in Hyderabad, over the HTTP control API. They are what the level floor in the auto squelch is set from, so that it is measured rather than picked.

## Format

The same shape as `test/fixtures/seek/`, one line per reading.

```
khz=102000 sig=68 sav=67 usn=96 wam=283 offset=18 st=False
```

| Field | Meaning |
|---|---|
| `khz` | The frequency that was tuned |
| `sig` | Signal level in tenths of a dBuV, so 334 is 33.4 dBuV |
| `sav` | The same level as the radio itself smoothed it, tenths of a dBuV |
| `usn` | Ultrasonic noise, tenths of a per cent |
| `wam` | Multipath, tenths of a per cent |
| `offset` | How far off centre the carrier reads, tenths of a kHz |
| `st` | The chip saw a stereo pilot |

## The files

| File | What it is |
|---|---|
| `shoulder-2026-09-13.log` | Nine frequencies, sixty readings each, every one a distinct snapshot |

## Why `sav` is captured and not worked out

`signalAverage` in `core/signal.c` forgets per reading, not per millisecond. It keeps nine tenths of what it had and adds the new sample, so its time constant is about ten readings whatever the clock is doing. The radio judges the squelch ten times a second, which makes that about one second. This capture was taken over HTTP at about three readings a second, which would make it about three and a third seconds.

So replaying the `sig` column through the average does not reproduce what the radio does, it applies a much heavier smoothing and flatters the result. The radio's own smoothed level is captured alongside instead, and the floor is judged against that. The tests replay `sav`, not `sig`.

**An earlier capture was thrown away.** It sampled without checking the snapshot sequence number, so about 40 per cent of its lines were the same reading recorded twice, which makes a signal look steadier than it is. Every line in this one is a distinct snapshot, checked by the sequence number and confirmed by there being no two identical consecutive lines.

## What was measured

Three kinds of channel.

| Frequency | What it is | Raw level, dBuV | Smoothed by the radio |
|---|---|---|---|
| 91100, 92700, 98300, 101900, 102800, 106400 | stations | 16.4 to 42.2 | 25.3 to 41.3 |
| 102000 | shoulder of 101.9 | 1.1 to 13.2 | 4.6 to 6.8 |
| 91000 | shoulder of 91.1 | 4.9 to 24.8 | 13.0 to 16.2 |
| 105000 | genuinely empty | -2.5 to 9.0 | 0.7 to 3.5 |

**The noise and multipath limits cannot be relied on to reject a shoulder.** The channel either side of a strong station is not empty: the sidebands of the station next door reach into it, so the tuner reports low noise and low multipath there, which is everything the squelch used to look at. Measured on 102.0 earlier the same day, the reading passed the noise and multipath test on 10 of 12 samples and the audio sat open on it.

How much work each test does depends on the conditions, which is the argument for having both. In this capture, taken in the evening, the two mechanisms had swapped over:

| Channel | noise rejects | multipath rejects | offset rejects | the floor rejects |
|---|---|---|---|---|
| 102000 | 16 of 60 | 53 of 60 | 18 of 60 | 60 of 60 |
| 91000 | 42 of 60 | 29 of 60 | 30 of 60 | 36 of 60 |
| 105000 | 25 of 60 | 60 of 60 | 2 of 60 | 60 of 60 |

## Why the floor is 15 dBuV

Two numbers bound it.

**It must sit below every station.** The weakest station ever measured on this radio read 19.6 dBuV, in the seek sweep the day before. Silence on a station you can hear is a worse fault than audio on a channel you did not want, so the floor errs below rather than above.

**It should sit above the shoulders.** In this capture the higher of the two shoulders smoothed to 16.2 dBuV, which is above 15. Earlier the same afternoon the same channel smoothed to 13.3. It moved three dB in a few hours.

So the window between the loudest shoulder and the weakest station is a few dB and it moves. 15 keeps 4.6 dB of margin below the weakest station and accepts that the shoulder sometimes reaches past it.

## Why one good reading is not enough to open

With the floor at 15, two readings out of sixty on the 91.0 shoulder passed every test at once. They were three apart, and had a smoothed level of 15.8 and 16.0 with noise of 93 and 98 against a limit of 120 and multipath of 226 and 228 against 230: marginal on every axis at the same moment.

Because opening was immediate and shutting waits out a one second hold, each of those readings would unmute the radio for a full second. Requiring two of the last three throws them away, since they were three apart.

| Rule | Shoulder 91.1 | Shoulder 101.9 | Every station |
|---|---|---|---|
| Floor 15, open on one reading | 10 of 60 open, 3 changes | 3 of 60, 1 change | 60 of 60 open, 0 changes |
| **Floor 15, open on two of the last three** | **3 of 60 open, 1 change** | 3 of 60, 1 change | **60 of 60 open, 0 changes** |

The three that remain are the replay starting in the open state and waiting out the hold, not chatter. Raising the floor to 17 would also have shut that shoulder, and it was not done because it would have cost two dB of the margin below the weakest station, which is the side that must not be got wrong.

**Two of the last three, not two in a row.** Two in a row was tried first and is worse than the fault it fixes. A signal sitting exactly on a limit alternates good and bad from one reading to the next, so it never produces two together and would stay muted for ever, and in manual mode the raw level jumps enough that a threshold set near the signal does the same. Two of the last three reaches its count on the second good reading either way, while the shoulder readings, which were three apart, never do.

Staying open takes only one good reading. That asymmetry is the point: opening is a decision about whether there is a station there, and staying open is a decision about whether to silence something somebody is already listening to.

## What it did on the radio

Measured after the change, thirty readings each, with the floor at its default.

| Channel | Open | Changes of state |
|---|---|---|
| 102.0, shoulder | 0 of 30 | 0 |
| 91.0, shoulder | 0 of 30 | 0 |
| 105.0, empty | 0 of 30 | 0 |
| 92.7, station | 30 of 30 | 0 |
| 101.9, station | 30 of 30 | 0 |

Before the change the ticket measured 102.0 as open on 23 of 25 readings with four changes of state.

**Opening is still quick.** Tuning from a shut shoulder straight to a station, the audio opened 0.30, 0.31 and 0.41 seconds after the tune command over three trials, including the HTTP round trip and the tuner settling. It was 0.17 to 0.25 s before opening required two of the last three readings, so about 0.15 s of that is the price of not opening on one reading. The level average is started again on every retune so that it does not also have to climb.

## The floor is a setting

`sqf` on `POST /api/settings`, in whole dBuV, 0 to 40, where 0 switches it off and leaves the squelch judging on noise, multipath and offset alone, which is what the reference firmware does.

It is settable because it is the fragile number. It rejects the channel beside a strong station, and how strong that channel reads depends on where the radio is and what is on air. Confirmed wired on the radio rather than assumed: on 101.9 reading 25.1 dBuV smoothed, a floor of 15 left it open on 15 of 15 readings, a floor of 40 shut it on 15 of 15, and 15 again reopened it.

## The interaction with seek

Seek has its own level floor, in `core/seek.c`, which runs from 19.0 dBuV at sensitivity 1 down to 4.0 at sensitivity 6. At the default of 4 that is 10.0 dBuV, which is **below** this squelch floor, so seek can in principle stop on a channel the squelch then mutes. A reading in this capture would do it: raw 20.5 passes seek's level test while the smoothed 16.2 or less fails the squelch's.

It was tested rather than assumed. Three seeks from below each of the two shoulders stepped past them and stopped on real stations, with the audio open every time, because seek's noise and multipath tests reject the shoulders even though its level test alone would not.

The two are deliberately not coupled: the seek sensitivity is a choice about how hard to hunt, and the squelch floor is a fact about what a station looks like here. Checklist row 152g is the check, and it says to report it rather than nudge either number.

## How it was taken

Tuning each frequency over `POST /api/tune`, waiting three seconds for the reading to settle, then sampling `GET /api/state` until sixty distinct snapshots had been seen, rejecting any repeat of the `seq` field. A sweep is worth retaking from a different place or at a different time of day. The two shoulders in this capture differ by nine dB in their smoothed level, and one of them moved three dB in an afternoon, so a rule that works here today is not yet known to be a good rule.
