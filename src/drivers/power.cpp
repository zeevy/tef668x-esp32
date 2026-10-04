/* Deep sleep and the knob that wakes it. */
#include "power.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <esp_sleep.h>

#include "board/board.h"
#include "encoder.h"

/*
 * The longest the knob may still be held when sleep is asked for. The wake is
 * on the knob's pin being low, so sleeping while it is pressed wakes the
 * radio again at once. A hand still on it after this has been pressing on
 * purpose, and a wake straight away is then the right answer.
 */
#define POWER_RELEASE_WAIT_MS 5000

/* After the knob is let go, for its contacts to stop bouncing, or one bounce
 * low wakes the radio as it falls asleep. */
#define POWER_SETTLE_MS 100

void powerBegin(void) {
  gpio_deep_sleep_hold_dis();
  gpio_hold_dis((gpio_num_t)PIN_BACKLIGHT_PWM);
}

bool powerWokeFromSleep(void) {
  return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0;
}

void powerDeepSleep(void) {
  /* Off and held off. A pin left to itself in deep sleep floats, and the
   * panel light could glow on whatever it drifts to. */
  ledcDetach(PIN_BACKLIGHT_PWM);
  pinMode(PIN_BACKLIGHT_PWM, OUTPUT);
  digitalWrite(PIN_BACKLIGHT_PWM, LOW);
  gpio_hold_en((gpio_num_t)PIN_BACKLIGHT_PWM);
  gpio_deep_sleep_hold_en();

  const uint32_t from = millis();
  while (encoderButtonDown(PANEL_BUTTON_ENCODER) &&
         (uint32_t)(millis() - from) < POWER_RELEASE_WAIT_MS) {
    delay(10);
  }
  delay(POWER_SETTLE_MS);

  Serial.println(F("[power] asleep until the knob is pressed"));
  Serial.flush();
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_ENCODER_BUTTON, 0);
  esp_deep_sleep_start();
}
