/* Tests for the join decision. Runs on a PC. */
#include <unity.h>

#include <stdint.h>

#include "core/wifi_join.h"

void setUp(void) {}
void tearDown(void) {}

/* The radio as the caller sees it: credentials stored, nothing associated. */
static WifiJoinInput at(uint32_t nowMs) {
  WifiJoinInput in;
  in.nowMs = nowMs;
  in.hasCredentials = true;
  in.linkUp = false;
  in.apClients = 0;
  in.retryNow = false;
  in.hotspot = WIFI_HOTSPOT_AUTO;
  in.wifiOn = true;
  return in;
}

/* Take the radio to ONLINE the way it really gets there. */
static void bringOnline(WifiJoin *j, uint32_t nowMs) {
  wifiJoinReset(j);
  WifiJoinInput in = at(nowMs);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(j, &in));
  in.linkUp = true;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_CAME_ONLINE, wifiJoinStep(j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_ONLINE, j->state);
}

/* Take the radio to the access point by letting a join time out. */
static void bringToAp(WifiJoin *j, uint32_t nowMs) {
  wifiJoinReset(j);
  WifiJoinInput in = at(nowMs);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(j, &in));
  in.nowMs = nowMs + WIFI_JOIN_TIMEOUT_MS;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_ACCESS_POINT, j->state);
}

/* --------------------------------------------------------------- starting */

static void it_starts_by_trying_the_stored_network(void) {
  WifiJoin j;
  wifiJoinReset(&j);
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_OFFLINE, j.state);

  WifiJoinInput in = at(1000);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_JOINING, j.state);
}

static void with_no_credentials_it_goes_straight_to_the_access_point(void) {
  /* There is nothing to try and no way to be given anything to try except
   * the setup page, so waiting twenty seconds first would be twenty seconds
   * spent on a certainty. */
  WifiJoin j;
  wifiJoinReset(&j);
  WifiJoinInput in = at(1000);
  in.hasCredentials = false;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_ACCESS_POINT, j.state);
}

static void a_join_that_works_comes_online(void) {
  WifiJoin j;
  bringOnline(&j, 1000);
}

/* ---------------------------------------------------------- the timeout */

static void it_waits_the_whole_timeout_and_not_a_millisecond_less(void) {
  WifiJoin j;
  wifiJoinReset(&j);
  WifiJoinInput in = at(1000);
  wifiJoinStep(&j, &in);

  in.nowMs = 1000 + WIFI_JOIN_TIMEOUT_MS - 1;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_JOINING, j.state);

  in.nowMs = 1000 + WIFI_JOIN_TIMEOUT_MS;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_ACCESS_POINT, j.state);
}

static void a_join_that_lands_on_the_last_pass_still_counts(void) {
  /* The link is read before the clock, so a network that associates in the
   * same pass the timeout expires is joined rather than given up on. */
  WifiJoin j;
  wifiJoinReset(&j);
  WifiJoinInput in = at(1000);
  wifiJoinStep(&j, &in);

  in.nowMs = 1000 + WIFI_JOIN_TIMEOUT_MS + 5000;
  in.linkUp = true;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_CAME_ONLINE, wifiJoinStep(&j, &in));
}

static void nothing_happens_while_a_join_is_in_flight(void) {
  /* While a join is in flight the caller is told to do nothing, so it gets
   * on with drawing the panel and reading the knob. */
  WifiJoin j;
  wifiJoinReset(&j);
  WifiJoinInput in = at(1000);
  wifiJoinStep(&j, &in);
  for (uint32_t t = 1100; t < 1000 + WIFI_JOIN_TIMEOUT_MS; t += 100) {
    in.nowMs = t;
    TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  }
}

