/**
 * @file test_access_pin.cpp
 * @brief Tests for the access PIN and its attempt gate. Runs on a PC.
 */
#include <unity.h>

#include "core/access_pin.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void the_default_pin_is_all_zeros(void) {
  TEST_ASSERT_EQUAL_UINT32(0, ACCESS_PIN_DEFAULT);
  TEST_ASSERT_TRUE(accessPinIsDefault(ACCESS_PIN_DEFAULT));
  TEST_ASSERT_TRUE(accessPinIsDefault(0));
  TEST_ASSERT_FALSE(accessPinIsDefault(1));
  TEST_ASSERT_FALSE(accessPinIsDefault(999999));
}

static void the_default_pin_still_has_to_be_typed_in_full(void) {
  /* The default is public, but it is not a way past the gate without being
   * given. An empty or short entry is still refused. */
  uint32_t value = 0;
  TEST_ASSERT_FALSE(accessPinParse("", &value));
  TEST_ASSERT_FALSE(accessPinParse("0", &value));
  TEST_ASSERT_TRUE(accessPinParse("000000", &value));
  TEST_ASSERT_EQUAL_UINT32(ACCESS_PIN_DEFAULT, value);
}

static void formatting_keeps_the_leading_zeros(void) {
  char out[ACCESS_PIN_DIGITS + 1];
  accessPinFormat(42, out);
  TEST_ASSERT_EQUAL_STRING("000042", out);
  accessPinFormat(999999, out);
  TEST_ASSERT_EQUAL_STRING("999999", out);
  accessPinFormat(0, out);
  TEST_ASSERT_EQUAL_STRING("000000", out);
}

static void formatting_wraps_a_value_that_is_too_big(void) {
  char out[ACCESS_PIN_DIGITS + 1];
  accessPinFormat(1000007, out);
  TEST_ASSERT_EQUAL_STRING("000007", out);
}

static void parsing_takes_six_digits_and_nothing_else(void) {
  uint32_t value = 0;
  TEST_ASSERT_TRUE(accessPinParse("000042", &value));
  TEST_ASSERT_EQUAL_UINT32(42, value);
  TEST_ASSERT_FALSE(accessPinParse("42", &value));
  TEST_ASSERT_FALSE(accessPinParse("0000420", &value));
  TEST_ASSERT_FALSE(accessPinParse("00 042", &value));
  TEST_ASSERT_FALSE(accessPinParse("12345a", &value));
  TEST_ASSERT_FALSE(accessPinParse(NULL, &value));
}

static void a_right_pin_opens_the_gate(void) {
  AccessPinGate gate;
  accessPinGateReset(&gate);
  TEST_ASSERT_TRUE(accessPinGateCheck(&gate, 123456, 123456, 1000));
  TEST_ASSERT_FALSE(accessPinGateLocked(&gate, 1000));
}

static void four_wrong_attempts_do_not_lock_the_gate(void) {
  AccessPinGate gate;
  accessPinGateReset(&gate);
  for (int i = 0; i < ACCESS_PIN_MAX_ATTEMPTS - 1; i++) {
    TEST_ASSERT_FALSE(accessPinGateCheck(&gate, 123456, 111111, 1000));
  }
  TEST_ASSERT_FALSE(accessPinGateLocked(&gate, 1000));
  TEST_ASSERT_TRUE(accessPinGateCheck(&gate, 123456, 123456, 1000));
}

static void the_fifth_wrong_attempt_locks_the_gate(void) {
  AccessPinGate gate;
  accessPinGateReset(&gate);
  for (int i = 0; i < ACCESS_PIN_MAX_ATTEMPTS; i++) {
    TEST_ASSERT_FALSE(accessPinGateCheck(&gate, 123456, 111111, 1000));
  }
  TEST_ASSERT_TRUE(accessPinGateLocked(&gate, 1000));
  /* Even the right PIN is refused while the lock holds. */
  TEST_ASSERT_FALSE(accessPinGateCheck(&gate, 123456, 123456, 1000));
}

static void a_right_attempt_clears_the_wrong_count(void) {
  AccessPinGate gate;
  accessPinGateReset(&gate);
  for (int i = 0; i < ACCESS_PIN_MAX_ATTEMPTS - 1; i++) {
    accessPinGateCheck(&gate, 123456, 111111, 1000);
  }
  TEST_ASSERT_TRUE(accessPinGateCheck(&gate, 123456, 123456, 1000));
  /* Four more wrong attempts must still not lock, because the count reset. */
  for (int i = 0; i < ACCESS_PIN_MAX_ATTEMPTS - 1; i++) {
    accessPinGateCheck(&gate, 123456, 111111, 1000);
  }
  TEST_ASSERT_FALSE(accessPinGateLocked(&gate, 1000));
}

