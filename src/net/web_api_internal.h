/*
 * What the web_api_*.cpp files share: the reply helpers every handler
 * uses, and each file's own routes.
 *
 * Not installed outside net/.
 */
#ifndef NET_WEB_API_INTERNAL_H
#define NET_WEB_API_INTERNAL_H

#include <Arduino.h>

#include "core/radio.h"
#include "web_internal.h"

/* Which part of the settled state a reply should describe. */
typedef enum {
  API_SAY_TEXT = 0,  /* Whatever the caller passed in. */
  API_SAY_TUNE,      /* The band and frequency it reached. */
  API_SAY_BANDWIDTH, /* The bandwidth it reached. */
  API_SAY_MODE,      /* The tuning mode it reached. */
  API_SAY_MUTE,      /* Whether it is muted. */
  API_SAY_FEATURES   /* Which of the four iMS and EQ states it reached. */
} ApiSay;

/* How long a request waits for the radio task to carry a command out. */
#define API_SETTLE_MS 500

void apiFail(int code, const String &why);

/* Refuse a width that is not one of the tuner's fixed FM widths, naming them
 * from the band plan's own list, so the reply cannot drift from the check. */
void apiFailFmWidth(const char *name);

String apiDescribe(const RadioSettings *s);

/* The status for a command the radio did not carry out. 502 when the tuner
 * did not take it, since nothing was wrong with the request and sending it
 * again does no good; 400 when the radio refused it. */
int apiCommandStatus(RadioError why);

/*
 * Check a command, carry it out, and say what happened.
 *
 * The one place a request turns into a command. The simple command endpoints
 * end here, so they cannot drift apart in how they validate or what they
 * report.
 *
 * It waits for the radio to finish. Without that wait a script that sends two
 * requests back to back gets the second one judged against the state before
 * the first, which refuses things that are allowed and reports frequencies the
 * radio is not on.
 */
void apiSubmit(const RadioCommand *command, const String &said,
               ApiSay say = API_SAY_TEXT);

bool apiNumber(const char *name, long *out, long low, long high);

/* The `/api` routes of web_api_tune.cpp. */
void webApiTuneRoutes(WebContext *web);

/* The `/api` routes of web_api_dx.cpp. */
void webApiDxRoutes(WebContext *web);

/* The `/api` routes of web_api_settings.cpp. */
void webApiSettingsRoutes(WebContext *web);

/* The `/api` routes of web_api_presets.cpp. */
void webApiPresetsRoutes(WebContext *web);

/* The `/api` routes of web_api_log.cpp. */
void webApiLogRoutes(WebContext *web);

/* The `/api` routes of web_api_screen.cpp. */
void webApiScreenRoutes(WebContext *web);

/* The `/api` routes of web_api_tuner.cpp. */
void webApiTunerRoutes(WebContext *web);

#endif /* NET_WEB_API_INTERNAL_H */
