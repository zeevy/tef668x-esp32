# RDS captures

Real groups taken off air from the ATS-125 on 13 September 2026, in Hyderabad, over `GET /api/rds/raw`. They are what `core/rds.c` is checked against, so that the decoder is tested on a broadcast that was actually transmitted rather than on groups somebody wrote out.

## Format

One line per group, oldest first. Comment lines start with `#`.

```
# khz=93500
# seq A B C D err
0 0935 0408 E0CD 2020 00
1 0935 2400 2020 5245 00
```

| Field | Meaning |
|---|---|
| `seq` | Group number since the radio tuned this station. A gap means the capture missed one, and the script writes a `# missed` line where that happened |
| `A` to `D` | The four blocks, as hex |
| `err` | The tuner's own confidence, two bits per block with block A in the top pair. 0 clean, 1 or 2 corrected, 3 not corrected |

The last line of each file records what the radio itself had decoded when the capture ended.

## The files

| File | Station | Groups | Group types |
|---|---|---|---|
| `fm-91100-2026-09-13.log` | Radio City 91.1 | 513 | 0A 388, 2A 125 |
| `fm-93500-2026-09-13.log` | Red FM 93.5 | 1032 | 0A 516, 2A 516 |
| `fm-94300-2026-09-13.log` | Fever FM 94.3 | 532 | 0B 265, 2A 267 |
| `fm-95000-2026-09-13.log` | Mirchi 95 | 533 | 0A 106, 2A 427 |
| `fm-98300-2026-09-13.log` | Radio Mirchi 98.3 | 706 | 0A 140, 2A 566 |
| `fm-106400-2026-09-13.log` | Magic FM 106.4 | 692 | 0A 346, 2A 346 |
| `fm-91100-weak-2026-09-13.log` | Radio City 91.1, aerial collapsed | 1387 | 0A and 2A |
| `fm-106400-weak-2026-09-13.log` | Magic FM 106.4, aerial collapsed | 1390 | 0A and 2A |

The first six were taken with the whip out, at 34 to 48 dBuV. The last two were taken with it collapsed, at about 10 dBuV, and they are the only real damaged data there is.

## What is on air here

Thirteen FM stations are listed for Hyderabad. Each was tuned in turn on 13 September 2026 and the RDS status word read.

| kHz | Station | Signal | RDS |
|---|---|---|---|
| 90400 | Bol 90.4 | 8.9 dBuV | Not receivable here |
| 91100 | Radio City | 43.5 | Yes |
| 92700 | Big FM | 44.6 | No. The decoder locks and no group ever arrives |
| 93500 | Red FM | 47.5 | Yes |
| 94300 | Fever FM | 45.4 | Yes |
| 95000 | Mirchi 95 | 47.3 | Yes |
| 98300 | Radio Mirchi | 43.0 | Yes |
| 101900 | AIR FM Rainbow | 34.9 | No |
| 102800 | AIR Vividh Bharati | 37.5 | No |
| 104000 | Kool 104 | 28.3 | No |
| 105600 | Gyan Vani | 5.1 | Not receivable here |
| 106400 | Magic FM | 34.3 | Yes |
| 107800 | Deccan Radio | 6.1 | Not receivable here |

Six of the ten receivable stations carry RDS. Neither All India Radio service does.

## What the captures showed

### Two stations send no identifier at all

Fever FM 94.3 and Mirchi 95 send a programme identifier of `0000` in every single group, 532 and 533 of them. The standard keeps `0000` for a station that has not been given one, so it is not an identifier. A decoder that publishes it states `0000` as a fact, and anything comparing identifiers decides those two stations are the same one. `core/rds.c` treats zero as no identifier for that reason.

The other four send `3712`, `0935`, `26FF` and `1064`. Three of those four encode their own frequency in the last three digits, which is a local convention and not something to rely on.

### Two stations scroll their name, and one of them scrolls five

The station name field is eight characters and arrives two at a time over four groups. Some stations use it as a ticker.

| Station | Names it sends, in turn |
|---|---|
| Radio City 91.1 | `City FM `, `  ante  `, ` Radio  `, `  City  `, `  Moge  ` |
| Mirchi 95 | `MIRCHI 9`, `5       ` |