static void the_millis_wrap_does_not_make_a_join_immortal(void) {
  /* millis wraps every forty nine days. Compared rather than subtracted, a
   * join started just before the wrap would never time out and the radio
   * would sit in JOINING for ever with no access point. */
  WifiJoin j;
  wifiJoinReset(&j);
  WifiJoinInput in = at(0xFFFFFF00UL);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));

  in.nowMs = (uint32_t)(0xFFFFFF00UL + WIFI_JOIN_TIMEOUT_MS - 1);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));

  in.nowMs = (uint32_t)(0xFFFFFF00UL + WIFI_JOIN_TIMEOUT_MS);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(&j, &in));
}

/* ------------------------------------------------------------- retrying */

static void the_access_point_retries_on_the_interval(void) {
  WifiJoin j;
  bringToAp(&j, 1000);
  const uint32_t apAt = 1000 + WIFI_JOIN_TIMEOUT_MS;

  WifiJoinInput in = at(apAt + WIFI_AP_RETRY_INTERVAL_MS - 1);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));

  in.nowMs = apAt + WIFI_AP_RETRY_INTERVAL_MS;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_JOINING, j.state);
}

static void the_interval_is_time_on_the_access_point_not_time_joining(void) {
  /* The clock starts when the access point comes up, so twenty seconds of
   * failed join is not counted towards the five minutes. */
  WifiJoin j;
  bringToAp(&j, 1000);
  WifiJoinInput in = at(1000 + WIFI_AP_RETRY_INTERVAL_MS);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
}

static void a_retry_is_held_while_somebody_is_on_the_setup_page(void) {
  WifiJoin j;
  bringToAp(&j, 1000);
  const uint32_t due = 1000 + WIFI_JOIN_TIMEOUT_MS + WIFI_AP_RETRY_INTERVAL_MS;

  WifiJoinInput in = at(due);
  in.apClients = 1;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_ACCESS_POINT, j.state);

  /* And the hold pushes the next attempt a full interval out rather than
   * retrying the moment they close the page. */
  in.apClients = 0;
  in.nowMs = due + WIFI_AP_RETRY_INTERVAL_MS - 1;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  in.nowMs = due + WIFI_AP_RETRY_INTERVAL_MS;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));
}

static void the_station_count_is_only_wanted_when_a_retry_is_due(void) {
  WifiJoin j;
  bringToAp(&j, 1000);
  const uint32_t up = 1000 + WIFI_JOIN_TIMEOUT_MS;

  /* On the access point, but nowhere near the retry. Nothing to ask the
   * radio, which is the case that runs several hundred times a second. */
  TEST_ASSERT_FALSE(wifiJoinWantsApClients(&j, up, true));
  TEST_ASSERT_FALSE(
      wifiJoinWantsApClients(&j, up + WIFI_AP_RETRY_INTERVAL_MS - 1, true));

  /* Due, so the hold needs to know whether anybody is on the setup page. */
  TEST_ASSERT_TRUE(
      wifiJoinWantsApClients(&j, up + WIFI_AP_RETRY_INTERVAL_MS, true));
}

static void the_station_count_is_never_wanted_off_the_access_point(void) {
  WifiJoin j;
  wifiJoinReset(&j);
  TEST_ASSERT_FALSE(wifiJoinWantsApClients(&j, 1000, true));

  WifiJoinInput in = at(1000);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));
  TEST_ASSERT_FALSE(wifiJoinWantsApClients(&j, 1000, true));

  bringOnline(&j, 2000);
  TEST_ASSERT_FALSE(
      wifiJoinWantsApClients(&j, 2000 + WIFI_AP_RETRY_INTERVAL_MS, true));

  /* And not with a null decision, for the same reason nothing else acts on
   * one: the caller has not been started yet. */
  TEST_ASSERT_FALSE(wifiJoinWantsApClients(NULL, 1000, true));
}

