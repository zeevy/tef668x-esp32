/**
 * @file board_ats125.h
 * @brief Pin map and feature flags for the ATS-125.
 *
 * Every number here was read off the chip or taken from the working PE5PVB
 * firmware, and is written down with its source in HARDWARE.md. Nothing in
 * here is a guess. Anything still unconfirmed is marked.
 */
#ifndef BOARD_BOARD_ATS125_H
#define BOARD_BOARD_ATS125_H

/** Board id, lower case. Used in build output, JSON and file names. */
#define BOARD_NAME "ats125"

/** The same name as it is shown to a person, on screen and on the web page. */
#define BOARD_NAME_DISPLAY "ATS125"

/** mDNS name and the ArduinoOTA hostname, so the radio is tef668x.local. */
#define BOARD_HOSTNAME "tef668x"

/** SSID prefix for the setup access point. The last two MAC bytes follow. */
#define BOARD_AP_PREFIX "tef668x-setup"

/* ---------------------------------------------------------------------------
 * Feature flags. A flag at 0 means that code is not compiled and costs
 * nothing. Phase 0 is Wi-Fi and updates only, so everything else is off until
 * the phase that brings it in.
 * ------------------------------------------------------------------------ */
#define FEATURE_WIFI 1
#define FEATURE_OTA 1
#define FEATURE_WEB_UI 1
#define FEATURE_TELEMETRY 0
#define FEATURE_SPECTRUM 0
#define FEATURE_ALARM 0
#define FEATURE_TOUCH 0
#define FEATURE_RTC 0
#define FEATURE_AIR_BAND 0

/* ---------------------------------------------------------------------------
 * Pins. From the defines at the top of TEF6686_ESP32.ino in the PE5PVB
 * firmware. Pins 34, 35, 36 and 39 are input only on the ESP32.
 * ------------------------------------------------------------------------ */
#define PIN_ENCODER_A 34
#define PIN_ENCODER_B 36
#define PIN_ENCODER_BUTTON 39
#define PIN_SQUELCH_ADC 35 /**< ADC1, which keeps working with Wi-Fi on. */
#define PIN_BATTERY_ADC 13
#define PIN_BUTTON_BAND 4
#define PIN_BUTTON_BW 25
#define PIN_BUTTON_MODE 26
#define PIN_BACKLIGHT_PWM 2
#define PIN_STANDBY_LED 19 /**< Open: HARDWARE.md notes 19 is also SPI MISO. */
#define PIN_SMETER_PWM 27
#define PIN_TOUCH_IRQ 33
#define PIN_KEYPAD_IRQ 14

/* Display, from TFT_eSPI/User_Setup.h in the PE5PVB fork. */
#define PIN_TFT_CS 5
#define PIN_TFT_DC 17
#define PIN_TFT_RST 16
#define PIN_TOUCH_CS 32

/* I2C. Wire.begin() is called with no arguments, so these are the ESP32
 * defaults that the working firmware relies on. */
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22

/* I2C addresses. */
#define I2C_ADDR_TUNER 0x64   /**< TEF6686 Lithio. */
#define I2C_ADDR_KEYPAD 0x20  /**< PCA9555PW I/O expander. */
#define I2C_ADDR_RTC 0x32     /**< RX8010, optional, probed for. */

/* ---------------------------------------------------------------------------
 * Display size, so the UI layer never hard codes it.
 * ------------------------------------------------------------------------ */
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

#endif /* BOARD_BOARD_ATS125_H */
