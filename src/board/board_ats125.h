/*
 * Pin map and feature flags for the ATS-125.
 *
 * Every number here was read off the chip or taken from the working PE5PVB
 * firmware. Nothing in here is a guess.
 */
#ifndef BOARD_BOARD_ATS125_H
#define BOARD_BOARD_ATS125_H

/* Board id, lower case. Used in build output, JSON and file names. */
#define BOARD_NAME "ats125"

/* The same name as it is shown to a person, on screen and on the web page. */
#define BOARD_NAME_DISPLAY "ATS125"

/* mDNS name and the ArduinoOTA hostname, so the radio is tef668x.local. */
#define BOARD_HOSTNAME "tef668x"

/* SSID prefix for the setup access point. The last two MAC bytes follow. */
#define BOARD_AP_PREFIX "tef668x-setup"

/* ---------------------------------------------------------------------------
 * Feature flags. A flag at 0 means that code is not compiled and costs
 * nothing. A reserved flag stays at 0, because no code reads it yet.
 * ------------------------------------------------------------------------ */
#define FEATURE_OTA 0 /* ArduinoOTA listener, off: /update does the job. */
#define FEATURE_TELEMETRY \
  0 /* JSON state over UDP. Reserved: no code reads this flag yet. */
#define FEATURE_SPECTRUM \
  0 /* A layout of the radio screen around the band's spectrum. Reserved: no \
     * code reads this flag yet. The band scope and the sweeps do not use it. \
     */
#define FEATURE_ALARM \
  0 /* Alarm clock, not auto off. Reserved: no code reads this flag yet. */
#define FEATURE_TOUCH \
  1 /* Reads the XPT2046 touch chip and reports its raw readings. No touch
     * acts on the radio yet. */
#define FEATURE_RTC \
  0 /* RX8010 real time clock. Reserved: no code reads this flag yet. */
#define FEATURE_AIR_BAND \
  0 /* Needs a converter board. Reserved: no code reads this flag yet. */
#define FEATURE_PANEL_BUTTONS \
  1 /* The BAND, BW and MODE keys. 0 for a board with only the knob. */

/* ---------------------------------------------------------------------------
 * Pins. From the defines at the top of TEF6686_ESP32.ino in the PE5PVB
 * firmware. Pins 34, 35, 36 and 39 are input only on the ESP32.
 * ------------------------------------------------------------------------ */
#define PIN_ENCODER_A 34 /* Rotary encoder A. Input only pin. */
#define PIN_ENCODER_B 36 /* Rotary encoder B. Input only pin. */
#define PIN_ENCODER_BUTTON \
  39                        /* Encoder push. Held at power on means recovery. */
#define PIN_SQUELCH_ADC 35  /* ADC1, which keeps working with Wi-Fi on. */
#define PIN_BATTERY_ADC 13  /* Battery voltage divider. */
#define PIN_BUTTON_BAND 4   /* BAND button. Long press opens the RDS screen. */
#define PIN_BUTTON_BW 25    /* BW button, changes bandwidth. */
#define PIN_BUTTON_MODE 26  /* MODE button, cycles the tuning mode. */
#define PIN_BACKLIGHT_PWM 2 /* Display backlight brightness. */
#define PIN_STANDBY_LED 19  /* Also SPI MISO. Never driven by this code. */
#define PIN_SMETER_PWM 27   /* Analogue S meter, driven by PWM. */
#define PIN_TOUCH_IRQ 33    /* XPT2046 pen line, low while a finger is down. */
#define PIN_KEYPAD_IRQ 14   /* PCA9555 interrupt, a key was pressed. */
#define PIN_TUNER_XTAL_ADC 15 /* Says which crystal the tuner has. */

/* What the crystal sense pin reads for each crystal, and how far off a reading
 * may sit. From the PE5PVB firmware, which runs on this board. The tuner has
 * its own crystal, separate from the ESP32's 40 MHz one, and the same firmware
 * runs on boards with different ones, so this is read and never assumed. */
#define XTAL_ADC_0V 0          /* Near 0 means a 9.216 MHz crystal. */
#define XTAL_ADC_1V 1050       /* About 1V means a 12 MHz crystal. */
#define XTAL_ADC_2V 2250       /* About 2V means a 55 MHz crystal. */
#define XTAL_ADC_TOLERANCE 300 /* How far a reading may sit from those. */

/* Display, from TFT_eSPI/User_Setup.h in the PE5PVB fork. */
#define PIN_SPI_SCK 18  /* VSPI clock, the ESP32 default. */
#define PIN_SPI_MOSI 23 /* VSPI data out, the ESP32 default. */
/* VSPI data in, the ESP32 default. The touch chip answers on it. The pin is
 * also listed as the standby LED, so it is only ever an input here. */
#define PIN_SPI_MISO 19
#define PIN_TFT_CS 5    /* ILI9341 chip select. */
#define PIN_TFT_DC 17   /* ILI9341 data or command select. */
#define PIN_TFT_RST 16  /* ILI9341 reset. */
#define PIN_TOUCH_CS 32 /* XPT2046 chip select, its own line. */

/* The touch chip's raw x and y at the four corners of the picture, top
 * left, top right, bottom right and bottom left, read with a pen on one
 * ATS-125: the map used until a person calibrates their own. Another
 * radio's glass can sit some pixels off it. */
#define TOUCH_CAL_CORNERS {{223, 3886}, {243, 481}, {3901, 484}, {3894, 3900}}

/* I2C. Wire.begin() is called with no arguments, so these are the ESP32
 * defaults that the working firmware relies on. */
#define PIN_I2C_SDA 21 /* I2C data. The ESP32 default. */
#define PIN_I2C_SCL 22 /* I2C clock. The ESP32 default. */

/* I2C addresses. */
#define I2C_ADDR_TUNER 0x64  /* TEF6686 Lithio. */
#define I2C_ADDR_KEYPAD 0x20 /* PCA9555PW I/O expander. */
#define I2C_ADDR_RTC 0x32    /* RX8010, optional. Not used by the code yet. */

/* ---------------------------------------------------------------------------
 * Display size, so the UI layer never hard codes it.
 * ------------------------------------------------------------------------ */
#define DISPLAY_WIDTH 320  /* Panel width in pixels. */
#define DISPLAY_HEIGHT 240 /* Panel height in pixels. */

#endif /* BOARD_BOARD_ATS125_H */
