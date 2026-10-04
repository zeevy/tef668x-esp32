/*
 * The screens, drawn with LVGL.
 *
 * Text is placed by its baseline, because a row of labels lines up on their
 * baselines and not on their tops.
 *
 * It takes a struct of strings and numbers rather than the radio's own state,
 * so `ui/` does not read the radio's state: the layer above fills this in,
 * and the screen knows nothing about tuners or tasks.
 *
 * The radio screen is a table of panels rather than a drawing function. Which
 * table is used follows from the size of the display.
 */
#ifndef UI_SCREEN_H
#define UI_SCREEN_H

#include <stdbool.h>
#include <stdint.h>

/* Why there is no sound, which decides what the V: tile shows. */
typedef enum {
  SCREEN_AUDIO_ON = 0,    /* The volume, in amber. */
  SCREEN_AUDIO_MUTED,     /* A person did it. MUTE, in the warning colour. */
  SCREEN_AUDIO_SQUELCHED, /* The squelch did it. The volume, in grey. */
} ScreenAudio;

/*
 * What the network link is doing.
 *
 * Four states and not two. A radio that has given up on its network and one
 * that is fifteen seconds from being on it mean opposite things: the first
 * needs somebody to fix the credentials and the second needs nothing at all.
 *
 * The fourth is the radio's own access point. That is the one state where
 * somebody has to do something, so a radio serving the page that fixes the
 * problem must not look like a radio that has given up.
 *
 * The strength is `wifiBars`, below, and only when this is SCREEN_WIFI_
 * JOINED. The icon font has `wifi_1_bar` and `wifi_2_bar` as well as the
 * full `wifi`, so the symbol shows how strong the link is.
 */
typedef enum {
  SCREEN_WIFI_NONE = 0, /* Not joined and not trying. A crossed out symbol. */
  SCREEN_WIFI_TRYING,   /* Working through the stored credentials. */
  SCREEN_WIFI_AP,       /* Serving its own network so it can be fixed. */
  SCREEN_WIFI_JOINED,   /* On the network. */
} ScreenWifi;

/*
 * What to show on the radio screen.
 *
 * Every string must stay valid for the length of the call. A NULL string and
 * a false `has` flag mean the radio cannot answer, and the screen leaves that
 * part blank rather than printing a zero. A zero is a reading.
 */
