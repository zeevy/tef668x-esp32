/*
 * ArduinoOTA, so a development flash needs no cable and no person.
 *
 * This is the path behind
 * `pio run -e ats125 -t upload --upload-port <ip>`.
 */
#ifndef NET_OTA_SERVICE_H
#define NET_OTA_SERVICE_H

#include <Arduino.h>

/* Start the listener. The access PIN is the password, six digits. */
void otaBegin(uint32_t pin);

/*
 * Make a new access PIN the OTA password, at once.
 *
 * Without this the PIN set at boot stays the OTA password until the next
 * restart, so after a PIN change the old one could still flash firmware.
 */
void otaSetPin(uint32_t pin);

/*
 * Put the OTA service into mDNS.
 *
 * ArduinoOTA's own mDNS handling is off (see `otaBegin`), so this is the only
 * thing that registers `_arduino._tcp`. `wifi_manager.cpp` calls it each time
 * it starts the responder, which is each time the network comes up. Without
 * it the radio answers to its name on the web page and cannot be found by
 * name by the uploader, and the fallback of flashing by address hides it.
 *
 * The port and the auth flag are ArduinoOTA's own, which is why this lives
 * here rather than as two numbers in `wifi_manager.cpp`.
 */
void otaAdvertise(void);

/* `open` false closes the listener, for the web server switched off in the
 * settings; true opens it again, unless wrong PINs have closed it. */
void otaLoop(bool open);

bool otaInProgress(void);

#endif /* NET_OTA_SERVICE_H */
