/**
 * @file access_pin.c
 * @brief Implementation of the access PIN and its attempt gate.
 */
#include "access_pin.h"

bool accessPinIsDefault(uint32_t pin) {
  return pin == ACCESS_PIN_DEFAULT;
}

void accessPinFormat(uint32_t pin, char *out) {
  pin %= ACCESS_PIN_MODULUS;
  for (int i = ACCESS_PIN_DIGITS - 1; i >= 0; i--) {
    out[i] = (char)('0' + (pin % 10));
    pin /= 10;
  }
  out[ACCESS_PIN_DIGITS] = '\0';
}

bool accessPinParse(const char *text, uint32_t *out) {
  if (text == NULL || out == NULL) {
    return false;
  }
  uint32_t value = 0;
  size_t i = 0;
  for (; i < ACCESS_PIN_DIGITS; i++) {
    if (text[i] < '0' || text[i] > '9') {
      return false;
    }
    value = value * 10U + (uint32_t)(text[i] - '0');
  }
  if (text[i] != '\0') {
    return false;
  }
  *out = value;
  return true;
}

void accessPinGateReset(AccessPinGate *gate) {
  gate->wrong = 0;
  gate->lockedUntilMs = 0;
  gate->locked = false;
}

bool accessPinGateLocked(const AccessPinGate *gate, uint32_t nowMs) {
  if (!gate->locked) {
    return false;
  }
  /* Signed difference, so the answer stays right across the 49 day wrap. */
  return (int32_t)(nowMs - gate->lockedUntilMs) < 0;
}

uint32_t accessPinGateRetryAfterMs(const AccessPinGate *gate, uint32_t nowMs) {
  if (!accessPinGateLocked(gate, nowMs)) {
    return 0;
  }
  return gate->lockedUntilMs - nowMs;
}

bool accessPinGateCheck(AccessPinGate *gate, uint32_t expected, uint32_t given,
                        uint32_t nowMs) {
  if (accessPinGateLocked(gate, nowMs)) {
    return false;
  }
  if (gate->locked) {
    /* The lock has run out. Start the client again with a clean count. */
    accessPinGateReset(gate);
  }
  if (expected == given) {
    accessPinGateReset(gate);
    return true;
  }
  if (gate->wrong < 255) {
    gate->wrong++;
  }
  if (gate->wrong >= ACCESS_PIN_MAX_ATTEMPTS) {
    gate->locked = true;
    gate->lockedUntilMs = nowMs + ACCESS_PIN_LOCKOUT_MS;
  }
  return false;
}
