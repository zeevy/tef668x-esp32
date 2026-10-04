# Controls

Every button, knob and key, on every screen. The buttons, the knob press and ENTER can be tapped or held. A hold is 600 ms: it acts while the key is still down. A tap acts when you let go. The digits and DX act as soon as they go down, and have no hold.

<!-- photo: the front of the ATS-125 with numbered callouts on the tuning knob, the volume knob, BAND, BW, MODE, the keypad, ENTER and DX -->

## The controls

| Control | Where |
|---|---|
| Tuning knob | Turns, and presses down. The push is called PUSH in the HTTP API |
| Volume knob | Turns only |
| BAND, BW, MODE | Three buttons |
| Keypad | Digits 0 to 9, ENTER and DX |

In MAN mode, turning the tuning knob fast moves it further: two, four or six steps for each click. Everywhere else one click is one step. If you turn the knob while you press it, the press is dropped.

## On the radio screen

| Control | Tap or turn | Hold |
|---|---|---|
| Tuning knob, turn | Tunes, by the tuning mode: **MAN** steps by the step size, **AUTO** seeks the next station, **MEM** goes to the next preset, **MTR** (shortwave only) steps inside the broadcast metre bands | |
| Tuning knob, press | Opens the menu | Writes the station to the [station log](Station-Log.md) |
| BAND | The next band: LW, MW, SW, OIRT, FM, then LW again | The RDS pages, on FM and OIRT. On AM it shows `Switch to FM first` |
| BW | The next filter width. On FM the list includes automatic | The bandwidth page |
| MODE | The next tuning mode: MAN, AUTO, MEM, and MTR on shortwave | The menu |
| 0 to 9 | Types a frequency | |
| ENTER | Tunes the typed frequency | Writes the station to the station log |
| DX | Opens DX mode, on FM and OIRT. On AM it shows `Switch to FM first` | |
| Volume knob | The volume. In Manual squelch, the squelch level | |

In AUTO, a turn starts a seek. Turning the same way again stops it, and turning the other way seeks the other way.

After a station log hold, the name line above the frequency says what happened for 1.5 seconds: `Logged` and the frequency, `Already Logged`, `Not logged` or `Still tuning - wait`.

## Typing a frequency

Type the digits on the keypad, then press ENTER. [Tuning](Tuning.md#typing-a-frequency) has the details. You do not type a decimal point: the radio reads the number in the unit of the band, and tries the band you are on first. For example, `1064` on FM tunes 106.4 MHz, and `909` on MW tunes 909 kHz.

- If the number does not fit the band you are on, and fits more than one other band, a list asks which band to tune.
- If it fits no band, the screen says that the number `is in no band`.
- Up to 7 digits are taken. The number is dropped 10 seconds after the last digit.
- A turn of the knob, the knob press, BAND, BW, MODE or DX drops the number and does nothing else.

## In the menu

The menu has three levels: the groups, the rows of a group, and some rows that open a sub-group.

| Control | Tap or turn | Hold |
|---|---|---|
| Tuning knob, turn | Moves one row. While a value is being changed, changes the value | |
| Tuning knob, press | Opens a group or sub-group, starts changing a value, or runs an action. While a value is being changed, keeps it | Goes back one level. While a value is being changed, puts it back as it was |
| ENTER | The same as the knob press | The same as the knob hold |
| MODE | The same as the knob hold | Closes the menu, putting back a value being changed |
| BAND, BW | Nothing | Nothing |
| Keypad | Nothing, except the digits of the Web PIN | |

Most values change as you turn, so you hear or see the result at once. A few act only when you press to keep them: the themes, Display Rotation, Wi-Fi, Hotspot, Web Server and Web PIN. If you go back without keeping a value, the row shows `Not Saved` and the old value comes back. With no input for one minute, the menu closes and the radio screen comes back.

The menu opens again on the row you left, until the radio restarts.

## On the RDS pages

Hold BAND on FM to open them. They always open on the first page, and they do not time out.

| Control | Tap or turn | Hold |
|---|---|---|
| Tuning knob, turn | The next or previous page | |
| Tuning knob, press | Nothing | Closes the RDS pages |
| BAND | Nothing | Closes the RDS pages |
| BW | The next filter width | The bandwidth page |
| MODE | Closes the RDS pages | The menu |
| ENTER | The next page | Writes the station to the station log |
| DX | Opens DX mode. When the RDS pages were opened over DX mode, it closes both and goes back to the radio screen | |

## In DX mode

Press DX on FM to open it. It has four pages, **DX**, **Scope**, **Scanner** and **Catches**, and opens on DX. It does not time out, and it closes if the radio leaves FM.

These work on every page:

| Control | Tap | Hold |
|---|---|---|
| BAND | The next page | The RDS pages, over DX mode |
| BW | On the DX page, the next filter width for DX mode | The bandwidth page |
| MODE | Leaves DX mode | The menu |
| ENTER | The next page | Writes the station to the station log |
| DX | Leaves DX mode | |

The tuning knob does something different on each page:

| Page | Turn | Press | Hold |
|---|---|---|---|
| DX | Tunes by the step size | The RDS pages, over DX mode | Leaves DX mode |
| Scope | Moves the cursor one channel | Starts or stops a sweep | Tunes to the cursor |
| Scanner | Tunes, only while the scanner is stopped | Starts the scan, or goes on with it | Leaves DX mode |
| Catches | Moves the cursor one row | Tunes to that catch | Writes that catch to the station log. With no catches, leaves DX mode |

While the scanner runs, any key or turn only stops it.

## On the bandwidth page

Hold BW to open it. It shows a tile for each filter width of the band, and on FM also **iMS** and **EQ**.

| Control | Tap or turn | Hold |
|---|---|---|
| Tuning knob, turn | Moves between the tiles | |
| Tuning knob, press | Picks that width, or turns iMS or EQ on or off | Closes the page |
| ENTER | The same as the knob press | Nothing |
| BW | Closes the page | |
| MODE | Closes the page | The menu |
| BAND, keypad | Nothing | |

With no input for one minute, the page closes. It also closes if the band changes.

## The volume knob

The volume knob goes from -60 dB, the quietest the tuner takes, to 0 dB. The first tenth of the travel covers -60 to -30 dB, and the rest covers -30 to 0 dB, where most listening happens.

When squelch is set to Manual, the volume knob sets the squelch level instead, and the volume stays where it was. The bottom of the travel then means the squelch is always open.

The volume knob works on every screen, and moving it wakes a dimmed screen.

## A dimmed screen

When the screen has dimmed, the first key press or knob turn only wakes it, and does nothing else. So you can wake the radio without changing the station. The volume knob is the exception: it changes the volume and wakes the screen.

The same first input also skips the boot screen, and closes the `Update Failed` screen.

## While it sleeps

Only a press of the tuning knob wakes the radio from sleep. The radio then starts again from the boot screen, and that press does nothing else.

## On the recovery screen

Only the tuning knob works: turn it to move between rows, and press it to choose. Every row but Exit & Start Radio asks for a second press.
