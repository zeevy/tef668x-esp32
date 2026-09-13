/*
 * ArduinoOTA, so a development flash needs no cable and no person.
 *
 * This is the path behind
 * `pio run -e ats125 -t upload --upload-port <ip>`.
 */
#ifndef NET_OTA_SERVICE_H
#define NET_OTA_SERVICE_H

#include <Arduino.h>

void otaBegin(const char *password);

void otaLoop(void);

bool otaInProgress(void);

#endif /* NET_OTA_SERVICE_H */