typedef struct {
  /* ---- the header ---- */
  bool fm;          /* FM or OIRT: the scale's marks are 100 kHz apart. */
  const char *band; /* "FM", "MW" and so on. */
  /* The shortwave metre band the radio is inside, "31 m", or NULL on any
   * other band and between two metre bands. */
  const char *meterBand;
  /* The time, "HH:MM", or NULL until a server has answered. The status run
   * closes up over it. */
  const char *clock;
  ScreenAudio audio;
  /*
   * What the network is doing, which is always drawn: a symbol that
   * disappears when the radio is not joined reads as a radio that has lost
   * its network hardware rather than as one that is off the network.
   */
  ScreenWifi wifi;
  /* How many of the symbol's arcs are lit, 0 to 3, when `wifi` is
   * SCREEN_WIFI_JOINED. */
  uint8_t wifiBars;
  /*
   * The battery: the shape filled to `batteryPercent` when `batteryText` is
   * NULL, or `batteryText`, the voltage with no unit, and a small upright
   * shape filled the same way. `batteryValid` false leaves both out.
   */
  bool batteryValid;
  uint8_t batteryPercent;
  const char *batteryText;

  /* ---- the amber panel ---- */
  /*
   * The name line: the confirmation of a logbook write while it holds, else
   * what the station calls itself, else what a person called this memory
   * channel, else "---". The station name is not always eight characters: a
   * split name is stitched, so it can be as long as 32 and the line bounds
   * it.
   */
  /* "Logged 106.40" or "Not logged" for 1.5 s, or NULL. */
  const char *logConfirm;
  const char *stationName;
  const char *memoryName;
  const char *frequency; /* Already formatted, such as "104.00". */
  const char *unit;      /* "MHz" or "kHz". */
  const char *memory;    /* "P12", or NULL when not on a stored channel. */
  /*
   * Digits keyed on the keypad and not yet entered, formatted with a
   * trailing hyphen such as "104-", or NULL when nobody is typing. Drawn in
   * place of `frequency`, with no unit and no slot: both describe a tuned
   * frequency and this is not one yet.
   */
  const char *typing;
  int16_t signalDbuV;    /* Whole dBuV, held still. */
  bool signalValid;      /* False when the last reading failed. */
  uint8_t signalPercent; /* How tall the tuning scale's peak stands, 0 to
                            100, against the top for the band group. */
  /* The modulation meter, damped the way a level meter is: it rises to a
   * reading at once and falls back slowly, and a peak mark holds. */
  bool modulationValid;          /* False when the last reading failed. */
  uint8_t modulationPercent;     /* 0 to 100; a reading past 100 is full. */
  bool modulationPeakValid;      /* A peak mark is to be drawn. */
  uint8_t modulationPeakPercent; /* Where it is, 0 to 100. */

  /* ---- the line under the panel ---- */
  const char *radioText; /* Or NULL. */
  /* "WEDNESDAY, 30th September 2026", or NULL until a server has answered.
   * Shown when there is no radio text. */
  const char *date;

  /* ---- the scale ---- */
  /*
   * The band's two ends and the tuned frequency, in kHz. A span of 0 means
   * the band plan could not be read, and the scale is not drawn.
   */
  uint32_t sweepLowKHz;
  uint32_t sweepSpanKHz;
  uint32_t sweepKHz;

  /* ---- the tiles ---- */
  const char *tuneMode;    /* "MAN", "AUTO", "MEM" or "MTR". */
  const char *squelchMode; /* "OFF", "AUTO", or the Manual level "35dB". */
  const char *filter;      /* "DYN" while the tuner chooses, else "84k". */
  const char *volume;      /* "-6dB". */

  bool tunerReady;
  const char *fault; /* What went wrong, or NULL when nothing did. */
} ScreenState;

/* How many self tests the boot screen has room for. Two columns of three. */
#define SCREEN_BOOT_STEPS 6

/* What one self test came back with. */
typedef enum {
  SCREEN_BOOT_WAITING = 0, /* Not answered yet. A dash, not a tick. */
  SCREEN_BOOT_OK,          /* A tick. */
  SCREEN_BOOT_FAILED,      /* A cross. */
} ScreenBootMark;

/*
 * One row of the self test.
 *
 * `value` is what the step found, when finding a number is the answer: the
 * count of stored channels, for one. It is drawn instead of the tick, because
 * a tick beside a number says nothing the number does not.
 */
typedef struct {
  const char *name; /* NULL for a row this build does not use. */
  ScreenBootMark mark;
  const char *value;
} ScreenBootStep;

/*
 * The boot screen.
 *
 * Every row is something `setup` actually answered. There are no Touch and
 * Clock rows, because this firmware can say nothing true about either at boot:
 * touch is not built, and the clock needs a network that deliberately arrives
 * after the radio does. A tick there would not be earned, so those two are
 * not rows.
 */
typedef struct {
  const char *product; /* TEF668X. */
  const char *board;   /* The board's name, on the right of the header. */
  /*
   * What the tuner says it is, once it has said it, and the firmware's own
   * version beside it.
   *
   * NULL until the patch is in, and drawn as nothing rather than as a guess.
   * The part number is read off the chip, so a line here means the I2C
   * conversation happened.
   */
  const char *tuner;
  ScreenBootStep steps[SCREEN_BOOT_STEPS];
  uint8_t done;  /* How many have answered. */
  uint8_t total; /* How many there are. */
} ScreenBoot;

/*
 * Put the boot screen up, fill it in, and take it down.
 *
 * `screenBootShow` is called again for every step, from `setup`, and draws
 * immediately: the poll that normally redraws the panel does not run until
 * `loop` does, and the whole point of this screen is what happens before
 * then.
 *
 * `screenBootEnd` deletes it and gives the panel back to the radio. Nothing
 * else hides it, so a caller that forgets leaves the radio behind a boot
 * screen for ever.
 */
