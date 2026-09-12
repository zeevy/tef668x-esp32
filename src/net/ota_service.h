/**
 * @file ota_service.h
 * @brief ArduinoOTA, so a development flash needs no cable and no person.
 *
 * This is the path behind
 * `pio run -e ats125 -t upload --upload-port <ip>`.
 */
#ifndef NET_OTA_SERVICE_H
#define NET_OTA_SERVICE_H

#include <Arduino.h>

/**
 * Start the ArduinoOTA listener.
 *
 * @param password  Required from the uploader. Pass the access PIN, so there
 *                  is one secret on the radio rather than two.
 */
void otaBegin(const char *password);

/** Service the listener. Call from the main loop. */
void otaLoop(void);

/**
 * True while an image is being written, so nothing else reboots the radio.
 *
 * @return true between the start and the end of a transfer.
 */
bool otaInProgress(void);

#endif /* NET_OTA_SERVICE_H */