Mirchi 95 sends each of its two three times before changing. Radio City rotates five in the first capture, and the longer weak one catches it rotating eight, including `91.1 FM `, `  91.1  `, ` Madhi  ` and `Madhi Lo`, which is an advertisement running through the station name field.

**This is what set the rule for publishing a name.** The first version of the decoder confirmed each of the eight positions on its own: a position was accepted once the same character had arrived there twice running. Replaying the Mirchi 95 capture through that, the radio published `5   HI 9`, which is half of one name and half of the other and was never transmitted. Different positions settled in different passes.

So a name is now published only when two complete passes of all eight positions arrive saying the same thing. Whole passes cannot mix. A station that rotates names shows each of them in turn, which is what it is sending, and there is no arrangement of the two that produces a name it did not send. `every_name_shown_is_one_the_station_sent` in `test/test_rds/` checks exactly that against all eight captures, on every group of every one of them.

### One station never marks the end of its text

Radio text is up to 64 characters in segments of four. A text shorter than that is supposed to end with a terminator, character `0D`.

Fever FM 94.3 does not send one. It sends segments 0 to 3, `FEVER 94.3 FM   `, over and over, and segments 4 to 15 never arrive. A decoder that waits for the terminator shows that station no text at all, for ever, with no way for a person to tell that from a fault.

So the segment address going backwards is taken as the pass having finished, and the length is the highest segment that pass reached. Fever FM never sends past segment 3, so its text is sixteen characters.

The length has to come from the segment number rather than from how many characters arrived, and that is not a fine distinction. Taking it from the run of characters received, confirmed by two passes agreeing, was tried and it failed on the radio rather than in a test: on 93.5 at 20 dBuV, with block D of segment 0 failing while block C survived, the run stalled at two characters every pass, the two passes agreed, and the radio published `GA` as a whole song title. On a steady bad signal the same blocks fail every pass, so two passes agreeing proves nothing at all. A segment number only ever grows within one text, so damage can delay a text but cannot shorten it.

The other five stations all terminate properly, so the rule never comes into play on them:

| Station | Radio text |
|---|---|
| Radio City 91.1 | `RADIO CITY 91.1` |
| Red FM 93.5 | `REDFM VINANDI VINANDI ULLASANGA UTHSAAHANGAA` |
| Fever FM 94.3 | `FEVER 94.3 FM` |
| Mirchi 95 | `MIRCHI 95.0MHZ` |
| Radio Mirchi 98.3 | `MIRCHI  98.3MHz` |
| Magic FM 106.4 | A song title, filling all 64 characters |

### The text does change, but only on Magic FM

Across the six captures taken with the aerial out, not one character of the radio text ever changed and not one station toggled its A/B flag. Five send a fixed station identification, and Magic FM happened to be on one song throughout.

The longer weak capture on Magic FM does cover it. Over its 1390 groups the A/B flag toggles, 357 groups one way and 336 the other, and the station sends three different texts: a song, its own slogan, and the next song. That is the changing text path, on real data.

### No station lists an alternative frequency

Four of the six send group 0A, which is the group that carries them, but every code in it is `224` or `205`. Those mean "none follow" and "filler". So there is no local evidence for the alternative frequency decoding at all, and it is proved only by made up groups. `no_station_here_lists_an_alternative` records that rather than leaving an empty list looking like a decoder that failed.

The same goes for the clock: no station here sends group 4A.

### The programme types are mostly wrong at the transmitter

| Station | Programme type sent | What it is |
|---|---|---|
| Radio City 91.1 | 0 | None |
| Red FM 93.5 | 12 | Easy listening |
| Fever FM 94.3 | 0 | None |
| Mirchi 95 | 16 | Weather |
| Radio Mirchi 98.3 | 16 | Weather |
| Magic FM 106.4 | 12 | Easy listening |

Two music stations announce themselves as Weather. That is set at the station and there is nothing the radio can do about it. It is recorded here so that a future reading of "Weather" on Mirchi is not taken for a decoder bug.