bool screenBootBegin(void);
void screenBootShow(const ScreenBoot *boot);
void screenBootEnd(void);

/* How many menu rows the panel holds at once. */
#define SCREEN_MENU_ROWS 6

/* One row of the menu, as the screen needs it. */
typedef struct {
  const char *name;  /* NULL for a slot with nothing in it. */
  const char *value; /* What it is set to, or NULL for a row with no value. */
  bool selected;     /* The cursor is on it. */
  bool opens;        /* A group row, which opens a list: a chevron, and the
                        value is a count, drawn in `dead`. */
  bool secret;       /* The value is the access PIN, read out as stars. */
} ScreenMenuRow;

/* How many rows the picker screen holds at once. The same six the list
 * holds. */
#define SCREEN_MENU_PICKER_ROWS 6

/*
 * One row of a named-choice picker, `ScreenMenuValue.picker`'s own element.
 *
 * `isCursor` is where the knob is now, live; `isSaved` is what the row was
 * set to before this edit began, which is a different fact. `themeIndex` is
 * read only when `ScreenMenuValue.isTheme` is set, and is the saved theme
 * index this row names, for the swatches.
 */
typedef struct {
  const char *name; /* NULL for an unused slot; the list simply ends there. */
  bool isCursor;
  bool isSaved;
  uint8_t themeIndex;
} ScreenMenuPickerRow;

/*
 * The menu, as one screenful.
 *
 * A window on to a longer list rather than the list itself. Which six rows
 * these are is worked out by `menuWindowTop` in core, where it is tested; the
 * screen is handed six rows and draws six rows.
 *
 * The header is the title and nothing else. A note belongs to the row the
 * cursor is on, since every note answers a press on it, so it is drawn on
 * that row in place of its value until the next gesture.
 */
typedef struct {
  const char *title; /* The list's name, or "Menu" for the group list. */
  const char *note;  /* What the last press on the cursor's row did, or NULL. */
  ScreenMenuRow rows[SCREEN_MENU_ROWS];
  /* The whole list's length and its row at the top of the window, for the
   * scroll bar a list longer than the window shows. A total of 0 is none. */
  uint8_t total;
  uint8_t top;
} ScreenMenu;

/*
 * Put the menu up, fill it in, and take it down.
 *
 * The same shape as the boot screen and for the same reason: it owns the
 * panel on its own, and the radio layout is deleted before this is built and
 * rebuilt after it goes. Both at once does not fit in the LVGL pool.
 */
bool screenMenuBegin(void);
void screenMenuShow(const ScreenMenu *menu);
void screenMenuEnd(void);

/*
 * One setting, on its own, while it is being changed.
 *
 * The list cannot do this job. A value being edited inside a row has to say,
 * in a highlight a few pixels wide, that the knob has stopped moving the
 * cursor and started moving a number, and that is the one thing a person must
 * know before they turn it. A screen with nothing else on it says it without
 * being read.
 *
 * `value` and `unit` are separate so the number can be drawn at the size the
 * frequency is drawn at. That face carries digits, a point, a plus, a minus,
 * a space and a colon and nothing else, which is most of why it is only 8 KB,
 * so a value that is a word takes the largest face that has letters in it
 * instead.
 *
 * The bar is what the list never gave: where this value sits between its
 * limits. A brightness of 70 means nothing on its own and a great deal three
 * quarters of the way along a bar labelled 5 and 100.
 */
