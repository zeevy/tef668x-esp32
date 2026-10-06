/*
 * The few parts of the Arduino core the state document uses, for a build on
 * a PC. Only what web_state.cpp touches is here.
 */
#ifndef TEST_WEB_STATE_ARDUINO_H
#define TEST_WEB_STATE_ARDUINO_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <string>

/* F() marks text kept in flash. Here it is plain text with a type of its
 * own, as on the radio, so String has to take it the same way. */
class __FlashStringHelper;
#define F(text) (reinterpret_cast<const __FlashStringHelper *>(text))

/* Arduino's String, as far as the document needs it. A NULL is taken as
 * empty text, which is what the real one does. */
class String {
 public:
  String() {}
  String(const char *text) {
    *this += text;
  }
  explicit String(int v) : s_(std::to_string(v)) {}
  explicit String(unsigned v) : s_(std::to_string(v)) {}
  explicit String(long v) : s_(std::to_string(v)) {}
  explicit String(unsigned long v) : s_(std::to_string(v)) {}

  String &operator+=(const char *text) {
    if (text != NULL) {
      s_ += text;
    }
    return *this;
  }
  String &operator+=(const __FlashStringHelper *text) {
    return *this += reinterpret_cast<const char *>(text);
  }
  String &operator+=(long long v) {
    s_ += std::to_string(v);
    return *this;
  }
  String &operator+=(const String &other) {
    s_ += other.s_;
    return *this;
  }

  bool reserve(unsigned size) {
    s_.reserve(size);
    return true;
  }
  const char *c_str() const {
    return s_.c_str();
  }
  unsigned length() const {
    return (unsigned)s_.size();
  }

 private:
  std::string s_;
};

/* An hour, two minutes and three seconds since boot. */
static inline unsigned long millis(void) {
  return 3723000UL;
}

/* FreeRTOS: the bytes of the calling task's stack never used. */
typedef void *TaskHandle_t;
static inline unsigned uxTaskGetStackHighWaterMark(TaskHandle_t) {
  return 2600;
}

#endif /* TEST_WEB_STATE_ARDUINO_H */
