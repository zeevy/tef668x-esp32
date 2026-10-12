# Station Log

The station log is a list of the stations you have heard, written by the radio when you ask. It is separate from the [presets](Presets-and-Station-Scan.md). It keeps the time, the frequency, the signal readings and the RDS details of each.

## Log a station

On the radio screen, hold the tuning knob down. After 600 ms the radio writes the station and says what happened on the name line for 1.5 seconds:

| Message | What it means |
|---|---|
| `Logged 106.40` | The station was written, with its frequency |
| `Already Logged` | The log already has this station: the same band and frequency, and the same PI when both have one |
| `Still tuning - wait` | A seek, a station scan or a DX scan is running. Try again when it stops |
| `Not logged` | The radio could not read the station, or could not write the log file |

Other ways to log:

- **Hold ENTER**, on the radio screen, the RDS pages and the DX pages. On the RDS and DX pages the message shows in the header.
- **Menu > Go To > Log Current Station.** It closes the menu and logs.
- **DX mode** can log new stations by itself. See below.
- **Log this station** on the web page's [Station log](Web-Page.md#station-log) screen.

## What is kept

| Item | What it is |
|---|---|
| Time | The date and time from the network clock. If the radio has no time yet, the time since it started |
| Band and frequency | |
| Signal | The level in dBµV, USN (noise), multipath, co-channel and SNR, the filter width, and stereo |
| Name and PI | The station's RDS name and PI, only when they belong to that channel |
| Programme type and country | The RDS programme type, such as Pop Music, and the extended country code (ECC), only when they belong to that channel. With the PI, the ECC names the country |
| Radio text | When DX Scanner > Log Radio Text is On, which it is at the start |
| Note | Your own note, up to 32 characters, written on the web page |

The log holds 250 stations. When it is full, the oldest one is dropped to make room, so a new one is never refused.

On the web page's [Station log](Web-Page.md#station-log) screen a station can be renamed (up to 8 characters, as RDS names are), given a note, or deleted. The readings are never changed. A deleted station can be logged again. Logging, renaming or deleting takes about a second; the radio's screen waits for it, the sound does not. On the radio itself a station can be deleted from its card, below; there is no way to clear the whole log.

## View it on the radio

Open the menu and go to **Stations > Station Log**. The newest station is first. Each row shows the RDS name, or `PI` and the code when there was no name, or the band, and the frequency. Press a row to open its card.

![The Station Log in the menu](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/menu-station-log.png)

### An entry's card

![A station's card](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/menu-station-entry.png)

The card shows everything the log kept for the station:

- The frequency and the name as its title.
- Under it, when it was logged (such as `Today 17:08`, or the day and time while the radio's clock is not set, or `Time not known` for a station logged before the clock was ever set), the PI, the country and the programme type, each when the log has it.
- The readings as tiles: Level, SNR, Noise, Multipath, Stereo and Width on FM; Level, SNR, Co-channel and Width on AM.
- The radio text, and your note.

![An entry on shortwave](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/menu-station-am.png)

Two buttons sit at the foot. Turn the knob to move between them, and press it to act:

- **Tune** tunes to the station and leaves the card open, so you can hear it. A station whose frequency is not on the band plan in use shows `Not on the band plan` and tunes nothing.
- **Delete** deletes the entry. The first press turns the button red and says `Press again`; a second press within 5 seconds deletes it and goes back to the list. Turning the knob off it, going back or waiting 5 seconds lets it go. The last entry deleted takes you back to **Stations**.

![Delete waiting for its second press](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/menu-station-delete.png)

If the entry was changed or deleted on the web page while its card was open, the card says `Changed since this opened` and does nothing; go back and open it again. If the radio cannot write the delete, it says `Not deleted`. MODE, or a hold of the knob, goes back to the list, and so does a tap on the title while Touch is On.

## Export it

As a CSV file for a spreadsheet, with **Download** on the web page's Station log screen, or:

```bash
curl -s -o logbook.csv http://tef668x.local:8080/api/log.csv
```

The columns are `time,real,band,khz,level_dbuv,usn,multipath,cochannel,snr,stereo,bandwidth_khz,name,pi,rt,note,renamed,pty,country`. `renamed` is 1 when the name was typed on the web page, not received. `pty` is the programme type's number, 1 to 31. `country` is the two letter ISO code, such as `IN`, from the PI and the ECC together. Each is empty when it was not received. A country is never worked out from the PI alone, since one PI digit is shared by many countries. The time is written as `YYYY-MM-DD HH:MM` at the clock's UTC offset, or as the time since start, such as `+12345ms`, when the radio had no time.

`level_dbuv`, `usn`, `multipath` and `cochannel` are in tenths: `452` means 45.2 dBµV.

As JSON, one station per line, oldest first, after a first line with the count:

```bash
curl -s http://tef668x.local:8080/api/log
```

Both need no PIN, so anyone on the network can read the notes. To log the station the radio is on from a script, `POST /api/log` with the access PIN. It answers 409 when the station is already logged or the radio is still tuning.

## Logging from [DX mode](DX-Mode.md)

With **DX Scanner > Auto-Log Stations** On, which it is at the start, DX mode writes each new station to the log by itself: one whose PI it has never caught before and which is not in the log yet. It writes it when the RDS name arrives, or when you tune away or leave DX mode. The same check for a station already logged and the same 250 limit apply.

**DX Scanner > Learn Local Stations** marks the stations you can hear now as already caught, so they are never logged as new. It writes nothing to the log.