typedef struct {
  const char *name;  /* What is being changed. */
  const char *value; /* The number, or the word. */
  const char *unit;  /* What the number is in, or NULL. */
  bool hasRange;     /* Whether the bar means anything for this row. */
  int32_t min;
  int32_t max;
  int32_t at;
  const char *minText;
  const char *maxText;
  /* Something about this setting the screen cannot otherwise say, such as
   * "Applies after restart": on the cursor's row of a picker, and on the
   * line under the bar or the digits otherwise. NULL for none. */
  const char *note;

  /*
   * A named choice, picked from a short list instead of scrubbed on a bar.
   *
   * An enum has no distance, so it gets a list rather than the bar
   * `hasRange` draws.
   * `isPicker` is this row's own third shape, alongside the list screen and
   * the bar screen; neither of those changes when it is set. `picker` is
   * windowed the same way `ScreenMenu.rows` already windows a longer list,
   * six slots and no more, an unused one carrying a NULL `name`.
   */
  bool isPicker;
  bool isTheme; /* Show each row's own four swatches, not just its name. */
  ScreenMenuPickerRow picker[SCREEN_MENU_PICKER_ROWS];
  uint8_t pickerTotal; /* The whole list, as ScreenMenu's `total`. */
  uint8_t pickerTop;

  /*
   * Digits set one at a time, the Web PIN, in place of the value and the
   * bar. `digits` is all of them, `digitAt` the one being set, which is
   * boxed; the ones after it are dim, and a row of dots under the panel says
   * where it is. `digitPlace` is the line under the dots, "Digit 3 of 6",
   * and `label` what the panel says over the digits.
   */
  bool isDigits;
  const char *digits;
  uint8_t digitAt;
  const char *digitPlace;
  const char *label;
} ScreenMenuValue;

void screenMenuValueShow(const ScreenMenuValue *value);

/*
 * Everything the RDS decoder knows, over four pages: the station, its radio
 * text with RT+, its other networks, and the decoder's last minute.
 *
 * Reached by holding BAND, left with a tap of MODE, and paged with the knob,
 * which belongs to this screen while it is up in place of tuning: `input_task`
 * hands it clicks the same way it hands the menu clicks rather than the
 * accelerated steps that move the dial. It redraws from the poll, because
 * what it shows arrives a group at a time and a station takes seconds to say
 * its name.
 *
 * Every field is a string the caller has already decided about, and NULL
 * means the radio cannot answer yet. That is the whole shape of the RDS
 * decoder: a name that has not arrived and an empty name are different
 * things, and a screen that drew both as blank would lose that distinction.
 * A flag is three states for the same reason.
 */
#define SCREEN_RDS_PAGES 4
#define SCREEN_RDS_TAGS 3
#define SCREEN_RDS_AF 8
#define SCREEN_RDS_EON 3
#define SCREEN_RDS_GROUPS 6

typedef enum {
  SCREEN_RDS_UNKNOWN, /* Not heard yet. */
  SCREEN_RDS_NO,
  SCREEN_RDS_YES,
} ScreenRdsFlag;

typedef struct {
  const char *label; /* TITLE, ARTIST and so on. */
  const char *text;
} ScreenRdsTag;

typedef struct {
  const char *pi;
  const char *ps;    /* NULL until all eight characters came. */
  const char *freqs; /* NULL when none were sent. */
  ScreenRdsFlag ta;  /* An announcement on air on it now. */
} ScreenRdsEon;

/* One block's last minute, in tenths of a per cent of its groups, so a
 * lost block in a thousand still shows on its bar. */
typedef struct {
  uint16_t fixedTenths;
  uint16_t lostTenths;
  const char *text; /* "11 % . 0.6 %", or NULL for all clean. */
} ScreenRdsBlock;

typedef struct {
  const char *label; /* "0A". */
  uint8_t percent;
} ScreenRdsGroup;

