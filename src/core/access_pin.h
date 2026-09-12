/**
 * @file access_pin.h
 * @brief The six digit PIN that guards anything which changes the radio.
 *
 * The default is 000000 and it is not a secret. A radio still on the default
 * says so on every boot and on its own web page, so nobody is left thinking
 * they are protected when they are not. Changing it is the user's call.
 *
 * The earlier design derived the default from the MAC address. That was
 * dropped because the MAC is in every frame the radio sends and the
 * derivation is in this repository, so the result was public while looking
 * private. A default that is obviously public is safer than one that is
 * secretly public.
 *
 * Nothing in here touches hardware, so it builds and is tested on a PC.
 */
#ifndef CORE_ACCESS_PIN_H
#define CORE_ACCESS_PIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Digits in a PIN. Six gives a million combinations against ten thousand. */
#define ACCESS_PIN_DIGITS 6

/** PIN values are 0 to 999999. */
#define ACCESS_PIN_MODULUS 1000000UL

/** Wrong attempts allowed before the gate locks. */
#define ACCESS_PIN_MAX_ATTEMPTS 5

/** How long the gate stays locked once it trips, in milliseconds. */
#define ACCESS_PIN_LOCKOUT_MS 60000UL

/** What a radio comes up with until someone changes it. */
#define ACCESS_PIN_DEFAULT 0UL

/**
 * True when this PIN is the one every radio ships with.
 *
 * The caller uses this to decide whether to warn. It is not a security check,
 * because the default PIN still works.
 *
 * @param pin  The PIN the radio is using.
 * @return true when it is still ACCESS_PIN_DEFAULT.
 */
bool accessPinIsDefault(uint32_t pin);

/**
 * Write a PIN out as six digits with leading zeros, plus a null terminator.
 *
 * @param pin  The PIN value. Anything at or above 1000000 is taken modulo it.
 * @param out  Buffer of at least ACCESS_PIN_DIGITS + 1 bytes.
 */
void accessPinFormat(uint32_t pin, char *out);

/**
 * Read six digits back into a PIN value.
 *
 * @param text  The text to read. Must be exactly six digits, nothing else.
 * @param out   Receives the value when the text parses.
 * @return true when the text was exactly six digits.
 */
bool accessPinParse(const char *text, uint32_t *out);

/**
 * Attempt counter for one client, so guessing costs time.
 *
 * Zero initialised is a valid unlocked gate.
 */
typedef struct {
  uint8_t wrong; /**< Wrong attempts since the last success or unlock. */
  uint32_t lockedUntilMs; /**< Millisecond count the lock expires at. */
  bool locked;            /**< Whether lockedUntilMs means anything yet. */
} AccessPinGate;

/**
 * Put a gate back to unlocked with no wrong attempts.
 *
 * @param gate  The gate to clear.
 */
void accessPinGateReset(AccessPinGate *gate);

/**
 * Ask whether the gate is refusing attempts right now.
 *
 * The comparison is wraparound safe, so it still behaves after the
 * millisecond counter rolls over at about 49 days.
 *
 * @param gate   The gate.
 * @param nowMs  Milliseconds since boot.
 * @return true while the gate is refusing attempts.
 */
bool accessPinGateLocked(const AccessPinGate *gate, uint32_t nowMs);

/**
 * How long the caller has to wait before the gate takes attempts again.
 *
 * @param gate   The gate.
 * @param nowMs  Milliseconds since boot.
 * @return Milliseconds left, or 0 when the gate is not locked.
 */
uint32_t accessPinGateRetryAfterMs(const AccessPinGate *gate, uint32_t nowMs);

/**
 * Check one PIN attempt and update the gate.
 *
 * A locked gate always returns false and does not count the attempt, so
 * hammering it cannot extend the lock forever.
 *
 * @param gate      The gate for this client.
 * @param expected  The PIN the radio is using.
 * @param given     The PIN the client sent.
 * @param nowMs     Milliseconds since boot.
 * @return true only when the gate was open and the PIN matched.
 */
bool accessPinGateCheck(AccessPinGate *gate, uint32_t expected, uint32_t given,
                        uint32_t nowMs);

#ifdef __cplusplus
}
#endif

#endif /* CORE_ACCESS_PIN_H */
