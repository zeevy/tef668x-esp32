/* The web server, as far as the state document's route needs it. */
#ifndef TEST_WEB_STATE_WEBSERVER_H
#define TEST_WEB_STATE_WEBSERVER_H

#include "Arduino.h"

enum HTTPMethod { HTTP_GET };

class WebServer {
 public:
  void on(const char *, HTTPMethod, void (*)(void)) {}
  void send(int, const char *, const String &) {}
};

#endif /* TEST_WEB_STATE_WEBSERVER_H */