typedef struct {
  uint8_t page;          /* 0 to SCREEN_RDS_PAGES - 1. */
  const char *frequency; /* Which station this is about, every page's header. */
  const char *clock;     /* "05:09", or NULL until a server has answered. */
  /* A moment's message, typed digits or what a log hold did: it takes the
   * header's run, the frequency, page and clock giving way, or NULL. */
  const char *message;

  /*
   * Page 1, the station. `piSure` is false for a PI still being made sure
   * of, which is drawn dimmed. `country` is the two letter code from the PI
   * and the ECC together, and NULL without an ECC; `area` is the PI's
   * coverage nibble, never a country.
   */
  const char *ps;
  const char *ptyNumber; /* "PTY 12". */
  const char *ptyName;
  const char *pi;
  bool piSure;
  const char *country; /* Or, with no country, why: no ECC, or not listed. */
  bool countryNamed;   /* `country` is a country code, not a reason. */
  /* `country` is North American call letters worked out from the PI, a
   * guess, rather than a country or the station's own RT+ name. */
  bool countryGuess;
  /* `country` says another station is on the preset, "not P03", in red. */
  bool countryFault;
  const char *ptyn;
  const char *area;
  const char *language;
  const char *ecc;
  const char *ct;
  ScreenRdsFlag tp;
  ScreenRdsFlag ta;
  ScreenRdsFlag speech; /* YES speech, NO music. */
  ScreenRdsFlag stereo; /* The DI bit: YES stereo, NO mono. */

  /* Page 2, what it is saying. With no tag to show, `rtPlusNote` says why. */
  const char *text;
  ScreenRdsTag tag[SCREEN_RDS_TAGS];
  uint8_t tagCount;
  bool rtPlusRunning;
  const char *rtPlusNote;

  /* Page 3, its other frequencies and networks. `afMore` is how many did
   * not fit, shown on the last tile. */
  const char *af[SCREEN_RDS_AF];
  uint8_t afCount;
  uint8_t afMore;
  ScreenRdsEon eon[SCREEN_RDS_EON];
  uint8_t eonCount;
  uint8_t eonMore;   /* Listed networks that did not fit. */
  bool eonMoreHeard; /* More were named than the radio keeps. */

  /*
   * Page 4, the decoder over the last minute. `sync` alone says whether the
   * decoder is off, not locked, or locked and for how long. `blocksKnown` is
   * false with no group in the last minute, and then the rows are empty.
   */
  const char *span; /* The header of page 4: the frequency and the span. */
  const char *sync;
  bool syncGood;
  const char *rate;
  const char *bler;
  const char *lost;
  bool blocksKnown;
  ScreenRdsBlock block[4];
  ScreenRdsGroup group[SCREEN_RDS_GROUPS];
  uint8_t groupCount;
  /* With no group type to show: none in the minute, or none with a clean
   * block B to read a type from. */
  const char *groupNote;
} ScreenRds;

bool screenRdsBegin(void);
void screenRdsShow(const ScreenRds *rds);
void screenRdsEnd(void);

/*
 * The DX page, the first page of DX mode.
 *
 * The same shape as the RDS screen: every field is decided by the caller,
 * and NULL, a -1 or a false means the radio cannot say yet, which the page
 * draws as blank rather than as a zero.
 *
 * No DX page has a line of hints at the foot. A moment's message, such as
 * what a log hold did, comes in `position` while it shows, with the clock
 * and the header's context NULL, so it takes the right of the header.
 */
#define SCREEN_DX_PS_LEN 8
#define SCREEN_DX_HISTORY 60

/* The PI tile's five looks, by shape and not only by colour. */
typedef enum {
  SCREEN_DX_PI_NONE,      /* An empty grey tile. */
  SCREEN_DX_PI_SEEN,      /* Grey, amber digits, a clock: heard once. */
  SCREEN_DX_PI_PARTIAL,   /* Grey, amber digits with `?`, a help mark. */
  SCREEN_DX_PI_CONFIRMED, /* Amber, a tick. */
  SCREEN_DX_PI_ZERO       /* Grey, dead digits, a block mark and NO ID. */
} ScreenDxPi;