static void the_station_count_is_not_wanted_with_nothing_to_retry(void) {
  /* An access point with no credentials stored never retries, so the count
   * would be read for ever and used for nothing. */
  WifiJoin j;
  wifiJoinReset(&j);
  WifiJoinInput in = at(1000);
  in.hasCredentials = false;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(&j, &in));

  TEST_ASSERT_FALSE(
      wifiJoinWantsApClients(&j, 1000 + WIFI_AP_RETRY_INTERVAL_MS, false));
}

static void the_access_point_never_retries_with_nothing_to_retry(void) {
  WifiJoin j;
  wifiJoinReset(&j);
  WifiJoinInput in = at(1000);
  in.hasCredentials = false;
  wifiJoinStep(&j, &in);

  in.nowMs = 1000 + WIFI_AP_RETRY_INTERVAL_MS * 10;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_ACCESS_POINT, j.state);
}

static void credentials_saved_are_tried_at_once(void) {
  /* Somebody has just typed a password. Making them wait out the interval to
   * find out whether it worked is the fault this flag exists to stop. */
  WifiJoin j;
  bringToAp(&j, 1000);
  WifiJoinInput in = at(1000 + WIFI_JOIN_TIMEOUT_MS + 1);
  in.retryNow = true;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_JOINING, j.state);
}

static void a_retry_asked_for_beats_the_hold(void) {
  /* The person on the setup page is the person who asked. */
  WifiJoin j;
  bringToAp(&j, 1000);
  WifiJoinInput in = at(1000 + WIFI_JOIN_TIMEOUT_MS + 1);
  in.retryNow = true;
  in.apClients = 1;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));
}

static void a_retry_asked_for_with_nothing_stored_is_ignored(void) {
  WifiJoin j;
  bringToAp(&j, 1000);
  WifiJoinInput in = at(1000 + WIFI_JOIN_TIMEOUT_MS + 1);
  in.retryNow = true;
  in.hasCredentials = false;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_ACCESS_POINT, j.state);
}

/* -------------------------------------------------------- staying joined */

static void a_single_bad_read_does_not_lose_the_network(void) {
  WifiJoin j;
  bringOnline(&j, 1000);
  WifiJoinInput in = at(2000);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_ONLINE, j.state);

  in.linkUp = true;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_STAY_ONLINE, wifiJoinStep(&j, &in));
  TEST_ASSERT_FALSE(j.dropping);
}

static void the_grace_period_is_time_and_not_a_number_of_reads(void) {
  /* The whole reason this is a duration. The caller runs it about five
   * hundred times a second, and it must mean the same if that ever changes. */
  WifiJoin j;
  bringOnline(&j, 1000);

  WifiJoinInput in = at(2000);
  for (int i = 0; i < 5000; i++) {
    /* Thousands of reads, all inside the grace period. */
    in.nowMs = 2000 + (uint32_t)(i % (int)WIFI_DROP_GRACE_MS);
    TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  }
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_ONLINE, j.state);

  /* And two reads are enough once the time has actually passed. */
  in.nowMs = 2000 + WIFI_DROP_GRACE_MS;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(&j, &in));
}

static void a_link_down_for_the_whole_grace_falls_back(void) {
  WifiJoin j;
  bringOnline(&j, 1000);

  WifiJoinInput in = at(2000);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  in.nowMs = 2000 + WIFI_DROP_GRACE_MS - 1;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  in.nowMs = 2000 + WIFI_DROP_GRACE_MS;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_ACCESS_POINT, j.state);
}

static void the_grace_starts_again_after_a_good_read(void) {
  /* Otherwise a router that blinks once an hour eventually adds up to a
   * fallback on a network that never actually went away. */
  WifiJoin j;
  bringOnline(&j, 1000);
  WifiJoinInput in = at(2000);
  for (int i = 0; i < 20; i++) {
    in.nowMs = 2000 + (uint32_t)i * WIFI_DROP_GRACE_MS;
    in.linkUp = false;
    wifiJoinStep(&j, &in);
    in.linkUp = true;
    wifiJoinStep(&j, &in);
  }
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_ONLINE, j.state);
}

