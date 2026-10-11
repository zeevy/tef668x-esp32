# HTTP API

Everything the radio's screen can do, a script can do over HTTP. Every answer says what the radio actually did, and a refusal says why in plain words. The [web page](Web-Page.md) uses this same API.

The radio answers at `http://tef668x.local:8080`, or at its address on your network. The examples set it once:

```bash
R=http://tef668x.local:8080
```

Every sample answer on this page was taken from a radio running the firmware this wiki describes. The network name and address in them are placeholders.

## Signing in

Reading needs no PIN, except `/api/settings`. Every change needs the access PIN: send it once to `/auth`, which answers with a session cookie, and send the cookie with each change.

```bash
curl -s -c jar -d pin=000000 $R/auth      # sign in, keep the cookie in the file jar
curl -s -b jar -d khz=102800 $R/api/tune  # use the cookie
```

```
HTTP/1.1 200 OK
Set-Cookie: tefsid=<32 hex>; Path=/; Max-Age=1800; HttpOnly; SameSite=Strict

Signed in.
```

| Rule | What it means |
|---|---|
| The PIN | Six digits, `000000` on a new radio. Change it on the web page or in the menu, see [First Start and Wi-Fi](First-Start-and-Wi-Fi.md#change-the-access-pin) |
| The session | Lasts 30 minutes from signing in. Using it does not make it longer |
| One at a time | Signing in again, from anywhere, ends the session before |
| It ends | When the radio restarts, the PIN changes, or the web server is turned off |
| Wrong PINs | After five wrong PINs, `/auth` is locked for 60 seconds, for everyone. A right PIN starts the count again |

A change without the cookie:

```
$ curl -s -d khz=106400 $R/api/tune
Enter the access PIN first.                     [403]
```

A wrong PIN, and the sixth wrong PIN in a row:

```
$ curl -s -d pin=111111 $R/auth
That PIN is not right.                          [403]
Too many wrong PINs. Try again in a minute.     [429, with Retry-After]
```

## How answers read

Most answers are one line of plain text, ending in a newline. The status code says how it went:

| Code | What it means |
|---|---|
| 200 | Done. The line says the state it reached, such as `FM 106.40 MHz` |
| 202 | The radio took it but has not confirmed it yet. Read `/api/state` to see where it got to. Do not send it again |
| 400 | Refused, and the line says why, for example `That is not a band. Use one of LW MW SW OIRT FM.` |
| 403 | Not signed in: `Enter the access PIN first.` |
| 404 | Under `/api/`, no such route or the wrong method: `Nothing on the API answers this. Check the path, and whether it takes GET or POST.` Also an empty preset or a catch no longer in the list. Any other unknown path, such as `/radio` from an older firmware, is sent to `/`, the web page |
| 409 | Not now. For a key or a touch: `The last key sent has not been handled yet. Send again.` For a touch also when Touch is Off, the screen is dark or held by another screen, the calibration screen is open, or a finger could not make that gesture there |
| 413 | A form or text body over 8192 bytes. A firmware upload is not limited by this |
| 429 | Too many wrong PINs |
| 500 | It could not be stored |
| 502 | `Not done: the tuner did not take it, and the radio is trying again.` |
| 503 | `The radio is busy. Try again in a moment.` Nothing was taken |

Fields are sent as form fields (`curl -d name=value`) or in the query string. A seek, a scan, a sweep and a Learn Local Stations pass answer at once and run on their own: read `/api/state` or `/api/dx` to follow them.

While a DX level sweep runs, about 4 seconds, the radio answers nothing. A request sent then is answered when the sweep ends.

## Reading the radio

### GET /api/state

Everything about the radio, as one JSON object. No PIN. `/status.json` is the same.

```bash
curl -s $R/api/state
```

<details><summary>A sample answer</summary>

```json
{"brd":"ats125","ver":"0.2.0","bid":"eab5c49+","slt":"app0","cnf":true,"upd":{"st":"none","ver":null,"sz":null},"net":"station","apn":null,"rssi":-57,"bars":2,"rst":"software (update)","ip":"192.168.1.40","dpn":true,"pcl":{"on":false,"cli":0,"ips":[],"ref":0},"liv":{"n":1,"ref":0,"max":313},"dx":{"on":false,"pg":1,"n":1,"rev":856561604,"swr":14769,"swp":false,"scn":"idle","sck":0,"scf":0,"scd":0,"sct":0,"dst":0,"dsc":0,"ddw":25,"wup":0,"wri":0,"wms":0},"scp":{"rev":2,"run":false},"hep":116204,"hmn":59988,"hmp":64580,"hlb":42996,"stk":{"rad":1392,"lop":7424},"up":4155,"slp":null,"pnl":{"lit":0,"dim":true,"sdb":14,"cur":null,"swp":47,"lvu":11760,"lvt":22036,"lvp":54,"lvb":9816,"lvm":16940,"psh":{"n":7,"px":4536,"us":4060,"max":591,"ref":5927}},"bat":{"fit":true,"mv":null,"boot":3724},"clk":{"syn":true,"age":4146,"now":"03:37"},"asv":{"n":0,"dif":false,"due":0,"bad":false,"idl":10000},"pst":{"n":9,"bad":false,"lost":0},"log":{"fit":true,"n":0},"inp":{"pad":true,"clk":0,"prs":0,"lst":"","lms":0,"typ":"","lns":65535,"pot":334,"pdb":-44,"tch":{"on":true,"pen":false,"dn":0,"rd":0,"ges":0,"chip":true,"cal":"stored","px":null,"py":null,"x":null,"y":null,"z1":null,"z2":null}},"tun":{"prt":"TEF6686","pch":102,"xad":0,"xtl":"9.216 MHz","fsi":false,"frd":false,"dr":false,"bnd":"FM","khz":106400,"f":"106.40","unt":"MHz","stp":100,"bws":0,"dxw":0,"vol":-44,"mut":false,"tmd":"Auto","pst":9,"seq":133684,"ims":true,"eq":true,"mno":false,"dem":50,"fnb":0,"anb":90,"ahc":47,"lhc":52,"asm":28,"lsm":34,"snr":15,"cut":0,"bld":0,"hbl":0,"wid":false,"skg":false,"skf":false,"bep":false,"sql":"Off","sqo":true,"hmu":false,"sig":135,"sav":141,"usn":93,"wam":186,"off":42,"bw":184,"mod":52,"st":true,"plt":true,"qst":1000,"agc":{"on":true,"gn":0,"avg":502,"set":true},"rds":{"syn":false,"pi":"1064","psp":"1064","pim":"match","pty":12,"ptn":"Easy Listening","tp":false,"ta":false,"ms":"speech","ps":" MAGIC  ","psh":" MAGIC  ","psm":255,"min":{"ms":56130,"grp":154,"blk":[[70,18,66],[5,85,64],[7,74,73],[3,80,71]],"typ":{"0A":3,"2A":2}},"grp":9833,"use":6687,"cor":9960,"bad":10540},"scn":false,"scd":0,"sct":0,"scf":0,"sca":0,"scr":0,"scc":null}}
```

</details>

Numbers are whole numbers. A value with a fraction is sent in tenths: `"sig":168` is 16.8 dBµV.

| Key | What it is |
|---|---|
| `brd`, `ver`, `bid` | The board, the firmware version, and the git commit it was built from, with `+` when built with changes not committed. `bid` is empty when the commit is not known |
| `slt`, `cnf` | The firmware slot it runs from, and whether that firmware is confirmed (false while on trial after an update) |
| `upd` | The update check: `st` is `off`, `wait`, `checking`, `none`, `found` or `failed`; `ver` and `sz` are the newer release's version and image size in bytes while `st` is `found`, and null otherwise |
| `net`, `rssi`, `bars`, `ip` | The network state (`station`, `joining`, `ap` for the hotspot, `offline`), the Wi-Fi signal in dBm and in bars (0 to 3, as the header draws it), the address. `rssi` and `bars` are null unless `net` is `station` |
| `apn` | The hotspot's own name, `tef668x-setup-XXXX`, while `net` is `ap`; null otherwise. The name of your own network is never in the state |
| `rst` | Why the radio last started, for example `power` or `software (update)` |
| `dpn` | True while the access PIN is still `000000` |
| `pcl` | The [PC Link](PC-Tools.md): `on` while port 7373 is open, `cli` how many PCs are signed in, `ips` their addresses, `ref` how many were turned away because three were in |
| `scp` | The [band scope](#band-scope)'s sweeps: `rev` moves with each new sweep, `run` is true while one is waiting or running. Read `GET /api/scope` again only when `rev` moves |
| `dx` | [DX mode](#dx-mode), as the first line of `GET /api/dx` has it: `on` whether it is open, `pg` the page the radio shows (0 DX, 1 Scope, 2 Scanner, 3 Catches), `n` how many catches and `rev` a number that moves when they change; `swr` the DX sweep's revision and `swp` true while one runs; the scanner's `scn` (`idle`, `running` or `stopped`), `sck` its channel in kHz (0 while idle), `scf` the stations found, and `scd` the channels done of `sct`; the scan settings `dst`, `dsc` and `ddw`, as [POST /api/settings](#post-apisettings) takes them; and the preset watch's last alert: `wup` the preset that came up, in kHz (0 for none), `wri` how far over its usual level, in tenths of a dB, and `wms` when, in ms since start |
| `liv` | The [live state](#get-apilive): `n` how many pages have it open, `ref` how many were turned away because two were open, `max` the longest one send of it held the radio since start, in ms |
| `hep`, `hmn`, `hmp`, `hlb` | Free memory now, the least since start, the least in the start before, and the largest free block, in bytes. `hmp` is there only when the radio restarted itself and noted it, as after an update. It is null after a power cycle or a crash |
| `stk` | Stack bytes each task has never reached since start: `rad` the radio task, `lop` the loop task |
| `up`, `slp` | Seconds since start, and seconds until Auto Off, or null when it is off |
| `pnl` | The screen, below |
| `bat` | The battery: `fit` true when the board can read it at all, `boot` the millivolts read at start up, `mv` the millivolts now (null while Wi-Fi is on, or when the reading failed), `last` the newest reading in millivolts (at start up, with Wi-Fi off, or with Wi-Fi paused by Battery Check) and `age` the seconds since it was taken, both null before any; `chk` the pauses for a reading: `n` how many, `ms` how long the last one held the radio's loop, `back` how long Wi-Fi took to be back after it, in milliseconds |
| `clk` | The clock: `syn` whether it has the time, `age` seconds since a time server last answered (null when none has), `now` the time as `HH:MM` (null when no server has answered) |
| `asv` | The automatic save of the settings: `n` saves written since start, `dif` true when what a save would store now is not what is stored, `due` ms until a save (0 when none is waiting), `bad` true when the last automatic write failed, `idl` the wait in use in ms (0 when the automatic save is off). It is not `sav` in `tun` |
| `pst` | The presets: `n` how many are stored, `rev` a number that changes whenever the list does (a rename or a move too, which leave `n` as it was), `bad` true when the last write to the list failed, `lost` how many start up threw away as unreadable |
| `log` | The station log: `fit` true when its flash partition mounted, `n` how many entries, `rev` a number that changes with every append, edit and delete, which a full log takes without `n` moving |
| `inp` | The controls, below |
| `tun` | The tuner and the station, below |

The `pnl` object:

| Key | What it is |
|---|---|
| `lit`, `dim` | The light in per cent, and whether it has dimmed after being left alone |
| `sdb` | The signal number the screen shows, in whole dBµV. The screen holds it still until the level moves a whole dB away |
| `cur` | The scope cursor in kHz while the panel shows DX mode's Scope page or the band scope. Null otherwise |
| `swp` | How long the last screen change took, in ms, 0 if none yet |
| `lvu`, `lvt`, `lvp` | LVGL's own memory: bytes in use, its size in bytes, and the per cent in use. `lvu` is null when LVGL is not running, and then `swp` and the other `lv` keys are left out |
| `lvb`, `lvm` | The largest free piece of LVGL's memory, and the most it has had in use since start, in bytes |
| `psh` | The pushes to the screen in the last whole second: `n` how many, `px` their pixels, `us` their time in all and `max` the longest, in µs, and `ref` the longest refresh, LVGL drawing and pushing together. Null before a whole second has passed |

The `inp` object:

| Key | What it is |
|---|---|
| `pad` | True when the keypad chip answered at start up |
| `clk`, `prs` | Knob clicks, and key and button events, since start |
| `lst`, `lms` | The last event in words, such as `BAND long`, and when it was, in ms since start (0 for never) |
| `typ` | Digits typed and not yet entered |
| `lns` | The keypad's sixteen raw lines, a 0 bit for each key held. 65535 until they have been read |
| `pot`, `pdb` | The volume knob's reading, 0 to 4095, and the volume in dB that reading gives |
| `pcl` | While the volume knob is being calibrated, its lowest and highest readings so far, as `min` and `max`. Not there otherwise |
| `tch` | The touch screen, below |

The `tch` object inside `inp` shows what the touch controller reads, to check the touch screen works. Outside the calibration screen a touch acts on the part of the screen it starts on, as [Touch Screen](Touch-Screen.md) describes, and beeps as Key Beeps says; `lst` then names it with that part, such as `touch tap band`:

| Key | What it is |
|---|---|
| `on` | True while the radio may read the touch screen: Controls > Touch is On, or the calibration screen is open. False otherwise. Then `pen` stays false and nothing else below changes. Even when it is true, the screen is not read while it is dark or during a level sweep: `pen` and `dn` still move then, the rest does not |
| `pen` | True while a finger or a pen is on the screen |
| `dn` | How many times the touch controller's pen line has gone low since start. A light touch can break contact and count more than once |
| `ges` | How many taps, holds, drags and swipes since start. A tap is counted a moment after the finger lifts, and a hold once it has been still for 1.5 s. Touches are not read while the panel is dark, and do not wake it |
| `chip` | True when the touch chip answered at start up with a reading of its own temperature, the boot screen's Touch row. False when it gave nothing, as a missing chip or a broken data line does |
| `rd` | How many readings have been taken since start. Readings are taken only while something is on the screen, up to 100 a second. A very short tap, made while the radio is busy redrawing, can be missed by `pen`, `dn` and `rd` alike |
| `x`, `y` | Where the last reading was, in the touch controller's own steps, 0 to 4095. These are not screen pixels: on the ATS-125, with Display Rotation at Normal, `x` grows down the screen and `y` grows from right to left |
| `cal` | `stored` when a calibration made on this radio is in use, `board` for the built-in map. See [Touch Screen](Touch-Screen.md) |
| `px`, `py` | Where the last touch landed, in screen pixels by that calibration, 0 to 319 across and 0 to 239 down. Null before the first touch |
| `z1`, `z2` | The two contact readings of the last reading, 0 to 4095. `z1` is near 0 with nothing on the screen. A finger pressed harder reads higher; a pen reads about the same however hard it is pressed |

`x`, `y`, `z1` and `z2` are null until the first reading. They keep the last reading after the finger lifts, and a reading taken just as a finger lifts can be far from where it was.

The `tun` object:

| Key | What it is |
|---|---|
| `prt`, `pch`, `xad`, `xtl` | The tuner chip, its patch, what the crystal sense pin read (raw), and the crystal that reading maps to |
| `fsi`, `frd`, `dr` | What the chip has: FM stereo improvement, full search RDS, and digital radio |
| `bnd`, `khz`, `f`, `unt` | The band, the frequency in kHz, and the frequency as the screen shows it, with its unit |
| `stp`, `bws`, `dxw` | The step in kHz, the filter width set (0 for automatic), and DX mode's width (0 while DX mode is closed) |
| `vol`, `mut`, `tmd`, `pst` | The volume in dB, mute as it was asked for, the tuning mode (`Manual`, `Auto`, `Presets`, `Meter band`), and the preset number (0 for none) |
| `mtb` | On SW, the metre band the dial is in, such as `22 m`, as the radio's header shows it. Left out on other bands and between two metre bands |
| `seq` | Goes up with every new reading the radio task makes. It stops when the radio task has stalled |
| `ims`, `eq`, `mno`, `dem` | iMS, the equaliser, forced mono, and de-emphasis in µs |
| `fnb`, `anb`, `ahc`, `lhc`, `asm`, `lsm` | The noise blankers in per cent, the AM high cut and soft mute levels in dBµV |
| `cut`, `bld`, `hbl` | The treble cut, the stereo blend and the combined blend the chip is applying now. They have no known unit. They are there only when the chip gave them |
| `wid` | The adaptive filter is allowed to open |
| `sig`, `sav` | The signal level now and its average, in tenths of a dBµV |
| `usn`, `wam` | Noise, and on FM multipath or on AM co-channel, in tenths of a per cent |
| `off`, `bw`, `mod` | The offset in tenths of a kHz, the filter width the tuner reports in kHz, and the modulation in per cent |
| `snr` | Signal to noise in dB, worked out by the firmware |
| `st`, `plt` | Stereo heard, and the stereo pilot |
| `qst` | The quality status word as the chip sent it. It carries the time stamp that says whether the readings have settled |
| `sql`, `sqo`, `sqa` | The squelch mode (`Off`, `Auto` or `Manual`), whether the squelch is open, and the Manual threshold in tenths of a dBµV. `sqa` is there only in Manual |
| `hmu` | The mute the tuner was told. `mut` is the mute that was asked for |
| `per` | What the tuner last refused, in words. There only when it refused something |
| `skg`, `skf` | A seek is running, and the last seek found a station |
| `bep` | A beep is sounding now |
| `agc` | The volume AGC: `on`, `gn` the gain now in dB, `avg` the running average of the modulation it works from in tenths of a per cent, and `set` whether that average has seen enough of this station to act on |
| `rds` | RDS, on FM only, below |
| `scn`, `scd`, `sct`, `scf`, `sca`, `scr`, `scc` | A station scan: running, channels done and in total, then stations found, stations added to a free slot, new stations with no free slot left, and whether it covered the whole band (false when a tune stopped it, null until a scan has finished) |

When the tuner did not start, `tun` holds why, so a fault can be read without a cable:

| Key | What it is |
|---|---|
| `err` | What stopped it, in words |
| `dev`, `hwd`, `swd` | The three identification words, in hex |
| `saw` | Something answered at the tuner's I2C address |
| `rbt`, `bot` | The operation status came back, and what it said. 0 means not patched yet |
| `ptd`, `try`, `wnt` | A patch was written in this start, which patch version was written, and which one the chip then asked for |
| `xad`, `xtl` | What the crystal sense pin read (raw), and the crystal that reading maps to |

The `rds` object. A field is there only once it has been received, so a name not heard yet is not the same as an empty name. With RDS switched off, the object is only `{"off":true}`.

| Key | What it is |
|---|---|
| `off` | True when RDS is switched off. Nothing else is there then |
| `syn` | The tuner is locked to an RDS bit stream |
| `pi`, `piz` | The programme identifier, four hex digits, and `piz` true when the station sends 0000, which names no station |
| `pih` | The PI last heard but not yet sure, with `?` for each digit that changed, such as `5?A1`. Left out once it is the answer |
| `ecc`, `cty` | The extended country code, two hex digits, and the station's country, two letters, from the ECC and the PI |
| `cs`, `csg` | North American call letters, from RT+ or worked out from the PI, and `csg` true when they were worked out from the PI and are a guess |
| `psp`, `pim` | The PI stored with the tuned preset, four hex digits, and `match` when this is that preset's station, `other` when not |
| `blk` | The error level of blocks A to D of the last group: 0 clean, 1 or 2 corrected, 3 not corrected |
| `pty`, `ptn` | The programme type, 0 to 31, and its name in the RDS region the settings name |
| `tp`, `ta` | The station carries traffic announcements at some time, and one is on air now |
| `ms` | `speech` or `music` |
| `ps` | The station name, the eight characters as they were sent |
| `psh`, `psm` | The station name as it builds up, eight characters, and which of its positions have arrived, one bit each, the lowest bit for the first. 255 when all eight have arrived |
| `psl` | The name stitched back together, when the station splits one across two passes. The radio's screen shows this in place of `ps` |
| `rt` | The radio text |
| `af` | The alternative frequencies, in kHz |
| `ct` | The date and time the station sent, in its own local time, with its offset from UTC, such as `2026-10-11T14:05+02:00` |
| `lng`, `lgn` | The programme language code, a number, and the name of that language when it is known |
| `rtp` | RT+: `on` the station sends it, `run` an item is playing, and `tags`, each with its type `t`, its label `l` and its words `s` (null when `rt` has no words for it) |
| `eon`, `eom` | Other networks heard in two groups or more, each with `pi`, `ps`, `tp`, `ta` (each of the last three null until heard), `af` in kHz, and `afm` true when it had more frequencies than were kept. `eom` is true when another network was named after the list was full |
| `min` | The last minute: `ms` the time it covers, `grp` the groups, `blk` the clean, corrected and lost counts of blocks A to D, and `typ` the count of each group type, such as `"0A"` |
| `grp`, `use` | Groups the tuner has handed over on this station, and how many of them something was decoded from |
| `cor`, `bad` | Blocks the tuner corrected, only counted and not used, and blocks it could not correct |

### GET /api/live

The same JSON as `GET /api/state`, sent to you twice a second over a WebSocket, so a page that shows the radio live does not have to keep asking. No PIN, as for `/api/state`. The web page uses it.

Asking for `/api/state` costs the radio a whole request each time, and a request holds it for long enough that once a second makes its own screen stutter. Over the socket one send holds it for about 10 ms.

- At most two pages at once: a third is answered 503 and should try again later.
- It only sends. Changes still go to their own routes, such as `POST /api/volume`, with the PIN.
- Nothing is sent while a level sweep reads the band, up to 7.5 s on MW. If nothing comes for 10 s, the radio has gone: it restarted, or left the Wi-Fi.
- Switching the web server off closes it.

```js
const ws = new WebSocket('ws://tef668x.local:8080/api/live');
ws.onmessage = (e) => console.log(JSON.parse(e.data).tun.f);
```

### GET /api/settings

The stored settings, as JSON. Needs the PIN.

```bash
curl -s -b jar $R/api/settings
```

```json
{"sid":"MyNetwork","pss":true,"dpn":true,"ldd":true,"sql":"Off","sbd":4,"sfq":106400,"svl":-20,"ims":1,"eq":1,"mno":0,"cut":0,"bld":0,"hbl":0,"fnb":0,"anb":100,"ahc":47,"lhc":52,"asm":28,"lsm":34,"dem":50,"abw":4,"tzo":"+05:30","rgn":3,"spc":0,"enc":0,"edr":0,"fsn":4,"asn":4,"smu":120,"bpk":3,"bpe":1,"bps":1,"sqf":12,"blt":90,"bdm":10,"bds":90,"rds":1,"ntp":1,"bat":1,"agt":70,"agb":0,"thm":10,"thn":0,"rot":0,"dst":0,"dsc":0,"dmf":50,"dml":99,"dlp":0,"dmu":0,"dal":1,"ddw":25,"dbw":114,"rrg":0,"drt":1,"dwt":1,"hsp":0,"web":1,"wif":1,"slp":0,"upc":1,"tof":0,"kpt":15,"pcl":0,"cst":["#141415","#09121a","#1b2a36","#d3d9de","#b78624","#0d567d","#e51f33","#270602","#ebe5e5"]}
```

Most keys are the same as for [POST /api/settings](#post-apisettings) below. `sid` is the network name and `pss` says whether a passphrase is stored; the passphrase itself is never sent. `tzo` is the UTC offset and `cst` the custom theme's colours. The rest (`sql`, `sbd`, `sfq`, `svl`, `ims` and the other reception keys, `abw`) are the stored radio settings, which are changed with the tuning and sound routes and kept with `/api/save`.

### GET /api/screen

What the screen shows now, one JSON object per line. The first line names the screen, the page and the theme. Then, on a screen a touch acts on, each part a touch acts on, as `zone` with its place and size, see [Touch](#touch). Then each text, with its place, colour and font, and each box.

```
{"screen":"radio","page":0,"dim":false,"theme":"Nightwatch"}
{"zone":"menu","x":0,"y":0,"w":38,"h":28}
{"zone":"band","x":38,"y":0,"w":42,"h":28}
{"zone":"name","x":0,"y":32,"w":320,"h":34}
{"zone":"freq","x":0,"y":66,"w":320,"h":54}
{"zone":"scale","x":0,"y":144,"w":320,"h":56}
{"zone":"mode","x":0,"y":204,"w":84,"h":28}
{"zone":"sql","x":84,"y":204,"w":76,"h":28}
{"zone":"bw","x":160,"y":204,"w":76,"h":28}
{"zone":"vol","x":236,"y":204,"w":84,"h":28}
{"x":0,"y":0,"w":320,"h":240,"fill":"#080C10","fillRole":"ground|header"}
{"x":36,"y":3,"w":25,"h":23,"c":"#F8B000","role":"radio","font":"title","text":"FM"}
{"x":248,"y":7,"w":16,"h":15,"c":"#8894A0","role":"dead","font":"icons","text":""}
{"x":248,"y":7,"w":16,"h":15,"c":"#30C098","role":"good","font":"icons","text":""}
{"x":214,"y":8,"w":24,"h":12,"border":1,"borderC":"#8894A0","borderRole":"dead"}
{"x":216,"y":10,"w":11,"h":8,"fill":"#30C098","fillRole":"good"}
{"x":238,"y":11,"w":2,"h":6,"fill":"#8894A0","fillRole":"dead"}
{"x":272,"y":7,"w":36,"h":17,"c":"#E0E4E0","role":"measurement","font":"small","text":"20:29"}
{"x":12,"y":7,"w":16,"h":15,"c":"#8894A0","role":"dead","font":"icons","text":""}
{"x":12,"y":32,"w":296,"h":88,"fill":"#F8B000","fillRole":"radio"}
...
```

No PIN.

## Tuning

All need the PIN.

| Route | Fields | Sample answer |
|---|---|---|
| `POST /api/tune` | `khz`, the frequency in kHz | `FM 106.40 MHz` |
| `POST /api/step` | `stp`, steps up (positive) or down (negative) | `FM 106.50 MHz` |
| `POST /api/band` | `bnd`: `LW`, `MW`, `SW`, `OIRT` or `FM` | `FM 106.40 MHz` |
| `POST /api/seek` | `dir`: `up` or `down` | `seeking up` |
| `POST /api/mode` | `mod`: `Manual`, `Auto`, `Presets` or `MeterBand` | `mode Manual`. MeterBand answers `mode Meter band` |
| `POST /api/step-size` | `khz`, a step the band offers. `bnd` sets another band's step | `step 100 kHz` |
| `POST /api/cycle` | `wht`: `band`, `bandwidth`, `mode`, `mute` or `features`. Steps it on: band, width and mode as their buttons do, mute on and off, and `features` through off, iMS, EQ and both | `mode Presets` |

```bash
curl -s -b jar -d khz=106400 $R/api/tune
curl -s -b jar -d stp=1 $R/api/step
curl -s -b jar -d bnd=FM $R/api/band
curl -s -b jar -d dir=up $R/api/seek
```

In tuning mode Auto, `/api/step` seeks, as the knob does. Set `mod=Manual` first to step.

Refusals, as the radio sent them:

```
That frequency is in no band.                         [400, khz=200000]
That is not a band. Use one of LW MW SW OIRT FM.      [400, bnd=AIR]
That is not a direction. Use up or down.              [400, dir=sideways]
```

## Sound

All need the PIN.

| Route | Fields | Sample answer |
|---|---|---|
| `POST /api/volume` | `db`, -60 to 24. The next turn of the volume knob replaces it | `volume -18 dB` |
| `POST /api/mute` | `on`: `1` or `0` | `muted`, `unmuted` |
| `POST /api/bandwidth` | `khz`, a width the band offers, `0` for automatic on FM and OIRT. `bnd` sets another band's width | `bandwidth automatic` |
| `POST /api/squelch` | `mod`: `off`, `auto` or `manual`. The Manual level comes from the volume knob only | `squelch Off` |
| `POST /api/fm` | Any of the reception settings below | The whole reception state, below |
| `POST /api/beep` | `ms`, 1 to 3000, and `hz`, 100 to 15000 (2000 if left out) | `beeping for 150 ms` |

`GET /api/bands` needs no PIN. It gives each band of the band plan: `band`, its edges `low` and `high` in kHz, the `steps` and filter `widths` it offers (0 on FM is automatic), and the `step` and `width` it has now, the live ones for the band tuned. `POST /api/step-size` and `POST /api/bandwidth` with `bnd` change another band's.

```
$ curl -s $R/api/bands
[{"band":"LW","low":144,"high":513,"steps":[1,9],"step":9,"widths":[3,4,6,8],"width":4},...]
```

`/api/fm` takes `ims`, `eq` and `mno` (0 or 1, FM only), `cut`, `bld` and `hbl` (0, or 20 to 60 dBµV, FM only), `fnb` and `anb` (0, or 50 to 150 %), `ahc` and `lhc` (0, or 20 to 60 dBµV), `asm` and `lsm` (0 to 50 dBµV), and `dem` (0, 50 or 75). It answers with every setting:

```
$ curl -s -b jar -d ims=1 -d eq=1 $R/api/fm
iMS on, EQ on, stereo, weak signal cut 0 blend 0 hiblend 0, blanker am 100 fm 0, am high cut 47 lw 52, am soft mute 28 lw 34, de-emphasis 50 us
```

```
The radio refused it: that bandwidth is not allowed here     [400, khz=99]
That is not a squelch mode. Use off, auto or manual.         [400, mod=foo]
```

## Keys

`POST /api/key` presses a control as a person would. It needs the PIN.

| Field | Values |
|---|---|
| `k` | `BAND`, `BW`, `MODE`, `PUSH` (the tuning knob's press), `ENTER`, `DX`, or a digit `0` to `9` |
| `e` | `short` or `long`. `DX` and the digits have no long press |
| `turn` | Turns the tuning knob by -20 to 20 clicks, not 0, in place of `k` |

```
$ curl -s -b jar -d k=BAND -d e=long $R/api/key
pressed BAND long
$ curl -s -b jar -d k=DX -d e=long $R/api/key
That key has no long press.                                   [400]
$ curl -s -b jar -d k=FOO $R/api/key
Give k, one of BAND BW MODE PUSH ENTER DX 0 to 9, or turn.    [400]
```

Only one key can wait at a time. A key sent before the one before it was handled gets 409: send it again. Read `/api/screen` to see the result of a key.

## Touch

`POST /api/touch` makes a gesture on the screen as a finger would. It needs the PIN. It acts on the part of the screen where it begins, as [Touch Screen](Touch-Screen.md#the-radio-screen) describes, and makes the same checks as a finger: it stops a running DX scan or clears a number being typed and does nothing else, and beeps as Key Beeps says. `GET /api/screen` lists the parts of the screen on show that a touch acts on, and lists none while a touch does nothing.

| Field | Values |
|---|---|
| `x` | Across, in screen pixels, 0 to 319 |
| `y` | Down, in screen pixels, 0 to 239 |
| `g` | `tap`, `hold`, `swipe-left`, `swipe-right`, `swipe-up`, `swipe-down` or `drag`. A swipe starts at `x`, `y`. On the value editor's minus and plus, where a resting finger repeats, `hold` steps once |
| `x2`, `y2` | For `drag` only: where it ends. The scale moves by the distance from `x` to `x2` |

```
$ curl -s -b jar -d x=160 -d y=120 -d g=tap $R/api/touch
touched tap at 160,120
$ curl -s -b jar -d x=160 -d y=170 -d x2=136 -d y2=170 -d g=drag $R/api/touch
touched drag at 160,170
$ curl -s -b jar -d x=160 -d y=120 -d g=poke $R/api/touch
Give g, one of tap hold swipe-left swipe-right swipe-up swipe-down drag.   [400]
```

It waits in the same one place as a key, so it gets 409 while a key or a touch sent before it is still waiting. It also gets 409, with the reason, wherever a finger on the screen would do nothing: with Controls > Touch Off; while the screen is dark, starting up, going to sleep or held by an update; while the calibration screen is open; and for a gesture a finger could not make there: a drag that starts off the scale or moves 16 pixels or less, which a finger makes a tap, or a swipe that starts on the scale, which a finger makes a drag. `inp.lst` in `/api/state` then says what it did: `api touch tap` and the like, or `api DX scan stopped` or `api typed number cleared` when that is all it did. It is not counted in `ges`, which counts what the screen itself felt.

## Presets

See [Presets and Station Scan](Presets-and-Station-Scan.md) for the CSV format.

| Route | PIN | Fields | Sample answer |
|---|---|---|---|
| `POST /api/presets` | Yes | `do=set` with `slot`, `khz`, and `bw` and `name` if wanted | `Preset 50 is FM 98.30 MHz` |
| `POST /api/presets` | Yes | `do=add` with `khz`, and `bw` and `name` if wanted. The radio stores it in the lowest free slot, and refuses a station that is a preset already | `Preset 6 is FM 98.30 MHz` |
| `POST /api/presets` | Yes | `do=recall` with `slot` | `FM 98.30 MHz` |
| `POST /api/presets` | Yes | `do=move` with `slot` and `to`. The two swap when `to` holds a preset | `Preset 3 moved to 10.`, `Presets 3 and 10 swapped.` |
| `POST /api/presets` | Yes | `do=clear` with `slot` | `Preset 10 deleted.` |
| `GET /api/presets.csv` | No | | The CSV file |
| `POST /api/presets/import` | Yes | `mode=merge` or `mode=replace` in the query, the CSV as the body, `Content-Type: text/csv` | `16 of 16 lines imported, 0 refused, 0 already taken, 0 names cut short` |
| `POST /api/scan` | Yes | `bnd`: `FM`, `MW`, `SW` or `LW`, FM if left out | `scanning FM` |

```
$ curl -s $R/api/presets.csv
slot,band,frequency,bandwidth,name,pi
1,FM,91100,0,,3712
2,FM,92700,0,,
3,FM,93500,0,,0935
...
```

```
Preset 99 is empty.                                                   [404, do=recall slot=99]
do has to be set, add, recall, clear or move.                         [400, do=erase]
98.30 MHz is already preset 5.                                        [409, do=add khz=98300]
Every preset slot is taken.                                           [409, do=add with all 99 slots used]
0 of 1 lines imported, 0 refused, 1 already taken, 0 names cut short  [200, merge into a used slot]
Line 1 could not be read, so nothing was changed.                     [400, replace with a bad file]
Give bnd, one of FM MW SW LW, or leave it out for FM.                 [400, scan bnd=OIRT]
```

A `replace` with only the header line clears them all.

## Station log

See [Station Log](Station-Log.md).

| Route | PIN | What it does |
|---|---|---|
| `GET /api/log` | No | The log, one JSON object per line, oldest first, after a first line with the count. `last=N` sends only the newest N; the count is still the whole log's |
| `GET /api/log.csv` | No | The log as a CSV file |
| `POST /api/log` | Yes | Logs the station the radio is on. Answers `logged`, or 409 when the station is already logged or the radio is still tuning |
| `POST /api/log` | Yes | `do=edit` with `id`, `time`, `khz` and `name`, `note` or both: changes the entry's name (up to 8 characters) and note (up to 32), plain characters only. A name that differs from the one stored marks the entry `renamed`. The readings are never changed. `Entry changed.` |
| `POST /api/log` | Yes | `do=delete` with `id`, `time` and `khz`: deletes the entry, frees its place, and its station can be logged again. `Entry deleted.` |

`id`, `time` and `khz` are the entry's own, as `GET /api/log` gives them. When the log has moved on since it was read, so that `id` holds another entry, the answer is 409 and nothing changes. A delete gives every later entry a new `id`, so read the log again after one. Each change takes about a second, as the log's file is written again from the changed entry on; the radio's screen waits for it.

```
$ curl -s $R/api/log
{"n":75}
{"id":0,"time":1789662597,"real":true,"band":"FM","khz":102800,"level_dbuv":342,"usn":8,"multipath":30,"cochannel":0,"snr":25,"stereo":true,"bw_khz":184,"name":null,"pi":null,"rt":null,"note":null,"renamed":false}
...
$ curl -s $R/api/log.csv
time,real,band,khz,level_dbuv,usn,multipath,cochannel,snr,stereo,bandwidth_khz,name,pi,rt,note,renamed
2026-09-17 21:59,1,FM,102800,342,8,30,0,25,1,184,,,,,0
...
```

`id` is the entry's place in the log, which it keeps until 250 newer ones have pushed it out. `time` in the JSON is seconds since 1970 in UTC, or milliseconds since start when `real` is false, which says whether the clock was set. `note` is your own note, null for none. `level_dbuv`, `usn`, `multipath` and `cochannel` are in tenths.

## DX mode

See [DX Mode](DX-Mode.md).

| Route | PIN | Fields | Sample answer |
|---|---|---|---|
| `POST /api/dx` | Yes | `on`: `1` or `0`. `khz`: DX mode's width. `scan`: `1` or `0` | `DX on, 114 kHz`, `DX off` |
| `POST /api/dx` | Yes | `sweep=1`, alone, while DX mode is open | `sweeping, GET /api/dx/sweep has it in about 4 s` |
| `POST /api/dx` | Yes | `baseline`: `now` keeps the last sweep as the baseline, `1` to `7` keeps that kept sweep, as `kept` in `GET /api/dx/sweep` lists them, `auto` goes back to the median | `baseline fixed on kept sweep 3` |
| `POST /api/dx` | Yes | `learn=1` alone: Learn Local Stations. `log` (the place in the list) or `logid` (a catch's `id`): log that catch | |
| `GET /api/dx` | No | | DX mode's state, then one line per catch |
| `GET /api/dx/sweep` | No | | The last sweep, with the level of every channel |
| `GET /api/dx.csv` | No | | The catches as a CSV file for FMLIST |

```
$ curl -s $R/api/dx
{"on":true,"page":0,"cursor":0,"n":1,"dropped":0,"sweeping":false,"sweep_rev":10597,"scan":"idle","scan_khz":0,"scan_found":0,"scan_passed":0,"scan_total":0,"watch":true,"watch_n":0,"watch_up":null}
{"i":0,"id":17592,"time":1791130887,"real":true,"band":"FM","khz":106400,"pi":"1064","name":" MAGIC  ","country":null,"new":false,"count":1,"level_dbuv":364,"logged":false,"due":true}
```

```
$ curl -s $R/api/dx/sweep
{"rev":34520,"running":false,"abandoned":false,"time":1791613955,"real":true,"took_ms":3278,"width":114,"low":87000,"step":100,"count":211,"floor":19,"baseline":"fixed","baseline_sweeps":1,"level":[86,71,77,...],...,"kept":[1791613910,1791613668,1791613663,1791613645,1791605822,1791600852,1791600845],"baseline_kept":5}
```

The sweep's `level`, `baseline_level`, `rise` and `peak` lists have one value per channel from `low` in steps of `step` kHz, in tenths of a dBµV, with null for no reading. `floor`, the level a quarter of the channels are below, is in tenths too. `kept` lists when each kept sweep but the latest was taken, in UTC seconds, newest first, null for one taken while the clock was not set. `baseline_kept` is the kept sweep fixed as the baseline, counted from 1, `0` for the median and `-1` for a fixed sweep that is not in `kept`: the latest, or one no longer kept.

### Band scope

The sweep of the [band scope](Band-Scope.md), outside DX mode, of the whole band the radio is on or of a span round the dial: FM and OIRT through DX mode's width, AM through the radio's own AM width, each AM channel read 40 ms after its tune. A PC's [spectral scan](PC-Tools.md#spectral-scan) is the same sweep over the range the PC asks for, FM through the width it asks for, and shows here too. The sound is muted while it runs, about 4 s for the whole FM band, 7.5 s for MW and under a second for a 3.6 MHz span on FM, and the web server answers nothing until it ends. The whole of SW is more channels than a sweep holds, so on SW the whole band is the metre band the dial is in, such as the 22 m band, 13570 to 13870 kHz in 61 channels. Between two metre bands, ask for a span.

| Route | PIN | Fields | Sample answer |
|---|---|---|---|
| `POST /api/scope` | Yes | `sweep=1`, and `span` in kHz: left out or `0` for the whole band, else that much round the dial, kept inside the band. 400 for more than 431 channels: on SW the whole band between two metre bands, or a span over 2150 kHz | `sweeping` |
| `GET /api/scope` | No | | The last sweep: `rev`, `running`, `abandoned` (the last one asked for ended early and this is the one before it), `time`, `real`, `took_ms`, `width`, `low`, `step`, `count`, `whole`, `floor` (the whole band only), `band` (the band it was swept on) and `level`, as above |

```
$ curl -s -OJ $R/api/dx.csv
Date,Time,Frequency,PI,Signal,Stereo,TA,TP,PTY,ECC,PS,Radiotext
04-10-2026,16:21:26,106.40 MHz,1064,36.4 dBμV,•, , ,12,--, MAGIC  ,
```

## Settings

### POST /api/settings

Changes stored settings. Needs the PIN. Send any mix of the keys:

```
$ curl -s -b jar -d blt=90 $R/api/settings
Saved and in use now.
$ curl -s -b jar -d rot=90 $R/api/settings
The rotation is 0 or 180.                                   [400]
```

A setting read at start up answers `Read at start up, so reboot for that to take effect.`

| Key | Values | What it is |
|---|---|---|
| `sid`, `pwd` | Text | The Wi-Fi network name and passphrase. The radio moves to that network at once |
| `pin` | Six digits | A new access PIN. It ends the session |
| `tzo` | `+05:30` style, `-12:00` to `+14:00` | The clock's offset from UTC |
| `ntp` | 0, 1 | Network time |
| `rgn` | 0 to 4 | FM band plan: Full, Japan, Wide, 87-108 MHz, Worldwide. After a restart |
| `spc` | 0, 1 | MW step 9 or 10 kHz. After a restart |
| `fsn`, `asn` | 1 to 6 | Seek sensitivity on FM and AM |
| `sqf` | 0 to 40 | Squelch floor in dBµV, 0 for off |
| `agt`, `agb` | 0 or 30 to 80; 0 to 8 | Volume AGC target in per cent (0 for off), and boost in dB |
| `smu` | 0 to 500 | Mute ramp in ms |
| `bpk`, `bpe`, `bps` | 0 to 3; 0, 1; 0, 1 | Key beeps, band edge beep, start chime (after a restart) |
| `blt`, `bdm`, `bds` | 5 to 100; 0 to 100; 0 to 240 | Brightness, dim level, seconds before dimming (0 never) |
| `thm`, `thn` | 0 to 15 | Day and night theme: 0 Nightwatch, 1 Daylight, 2 Red Night, 3 Phosphor, 4 Clear, 5 Custom, 6 Slate, 7 Paper, 8 LCD, 9 Ember, 10 Clear Day, 11 High Contrast, 12 Mono, 13 Hi-Fi, 14 Violet, 15 Blossom |
| `tc0` to `tc8` | `#rrggbb` | The custom theme's colours |
| `rot` | 0, 180 | Screen rotation |
| `bat` | 0 to 2 | Battery mark: off, per cent, volts |
| `bck` | 0 or 1 | Battery Check: 1 reads the battery every 5 minutes with Wi-Fi stopped for a moment, while nobody uses the radio |
| `rds`, `rrg` | 0, 1 | RDS decoder; RDS region Europe or North America |
| `hsp` | 0 to 2 | Hotspot: Auto, On, Off |
| `web`, `wif` | 0, 1 | Web server and Wi-Fi. 0 can only be undone from the radio's menu, or by Erase Settings on the recovery screen |
| `slp` | 0 to 600 | Auto Off in minutes, 0 for off |
| `upc` | 0, 1 | Check for Updates. Turned on, the radio looks in this start too |
| `tof` | 0, 1 | Touch off: 1 stops the radio reading the touch screen, 0 reads it. Controls > Touch shows 0 as On |
| `kpt` | 5 to 60 | Keypad Timeout: seconds a part typed number waits for the next key |
| `pcl` | 0, 1 | PC Link: 1 opens port 7373 for XDR-GTK and FM-DX Webserver |
| `enc`, `edr` | 0, 1 | Encoder type and direction. After a restart |
| `dst`, `dsc` | 0 to 2 | DX stop condition; DX scan range |
| `dmf`, `dml` | 1 to 99 | DX preset range |
| `ddw` | 5 to 300 | DX dwell in tenths of a second |
| `dbw` | An FM width | DX scan bandwidth in kHz |
| `dlp`, `dmu`, `dal`, `drt`, `dwt` | 0, 1 | Loop band, mute during scan, auto-log, log radio text, watch presets |

### POST /api/save

Stores the station and the radio's settings now, so the radio starts this way. Needs the PIN.

The radio also does this by itself, 10 s after the last change, but not while a seek or a scan runs. So this route is needed only to save at once.

```
$ curl -s -b jar -X POST $R/api/save
Saved. It will come up on 106.40 MHz, squelch Off.
```

## System

| Route | PIN | What it does |
|---|---|---|
| `GET /` | No | The [web page](Web-Page.md), one file, gzipped, with an `ETag` and `Cache-Control: no-cache`. A browser that sends the same ETag in `If-None-Match` gets 304 with no body, so the page is downloaded again only when a new firmware brings a new page |
| `POST /update` | Yes | Installs a firmware, sent as the form field `firmware`. See [Updating](Updating.md) |
| `POST /update/install` | Yes | Installs the newer release the update check found, from GitHub. Answers 202, then downloads and writes it and restarts into it. A failed download or sha256 leaves the old firmware running, with the update still offered. Answers 409 when nothing newer was found, while a new firmware is on trial, or during a station scan |
| `POST /reboot` | Yes | Restarts the radio. Refused with 409 while a new firmware is on trial |
| `POST /api/sleep` | Yes | Puts the radio to sleep. Answers `Going to sleep. Press the knob to wake the radio.` Refused with 409 while a new firmware is written or on trial |
| `POST /setpin` | Yes | Sets a new PIN, field `pin`. Ends the session |
| `POST /wifi` | Yes, except on the setup hotspot | Saves a network, fields `sid` and `pwd` |

`/auth`, `/update`, `/update/install`, `/reboot`, `/setpin` and `/wifi` answer as the API does: one line of plain text, with the status code saying how it went. A right PIN on `/auth` gets 200 `Signed in.` with the session cookie, and a call without a session gets 403 `Enter the access PIN first.`

## For measuring

These are used to measure the radio and are not needed for everyday use: `POST /api/seek/settle` (tune, wait and read the signal several times), `GET` and `POST /api/afcheck` (read one channel over and over), `POST /api/pot` (teach the radio the ends of the volume knob), `GET /api/dx/timing.csv` (how long RDS took to lock on each DX scan channel), and `GET /api/rds/raw` (the last raw RDS groups):

```
$ curl -s $R/api/rds/raw
# khz=106400 total=6288 held=128 lost=0 stat=8200 read=1
6160 1064 2190 4B41 5348 00
6161 1064 0180 E0CD 204D 00
...
```