### Nothing came back damaged with the aerial out

Not one block in any of the first six captures has an error code above 0. Every station that carries RDS here is strong, from 34 to 48 dBuV, and the tuner corrected nothing because there was nothing to correct.

Detuning by 100 kHz was tried on 93.5, 98.3 and 106.4 to produce a degraded stream, and it does not work: the tuner's decoder either locks cleanly or does not lock at all, and 100 kHz off a station it does not lock. There is no in between.

What does work is collapsing the whip. Every station drops from about 45 dBuV to about 10, and the two strongest RDS stations stay locked while the blocks start failing.

### Only a clean block can be trusted, and that is measured

With the aerial collapsed, the tuner reported these error levels:

| Capture | Groups | Clean | Corrected | Badly corrected | Not corrected |
|---|---|---|---|---|---|
| `fm-91100-weak` | 1387 | 5218 | 310 | 10 | 10 |
| `fm-106400-weak` | 1390 | 5106 | 425 | 13 | 16 |

Block A survived every single group of both, 1387 and 1390 of them. The chip protects the identifier hardest, which is why the identifier is the one field that never came out wrong here.

The first version of the decoder threw away only the blocks the tuner said it could not correct, and took the ones it said it had corrected. That is what the reference firmware does. Replaying these two captures through the three possible rules:

| Blocks accepted | Radio texts published on 106.4 |
|---|---|
| Clean, corrected and badly corrected | 11, four of them corrupt |
| Clean and corrected | 7, two of them corrupt |
| Clean only | 3, which is exactly what the station sent |

The corrupt ones were real and the radio published them as the text the station had sent:

```
RADIO CI4i#<8b>1.1
RADIO <c3>RTY 91.1
NAATHORA THAMASHALALO - NINNE Q]LLADATHA - SANJEEV WADHWANI + SU
MAwQCFM VINTOO MAIMARACHIPODAAM!
MAGICFM VINTOO MAIMARACHIPODAAM!                      @<10>
```

The last one is the worst kind: the terminator itself was corrupted, so the text ran on into the padding and picked up whatever was after it. Nothing about any of these looks wrong to the radio, and a person reading `MAwQCFM` has no way to know which letters are the station's.

`MAwQCFM` and the corrupted terminator both arrived in blocks the tuner reported as corrected at the **smallest** error it reports. So the correction is not reliable enough to decode from, and `core/rds.c` now takes clean blocks only.

What that costs is only speed, and only on a weak signal. On these two captures about a third of the name passes are lost, so a name takes about half as long again to appear. On any signal worth listening to every block is clean and it costs nothing at all.

The station name never came out corrupt under either rule, because the two matching passes rule caught it: `' MpÄIC  '` and `' MAGIC @'` each arrived once in the 106.4 capture and neither was ever repeated.

## How they were taken

`tools/rdscap.py`, which tunes over `POST /api/tune`, waits two seconds for the tuner and its decoder to settle, then polls `GET /api/rds/raw` once a second and writes every group it has not already seen.

The radio holds the last 128 groups. A station sends about 11.4 a second, so that is about eleven seconds, and polling once a second cannot miss any. None of these captures has a gap.

The ring is emptied on every retune, so a capture never holds the tail of the station before it. That was not true of the first version and it produced a wrong measurement straight away: Big FM 92.7 appeared to be sending RDS when what was in the buffer was Radio City's groups from the station tuned before it.

`tools/make_rds.py` turns the logs into `test/test_rds/captures.h`. Only the first 160 groups of each go in, and 400 of the two weak ones. Every field in every capture settles inside 55 groups, so 160 is well clear of what the tests need, and 400 is what it takes to include a block of every error level the tuner reports.

## Worth retaking

These are one afternoon in one room. Worth taking again from somewhere else or on a different day, and worth taking somewhere the signal is genuinely weak rather than deliberately weakened, because a collapsed aerial is not the same thing as distance: it attenuates evenly and adds no multipath and no interference from anything else.

Nothing here holds a station sending an alternative frequency list, a station sending the time, or a station whose identifier changes under the dial. All three are decoded and all three are proved only by made up groups.