static void the_grace_survives_the_millis_wrap(void) {
  WifiJoin j;
  const uint32_t nowMs = 0xFFFFFFFFUL - 200UL;
  bringOnline(&j, nowMs - 1000);

  WifiJoinInput in = at(nowMs);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  in.nowMs = (uint32_t)(nowMs + WIFI_DROP_GRACE_MS - 1);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  in.nowMs = (uint32_t)(nowMs + WIFI_DROP_GRACE_MS);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(&j, &in));
}

/* ------------------------------------------------------------- the edges */

static void an_access_point_that_will_not_start_backs_off(void) {
  /*
   * The caller puts the state back to OFFLINE when softAP fails, because a
   * radio claiming an access point that is not there is worse than one
   * saying it has nothing. It then lands here on the very next pass, two
   * milliseconds later, and without a back off that is a tight loop calling
   * softAP on the task this whole state machine exists to keep free.
   */
  WifiJoin j;
  wifiJoinReset(&j);
  WifiJoinInput in = at(1000);
  in.hasCredentials = false;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(&j, &in));

  /* What the caller does when softAP returns false. */
  j.state = WIFI_STATE_OFFLINE;

  for (uint32_t t = 1002; t < 1000 + WIFI_AP_RETRY_INTERVAL_MS; t += 2) {
    in.nowMs = t;
    TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  }
  in.nowMs = 1000 + WIFI_AP_RETRY_INTERVAL_MS;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(&j, &in));
}

static void boot_does_not_wait_for_the_back_off(void) {
  /* The same OFFLINE branch serves boot, and boot has nothing to back off
   * from. A radio that waited five minutes before its first join would be
   * offline for no reason. */
  WifiJoin j;
  wifiJoinReset(&j);
  WifiJoinInput in = at(0);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));

  wifiJoinReset(&j);
  in.nowMs = 12345;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));
}

static void a_null_argument_asks_for_nothing(void) {
  /* A caller that has not been started must not be told to tear the radio
   * down and put an access point up. */
  WifiJoin j;
  wifiJoinReset(&j);
  WifiJoinInput in = at(1000);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(NULL, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, NULL));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_OFFLINE, j.state);
  wifiJoinReset(NULL);
}

static void a_state_outside_the_enum_starts_again(void) {
  WifiJoin j;
  wifiJoinReset(&j);
  j.state = (WifiState)99;
  WifiJoinInput in = at(1000);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_OFFLINE, j.state);
}

static void the_whole_wrong_password_story_runs_through(void) {
  /* A wrong password from boot to the end, with the panel alive the whole
   * way. */
  WifiJoin j;
  wifiJoinReset(&j);

  /* Boot. The join starts and the caller is free immediately. */
  WifiJoinInput in = at(0);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));

  /* Twenty seconds of a working radio that is not on a network. */
  for (uint32_t t = 100; t < WIFI_JOIN_TIMEOUT_MS; t += 100) {
    in.nowMs = t;
    TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  }

  /* Then the setup page comes up. */
  in.nowMs = WIFI_JOIN_TIMEOUT_MS;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(&j, &in));

  /* Somebody joins it and fixes the password. */
  in.nowMs += 30000;
  in.apClients = 1;
  in.retryNow = true;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));

  /* This time it works. */
  in.nowMs += 3000;
  in.retryNow = false;
  in.apClients = 0;
  in.linkUp = true;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_CAME_ONLINE, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_ONLINE, j.state);
}

/* ---------------------------------------------------------- the hotspot */

/* On serves the hotspot at once with a network stored, and never leaves it
 * to try that network, not even when asked to. */