typedef struct {
  const char *position;   /* "1/1". */
  bool positionIsMessage; /* The position is a moment's message. */
  const char *clock; /* Local, "22:47", or NULL until a server has answered. */
  bool stereo;       /* A stereo pilot, drawn as the ring pair. */

  /* The name, a cell a character. `psShown` false leaves every cell empty;
   * true draws each cell as its character or, when `psHave` is false, as
   * the short bar that means that character has not arrived. */
  bool psShown;
  char ps[SCREEN_DX_PS_LEN];
  bool psHave[SCREEN_DX_PS_LEN];
  /* The whole name as one text once every character has arrived, without
   * the spaces a station pads it with, "MAGIC" from " MAGIC  "; empty while
   * any character is still missing. Letters differ in width, so a name in
   * fixed cells reads unevenly, and a fixed cell only matters while the
   * characters are still arriving. */
  char psWhole[SCREEN_DX_PS_LEN + 1];
  const char *pty;

  const char *frequency; /* "94.70". */
  const char *unit;      /* "MHz". */

  ScreenDxPi pi;
  char piDigits[5]; /* Four characters, `?` for a digit in doubt. */
  /* "IN", or NULL when there is none to name. On North America the
   * station's name or call letters instead. */
  const char *country;
  /* Draw the help mark: the country is not known, or the call letters are
   * only worked out from the PI. */
  bool countryUnsure;
  /* A confirmed PI that is not the stored one of the preset the dial is on:
   * a grey tile, a red cross, and `country` saying "not P03" in red. */
  bool piOther;

  /* The six readings, each a number already formatted. */
  const char *level;
  const char *usn;
  const char *wam;
  const char *offset;
  const char *bandwidth;
  const char *modulation;

  /* The last minute, oldest first, in tenths of a dBuV, and whether each
   * second had a reading at all. */
  int16_t historyTenths[SCREEN_DX_HISTORY];
  bool historyHave[SCREEN_DX_HISTORY];

  /* Each block's error level in the last group, 0 to 3, or -1 when no group
   * has arrived. */
  int8_t blockError[4];
} ScreenDx;

bool screenDxBegin(void);
void screenDxShow(const ScreenDx *dx);
void screenDxEnd(void);

/*
 * The DX Catches page, the fourth page of DX mode. Six rows a screen, the one
 * under the cursor amber, or an empty box when nothing has been caught.
 */
#define SCREEN_CATCH_ROWS 6

typedef struct {
  const char *time; /* Local "22:44", or NULL when the time was not known. */
  const char *frequency; /* "103.45". */
  const char *pi;        /* "2F17". */
  const char *ps;        /* The name, or NULL when none arrived. */
  const char *country;   /* "RO", or NULL. */
  bool countryUnsure;    /* No ECC arrived: the help mark. */
  bool isNew;            /* Never caught before: the NEW badge. */
  const char *level;     /* The best level, "27.9". */
  const char *count;     /* "\xC3\x973", the times it was confirmed. */
} ScreenCatchRow;

typedef struct {
  const char *range;      /* "1-6 of 12", or NULL when the list is empty. */
  const char *position;   /* "2/2". */
  bool positionIsMessage; /* The position is a moment's message. */
  const char *clock;      /* Local, or NULL. */
  uint8_t rows;           /* 0 to SCREEN_CATCH_ROWS. */
  uint8_t cursor;         /* Which of those rows is amber. */
  ScreenCatchRow row[SCREEN_CATCH_ROWS];
} ScreenCatches;

bool screenCatchesBegin(void);
void screenCatchesShow(const ScreenCatches *c);
void screenCatchesEnd(void);

/*
 * The DX Scanner page, the third page of DX mode. The amber panel with what the
 * scan is doing, the band's progress under it, and the station on the channel
 * in a tile at the foot.
 */
typedef enum {
  SCREEN_SCAN_IDLE = 0, /* No mark, and "Ready" where the frequency goes. */
  SCREEN_SCAN_RUNNING,  /* The run mark, the frequency and the dwell left. */
  SCREEN_SCAN_STOPPED,  /* The stopped mark and "STOPPED", and the frequency. */
} ScreenScanState;