static void the_lock_is_exactly_one_minute(void) {
  AccessPinGate gate;
  accessPinGateReset(&gate);
  const uint32_t now = 1000;
  for (int i = 0; i < ACCESS_PIN_MAX_ATTEMPTS; i++) {
    accessPinGateCheck(&gate, 123456, 111111, now);
  }
  /* One millisecond before the minute is up, still locked. */
  TEST_ASSERT_TRUE(accessPinGateLocked(&gate, now + ACCESS_PIN_LOCKOUT_MS - 1));
  /* On the boundary, open. */
  TEST_ASSERT_FALSE(accessPinGateLocked(&gate, now + ACCESS_PIN_LOCKOUT_MS));
  TEST_ASSERT_FALSE(
      accessPinGateLocked(&gate, now + ACCESS_PIN_LOCKOUT_MS + 1));
}

static void hammering_a_locked_gate_does_not_extend_the_lock(void) {
  AccessPinGate gate;
  accessPinGateReset(&gate);
  const uint32_t now = 1000;
  for (int i = 0; i < ACCESS_PIN_MAX_ATTEMPTS; i++) {
    accessPinGateCheck(&gate, 123456, 111111, now);
  }
  uint32_t lockedUntil = now + ACCESS_PIN_LOCKOUT_MS;
  for (int i = 0; i < 50; i++) {
    accessPinGateCheck(&gate, 123456, 111111, now + 100);
  }
  TEST_ASSERT_FALSE(accessPinGateLocked(&gate, lockedUntil));
}

static void the_gate_opens_again_after_the_lock_runs_out(void) {
  AccessPinGate gate;
  accessPinGateReset(&gate);
  const uint32_t now = 1000;
  for (int i = 0; i < ACCESS_PIN_MAX_ATTEMPTS; i++) {
    accessPinGateCheck(&gate, 123456, 111111, now);
  }
  TEST_ASSERT_TRUE(
      accessPinGateCheck(&gate, 123456, 123456, now + ACCESS_PIN_LOCKOUT_MS));
}

static void the_gate_still_works_across_the_millisecond_wrap(void) {
  AccessPinGate gate;
  accessPinGateReset(&gate);
  /* Thirty seconds before the counter wraps, so the lock expires after it. */
  const uint32_t now = 0xFFFFFFFFUL - 30000UL;
  for (int i = 0; i < ACCESS_PIN_MAX_ATTEMPTS; i++) {
    accessPinGateCheck(&gate, 123456, 111111, now);
  }
  /* Ten seconds later the counter has not wrapped and the lock holds. */
  TEST_ASSERT_TRUE(accessPinGateLocked(&gate, now + 10000UL));
  /* Forty seconds later the counter has wrapped but the minute is not up, so
   * the lock still holds. This is the case a naive comparison gets wrong. */
  TEST_ASSERT_TRUE(accessPinGateLocked(&gate, now + 40000UL));
  /* Seventy seconds later the minute is up and the lock is over. */
  TEST_ASSERT_FALSE(accessPinGateLocked(&gate, now + 70000UL));
}

static void the_retry_wait_counts_down(void) {
  AccessPinGate gate;
  accessPinGateReset(&gate);
  const uint32_t now = 1000;
  TEST_ASSERT_EQUAL_UINT32(0, accessPinGateRetryAfterMs(&gate, now));
  for (int i = 0; i < ACCESS_PIN_MAX_ATTEMPTS; i++) {
    accessPinGateCheck(&gate, 123456, 111111, now);
  }
  TEST_ASSERT_EQUAL_UINT32(ACCESS_PIN_LOCKOUT_MS,
                           accessPinGateRetryAfterMs(&gate, now));
  TEST_ASSERT_EQUAL_UINT32(ACCESS_PIN_LOCKOUT_MS - 5000,
                           accessPinGateRetryAfterMs(&gate, now + 5000));
  TEST_ASSERT_EQUAL_UINT32(
      0, accessPinGateRetryAfterMs(&gate, now + ACCESS_PIN_LOCKOUT_MS));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(the_default_pin_is_all_zeros);
  RUN_TEST(the_default_pin_still_has_to_be_typed_in_full);
  RUN_TEST(formatting_keeps_the_leading_zeros);
  RUN_TEST(formatting_wraps_a_value_that_is_too_big);
  RUN_TEST(parsing_takes_six_digits_and_nothing_else);
  RUN_TEST(a_right_pin_opens_the_gate);
  RUN_TEST(four_wrong_attempts_do_not_lock_the_gate);
  RUN_TEST(the_fifth_wrong_attempt_locks_the_gate);
  RUN_TEST(a_right_attempt_clears_the_wrong_count);
  RUN_TEST(the_lock_is_exactly_one_minute);
  RUN_TEST(hammering_a_locked_gate_does_not_extend_the_lock);
  RUN_TEST(the_gate_opens_again_after_the_lock_runs_out);
  RUN_TEST(the_gate_still_works_across_the_millisecond_wrap);
  RUN_TEST(the_retry_wait_counts_down);
  return UNITY_END();
}