static void a_hotspot_set_on_serves_it_and_tries_nothing(void) {
  WifiJoin j;
  wifiJoinReset(&j);
  WifiJoinInput in = at(1000);
  in.hotspot = WIFI_HOTSPOT_ON;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_ACCESS_POINT, j.state);
  in.nowMs = 1000 + WIFI_AP_RETRY_INTERVAL_MS * 3;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  in.retryNow = true;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_ACCESS_POINT, j.state);
}

/* Set On while joined, it leaves the network for the hotspot. */
static void a_hotspot_set_on_leaves_the_network(void) {
  WifiJoin j;
  bringOnline(&j, 1000);
  WifiJoinInput in = at(2000);
  in.linkUp = true;
  in.hotspot = WIFI_HOTSPOT_ON;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(&j, &in));
}

/* Back to Auto from On, the stored network is tried at once. */
static void a_hotspot_back_to_auto_tries_the_network_at_once(void) {
  WifiJoin j;
  wifiJoinReset(&j);
  WifiJoinInput in = at(1000);
  in.hotspot = WIFI_HOTSPOT_ON;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(&j, &in));
  in.hotspot = WIFI_HOTSPOT_AUTO;
  in.retryNow = true;
  in.nowMs = 2000;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));
}

/* Off: a join that times out is begun again, never the hotspot. */
static void a_hotspot_set_off_keeps_trying_instead(void) {
  WifiJoin j;
  wifiJoinReset(&j);
  WifiJoinInput in = at(1000);
  in.hotspot = WIFI_HOTSPOT_OFF;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));
  in.nowMs = 1000 + WIFI_JOIN_TIMEOUT_MS;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_JOINING, j.state);
}

/* Off: a network lost past the grace is joined again, not left. */
static void a_hotspot_set_off_joins_a_lost_network_again(void) {
  WifiJoin j;
  bringOnline(&j, 1000);
  WifiJoinInput in = at(2000);
  in.hotspot = WIFI_HOTSPOT_OFF;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  in.nowMs = 2000 + WIFI_DROP_GRACE_MS;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));
}

/* Off with nothing stored: no network and no hotspot. */
static void a_hotspot_set_off_with_nothing_stored_stays_off(void) {
  WifiJoin j;
  wifiJoinReset(&j);
  WifiJoinInput in = at(1000);
  in.hasCredentials = false;
  in.hotspot = WIFI_HOTSPOT_OFF;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_OFFLINE, j.state);
}

/* Set Off while serving: the stored network is tried, or with none the
 * hotspot stops; set back to Auto, it comes up again on the next pass. */
static void a_hotspot_set_off_while_serving_stops_it(void) {
  WifiJoin j;
  bringToAp(&j, 1000);
  WifiJoinInput in = at(30000);
  in.hotspot = WIFI_HOTSPOT_OFF;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));

  wifiJoinReset(&j);
  in = at(1000);
  in.hasCredentials = false;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(&j, &in));
  in.hotspot = WIFI_HOTSPOT_OFF;
  in.nowMs = 2000;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_TURN_OFF, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_OFFLINE, j.state);
  in.hotspot = WIFI_HOTSPOT_AUTO;
  in.nowMs = 2001;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_START_AP, wifiJoinStep(&j, &in));
}

/* ------------------------------------------------------- Wi-Fi switched off */

/* Off at start: nothing is started, not even the hotspot. */
static void wifi_switched_off_starts_nothing(void) {
  WifiJoin j;
  wifiJoinReset(&j);
  WifiJoinInput in = at(1000);
  in.wifiOn = false;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  in.hasCredentials = false;
  in.hotspot = WIFI_HOTSPOT_ON;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_OFFLINE, j.state);
}

/* Switched off while joined, or while serving the hotspot, it turns off. */
static void wifi_switched_off_leaves_the_network_and_the_hotspot(void) {
  WifiJoin j;
  bringOnline(&j, 1000);
  WifiJoinInput in = at(2000);
  in.linkUp = true;
  in.wifiOn = false;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_TURN_OFF, wifiJoinStep(&j, &in));
  TEST_ASSERT_EQUAL_INT(WIFI_STATE_OFFLINE, j.state);
  TEST_ASSERT_EQUAL_INT(WIFI_DO_NOTHING, wifiJoinStep(&j, &in));
  bringToAp(&j, 1000);
  in = at(30000);
  in.wifiOn = false;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_TURN_OFF, wifiJoinStep(&j, &in));
}

