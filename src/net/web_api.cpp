/*
 * The control API's shared reply helpers, and the call that registers every
 * `/api` route. The handlers are in one web_api_<resource>.cpp file each.
 */
#include "web_api_internal.h"
#include "web_internal.h"

#include "core/web_text.h"
#include "radio_task.h"

/* The server, the settings and the counts, handed over as the routes are
 * registered. */
static WebContext *sWeb = NULL;

void apiFail(int code, const String &why) {
  sWeb->server.send(code, "text/plain", why + "\n");
}

/* Refuse a width that is not one of the tuner's fixed FM widths, naming them
 * from the band plan's own list, so the reply cannot drift from the check. */
void apiFailFmWidth(const char *name) {
  String why = String(name) + " is one of the tuner's FM widths: ";
  const size_t count = bandBandwidthCount(BAND_FM);
  bool first = true;
  for (size_t i = 0; i < count; i++) {
    const uint16_t khz = bandBandwidthAt(BAND_FM, i);
    /* 0 is the tuner choosing for itself, not a width. */
    if (khz == 0) {
      continue;
    }
    if (!first) {
      why += i + 1 == count ? " or " : ", ";
    }
    why += (unsigned)khz;
    first = false;
  }
  apiFail(400, why + ".");
}

String apiDescribe(const RadioSettings *s) {
  char text[24];
  bandFormatWithUnit(s->band, s->freqKHz, text, sizeof(text));
  return String(bandName(s->band)) + " " + text;
}

/* The status for a command the radio did not carry out. 502 when the tuner
 * did not take it, since nothing was wrong with the request and sending it
 * again does no good; 400 when the radio refused it. */
int apiCommandStatus(RadioError why) {
  return why == RADIO_ERR_TUNER ? 502 : 400;
}

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
void apiSubmit(const RadioCommand *command, const String &said, ApiSay say) {
  RadioError why = RADIO_OK;
  RadioPostResult posted = radioPostAndSettle(command, API_SETTLE_MS, &why);
  if (posted == RADIO_POST_BUSY) {
    apiFail(503, "The radio is busy. Try again in a moment.");
    return;
  }
  if (posted == RADIO_POST_SLOW) {
    /* 202, not an error. The command is on the queue and will be carried out.
     * Calling this a failure would have the caller send it again, and the
     * radio would do it twice. */
    apiFail(202,
            "The radio took it but has not confirmed yet. Read "
            "/api/state to see where it got to.");
    return;
  }
  /* The radio's own verdict on this exact command, not a guess made before it
   * was sent. */
  if (why == RADIO_ERR_TUNER) {
    apiFail(502, String("Not done: ") + radioErrorText(why) + ".");
    return;
  }
  if (why != RADIO_OK) {
    apiFail(400, String("The radio refused it: ") + radioErrorText(why));
    return;
  }

  String answer = said;
  RadioSnapshot now;
  if (say != API_SAY_TEXT && radioGetSnapshot(&now)) {
    switch (say) {
      case API_SAY_TUNE:
        answer = apiDescribe(&now.settings);
        break;
      case API_SAY_BANDWIDTH:
        answer =
            now.settings.bandwidthKHz == 0
                ? String("bandwidth automatic")
                : String("bandwidth ") + now.settings.bandwidthKHz + " kHz";
        break;
      case API_SAY_MODE:
        answer = String("mode ") + tuneModeName(now.settings.tuneMode);
        break;
      case API_SAY_MUTE:
        answer = now.settings.muted ? String("muted") : String("unmuted");
        break;
      case API_SAY_FEATURES:
        /* Both named, on or off, so the reply says which of the four states
         * it landed in rather than only that something moved. */
        answer = String("iMS ") +
                 (now.settings.multipathSuppression ? "on" : "off") + ", EQ " +
                 (now.settings.equalizer ? "on" : "off");
        break;
      case API_SAY_TEXT:
      default:
        break;
    }
  }
  Serial.printf("[api] %s\n", answer.c_str());
  sWeb->server.send(200, "text/plain", answer + "\n");
}

bool apiNumber(const char *name, long *out, long low, long high) {
  if (!sWeb->server.hasArg(name)) {
    apiFail(400, String("Give ") + name + ".");
    return false;
  }
  const String raw = sWeb->server.arg(name);
  switch (webParseNumber(raw.c_str(), low, high, out)) {
    case WEB_NUMBER_OK:
      return true;
    case WEB_NUMBER_NOT_WHOLE:
      apiFail(400, String(name) + " has to be a whole number.");
      return false;
    case WEB_NUMBER_OUT_OF_RANGE:
    default:
      apiFail(400, String(name) + " has to be between " + low + " and " + high +
                       ".");
      return false;
  }
}

void webApiRegisterRoutes(WebContext *web) {
  sWeb = web;
  webApiTuneRoutes(web);
  webApiDxRoutes(web);
  webApiSettingsRoutes(web);
  webApiPresetsRoutes(web);
  webApiLogRoutes(web);
  webApiScreenRoutes(web);
  webApiTunerRoutes(web);
}