typedef struct {
  const char *found;      /* "3 found". */
  const char *position;   /* "2/3". */
  bool positionIsMessage; /* The position is a moment's message. */
  const char *clock;      /* Local, or NULL. */
  bool isNew; /* Stopped on a PI never caught before: the NEW pill. */
  ScreenScanState state;
  const char *mode;      /* "BAND + PRESETS". */
  const char *rule;      /* "STOP ON PI". */
  const char *frequency; /* "96.40", or NULL when idle. */
  const char *dwell;     /* "2.5 s". */
  const char *left;      /* "1.7", the dwell left, or NULL. */
  /* The band's progress, 0 to 1000, and whether to draw it at all. */
  bool hasProgress;
  uint16_t progressPermille;
  const char *from;  /* "87.5". */
  const char *to;    /* "108.0". */
  const char *step;  /* "89 / 206", or NULL. */
  bool stationOn;    /* The tile amber: stopped on a catch. */
  const char *pi;    /* "63B2", "63?2", or NULL for an empty tile. */
  bool piSure;       /* Confirmed as this channel's own, not only heard. */
  const char *ps;    /* The name, or NULL. */
  const char *level; /* "12.7", or NULL. */
  /* With no PI, what the tile says in its place: no station yet, or while a
   * scan runs, no RDS yet. NULL with a PI. */
  const char *note;
} ScreenScan;

bool screenScanBegin(void);
void screenScanShow(const ScreenScan *s);
void screenScanEnd(void);

/*
 * The DX Scope page: the latest level sweep over the band as bars, its
 * baseline, peak hold and noise floor over them, a strip of the rise over
 * the baseline under them, a cursor the knob moves, and the cursor channel's
 * readings in a tile at the foot.
 */
#define SCREEN_SCOPE_NONE INT16_MIN /* A channel with no reading. */

typedef struct {
  const char *context;    /* "3 min ago", "sweeping", or NULL. */
  const char *position;   /* "2/4". */
  bool positionIsMessage; /* The position is a moment's message. */
  const char *clock;      /* Local, or NULL. */
  uint16_t revision;      /* Moves whenever the three arrays change. */
  uint16_t count;         /* Channels; 0 when there is no sweep. */
  const int16_t *level;   /* `count` levels, tenths of a dBuV, or NONE. */
  const int16_t *base;    /* The baseline's, or NULL for none. */
  const int16_t *peak;    /* The peak hold's, or NULL for none. */
  int16_t floor;          /* The noise floor, or SCREEN_SCOPE_NONE. */
  uint16_t cursor;        /* The cursor's channel. */
  uint16_t dial;          /* The dial's channel, UINT16_MAX when off them. */
  bool sweeping;          /* A sweep is running: grey bars and "Sweeping". */
  const char *from;       /* "87.0", "97.5", "108.0" under the chart. */
  const char *mid;
  const char *to;
  const char *baseText;    /* "MEDIAN OF 5", "FIXED", or NULL. */
  const char *floorText;   /* "FLOOR 0.8", or NULL. */
  const char *empty;       /* In the chart when there is no sweep, or NULL. */
  const char *cursorFreq;  /* "98.30", or NULL with no sweep. */
  const char *cursorLevel; /* "51.4", or NULL for no reading. */
  const char *cursorRise;  /* "+0.4", or NULL with no baseline there. */
  bool riseUp;             /* Above the baseline: amber, else grey. */
} ScreenScope;

bool screenScopeBegin(void);
void screenScopeShow(const ScreenScope *s);
void screenScopeEnd(void);

/*
 * The bandwidth page: every width the band takes as a tile, automatic first
 * on FM, and on FM the iMS and equaliser switches after them. The width in
 * use and a switch that is on are filled; the cursor is a ring.
 */
#define SCREEN_BW_TILES 19
#define SCREEN_BW_WIDTH 0
#define SCREEN_BW_IMS 1
#define SCREEN_BW_EQ 2

typedef struct {
  const char *text; /* "AUTO", "114", "iMS On". */
  uint8_t kind;     /* SCREEN_BW_WIDTH, _IMS or _EQ: where it sits. */
  bool filled;      /* The width in use, or a switch that is on. */
  bool cursor;
} ScreenBwTile;