/* Switched back on, the stored network is tried on the next pass. */
static void wifi_switched_back_on_joins_at_once(void) {
  WifiJoin j;
  bringOnline(&j, 1000);
  WifiJoinInput in = at(2000);
  in.wifiOn = false;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_TURN_OFF, wifiJoinStep(&j, &in));
  in.wifiOn = true;
  in.nowMs = 2001;
  TEST_ASSERT_EQUAL_INT(WIFI_DO_BEGIN_JOIN, wifiJoinStep(&j, &in));
}

int main(int, char **) {
  UNITY_BEGIN();

  RUN_TEST(it_starts_by_trying_the_stored_network);
  RUN_TEST(with_no_credentials_it_goes_straight_to_the_access_point);
  RUN_TEST(a_join_that_works_comes_online);

  RUN_TEST(it_waits_the_whole_timeout_and_not_a_millisecond_less);
  RUN_TEST(a_join_that_lands_on_the_last_pass_still_counts);
  RUN_TEST(nothing_happens_while_a_join_is_in_flight);
  RUN_TEST(the_millis_wrap_does_not_make_a_join_immortal);

  RUN_TEST(the_access_point_retries_on_the_interval);
  RUN_TEST(the_interval_is_time_on_the_access_point_not_time_joining);
  RUN_TEST(a_retry_is_held_while_somebody_is_on_the_setup_page);
  RUN_TEST(the_access_point_never_retries_with_nothing_to_retry);
  RUN_TEST(the_station_count_is_only_wanted_when_a_retry_is_due);
  RUN_TEST(the_station_count_is_never_wanted_off_the_access_point);
  RUN_TEST(the_station_count_is_not_wanted_with_nothing_to_retry);
  RUN_TEST(credentials_saved_are_tried_at_once);
  RUN_TEST(a_retry_asked_for_beats_the_hold);
  RUN_TEST(a_retry_asked_for_with_nothing_stored_is_ignored);

  RUN_TEST(a_single_bad_read_does_not_lose_the_network);
  RUN_TEST(the_grace_period_is_time_and_not_a_number_of_reads);
  RUN_TEST(a_link_down_for_the_whole_grace_falls_back);
  RUN_TEST(the_grace_starts_again_after_a_good_read);
  RUN_TEST(the_grace_survives_the_millis_wrap);

  RUN_TEST(an_access_point_that_will_not_start_backs_off);
  RUN_TEST(boot_does_not_wait_for_the_back_off);

  RUN_TEST(a_null_argument_asks_for_nothing);
  RUN_TEST(a_state_outside_the_enum_starts_again);
  RUN_TEST(the_whole_wrong_password_story_runs_through);

  RUN_TEST(a_hotspot_set_on_serves_it_and_tries_nothing);
  RUN_TEST(a_hotspot_set_on_leaves_the_network);
  RUN_TEST(a_hotspot_back_to_auto_tries_the_network_at_once);
  RUN_TEST(a_hotspot_set_off_keeps_trying_instead);
  RUN_TEST(a_hotspot_set_off_joins_a_lost_network_again);
  RUN_TEST(a_hotspot_set_off_with_nothing_stored_stays_off);
  RUN_TEST(a_hotspot_set_off_while_serving_stops_it);
  RUN_TEST(wifi_switched_off_starts_nothing);
  RUN_TEST(wifi_switched_off_leaves_the_network_and_the_hotspot);
  RUN_TEST(wifi_switched_back_on_joins_at_once);
  return UNITY_END();
}