typedef struct {
  const char *context;  /* "FM \xC2\xB7 kHz". */
  const char *position; /* "7/19". */
  const char *clock;    /* Local, or NULL. */
  uint8_t count;
  ScreenBwTile tile[SCREEN_BW_TILES];
  bool hasSwitches; /* An FM band: the widths fill from the top. */
  const char *note; /* "Auto: 217 kHz", or NULL. */
} ScreenBw;

bool screenBwBegin(void);
void screenBwShow(const ScreenBw *s);
void screenBwEnd(void);

/* How many rows the recovery list holds on screen at once, and how many it
 * actually has. */
#define SCREEN_RECOVERY_VISIBLE 5
#define SCREEN_RECOVERY_ROWS 5

/*
 * One row of the recovery list.
 *
 * `value` is shown for the one row that carries one, the display's own
 * current rotation; every other row leaves it NULL and draws its name alone.
 * `inert` marks a row that is drawn and can be reached with the knob but does
 * nothing when pressed, Restore Previous Firmware after a USB flash, when
 * there is nothing to go back to; it reads no different from a live row,
 * since a screen with no theme and no icon set has nothing to dim it with,
 * and this is the one screen where a person is already reading every word
 * rather than skimming for a colour.
 */
typedef struct {
  const char *name; /* NULL for an unused slot past the last row. */
  const char *value;
  bool inert;
} ScreenRecoveryRow;

/*
 * Everything the recovery screen shows: the five rows, and which of them
 * the cursor is on.
 *
 * Always the first palette, never the saved theme. The setting that put the
 * radio here may be the theme itself, and a screen that reads it to draw its
 * own way out could be unreadable for the same reason it is needed.
 */
typedef struct {
  uint8_t cursor; /* Absolute, 0 to SCREEN_RECOVERY_ROWS - 1. */
  ScreenRecoveryRow rows[SCREEN_RECOVERY_ROWS];
  /* The foot line, or NULL for the usual one: what a second press on a row
   * waiting for it will do. */
  const char *hint;
} ScreenRecovery;

bool screenRecoveryBegin(void);
void screenRecoveryShow(const ScreenRecovery *recovery);
void screenRecoveryEnd(void);

/*
 * Build the screens.
 *
 * LVGL must already be running. Returns false if it is not, and a caller that
 * sees that should carry on without a screen: a radio with a dead panel still
 * has to be reachable over the air, because that is how a fix gets installed.
 */
bool screenBegin(void);

/*
 * Show the state.
 *
 * Only what changed is written. LVGL redraws only what it has been told is
 * dirty, so setting a label to the string it already holds costs a compare
 * and nothing else, but the strings still have to be built, and a full
 * repaint is twelve strips of 320 by 20 pixels, about 150 ms, 37 of it
 * pushes at the panel's 40 MHz.
 */
void screenShow(const ScreenState *state);

/*
 * Take the radio layout down.
 *
 * For a screen that has to own the panel on its own, which is every full
 * screen. The LVGL pool is 24 KB and this layout is about 15.6 KB
 * of it, so a second screen built on top of it does not fit and LVGL's answer
 * to an allocation it cannot make is an assertion, which this firmware turns
 * into a restart.
 *
 * `screenBegin` builds it again.
 */
void screenEnd(void);

/*
 * Say something across the middle of the screen, on its own.
 *
 * For the moments before there is any state worth showing, and for a tuner
 * that did not answer.
 */
void screenMessage(const char *line1, const char *line2);

/*
 * A firmware write, in progress, across the middle of the screen.
 *
 * The shape of the menu's value editor: the caption and the percentage, the
 * one thing that moves, on the amber panel, the bar under it, and a line
 * saying not to pull the plug. A write shows it from its first byte: a
 * `percent` below zero, before the size is known, draws it with no number
 * and an empty bar. `screenMessage`'s two lines are what is left when a
 * write fails, and calling either one hides whatever the other last drew.
 */
void screenUpdateVeilShow(int percent);

#endif /* UI_SCREEN_H */
