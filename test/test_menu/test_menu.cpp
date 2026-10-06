/* Tests for where the menu cursor goes. Runs on a PC. */
#include <unity.h>

#include <stdint.h>

#include "core/menu.h"

void setUp(void) {}
void tearDown(void) {}

/* Twelve groups, which is what the radio ships with. */
#define GROUPS 12

/* A shape for walking a list: nothing editable, just counts. */
static MenuShape listOf(uint8_t groups, uint8_t rows) {
  MenuShape s;
  s.groupCount = groups;
  s.rowCount = rows;
  s.kind = MENU_ROW_VALUE;
  s.min = 0;
  s.max = 100;
  s.step = 1;
  s.opensCount = 0;
  return s;
}

/* A shape for a value row with real limits. */
static MenuShape valueRow(int32_t min, int32_t max, int32_t step) {
  MenuShape s = listOf(GROUPS, 4);
  s.kind = MENU_ROW_VALUE;
  s.min = min;
  s.max = max;
  s.step = step;
  return s;
}

/* A shape for a row that opens a sub-group of `rows` rows. */
static MenuShape opensRow(uint8_t rows) {
  MenuShape s = listOf(GROUPS, 5);
  s.kind = MENU_ROW_OPENS;
  s.opensCount = rows;
  return s;
}

/* Open the menu and the first group, then move to `row` of its five. */
static void intoGroupRow(Menu *m, uint8_t row) {
  menuReset(m);
  menuOpen(m);
  MenuShape g = listOf(GROUPS, 5);
  menuPress(m, &g);
  menuTurn(m, row, &g);
}

static void a_row_opens_its_sub_group_and_back_returns_to_it(void) {
  Menu m;
  intoGroupRow(&m, 2);
  MenuShape opens = opensRow(3);
  TEST_ASSERT_EQUAL_INT(MENU_ENTERED_SUB, menuPress(&m, &opens));
  TEST_ASSERT_TRUE(m.inSub);
  TEST_ASSERT_EQUAL_UINT8(0, m.row);

  /* The sub-group's own three rows, and it stops at its last. */
  MenuShape inside = listOf(GROUPS, 3);
  TEST_ASSERT_EQUAL_INT(MENU_MOVED, menuTurn(&m, 5, &inside));
  TEST_ASSERT_EQUAL_UINT8(2, m.row);

  /* Back lands on the row that opened it, then back again leaves the group. */
  TEST_ASSERT_EQUAL_INT(MENU_LEFT_SUB, menuBack(&m));
  TEST_ASSERT_FALSE(m.inSub);
  TEST_ASSERT_EQUAL_UINT8(2, m.row);
  TEST_ASSERT_EQUAL_INT(MENU_LEFT_GROUP, menuBack(&m));
}

static void an_empty_sub_group_does_not_open(void) {
  Menu m;
  intoGroupRow(&m, 1);
  MenuShape empty = opensRow(0);
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuPress(&m, &empty));
  TEST_ASSERT_FALSE(m.inSub);
  TEST_ASSERT_EQUAL_UINT8(1, m.row);
}

static void an_edit_inside_a_sub_group_stays_in_it(void) {
  Menu m;
  intoGroupRow(&m, 3);
  MenuShape opens = opensRow(2);
  menuPress(&m, &opens);
  MenuShape value = valueRow(0, 10, 1);
  value.rowCount = 2;
  menuTurn(&m, 1, &value);
  m.value = 4;
  TEST_ASSERT_EQUAL_INT(MENU_EDIT_STARTED, menuPress(&m, &value));
  menuTurn(&m, 2, &value);
  TEST_ASSERT_EQUAL_INT(MENU_EDIT_KEPT, menuPress(&m, &value));
  TEST_ASSERT_TRUE(m.inSub);
  TEST_ASSERT_EQUAL_UINT8(1, m.row);

  /* A cancelled edit goes back to the sub-group too, not to the group. */
  menuPress(&m, &value);
  menuTurn(&m, 3, &value);
  TEST_ASSERT_EQUAL_INT(MENU_EDIT_UNDONE, menuBack(&m));
  TEST_ASSERT_EQUAL_INT32(6, m.value);
  TEST_ASSERT_TRUE(m.inSub);
  TEST_ASSERT_EQUAL_INT(MENU_LEFT_SUB, menuBack(&m));
  TEST_ASSERT_EQUAL_UINT8(3, m.row);
}

static void a_sub_group_does_not_open_inside_another(void) {
  Menu m;
  intoGroupRow(&m, 0);
  MenuShape opens = opensRow(4);
  menuPress(&m, &opens);
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuPress(&m, &opens));
  TEST_ASSERT_TRUE(m.inSub);
  TEST_ASSERT_EQUAL_UINT8(0, m.sub);
}

static void closing_and_opening_forget_the_sub_group(void) {
  Menu m;
  intoGroupRow(&m, 4);
  MenuShape opens = opensRow(2);
  menuPress(&m, &opens);
  TEST_ASSERT_EQUAL_INT(MENU_CLOSED_NOW, menuClose(&m));
  TEST_ASSERT_FALSE(m.inSub);
  TEST_ASSERT_EQUAL_INT(MENU_OPENED, menuOpen(&m));
  TEST_ASSERT_FALSE(m.inSub);
  TEST_ASSERT_EQUAL_UINT8(0, m.row);
}

static void it_starts_shut(void) {
  Menu m;
  menuReset(&m);
  TEST_ASSERT_FALSE(menuIsOpen(&m));
  TEST_ASSERT_FALSE(menuIsEditing(&m));

  /* And a knob turned with the menu shut belongs to the dial. */
  MenuShape s = listOf(GROUPS, 4);
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuTurn(&m, 3, &s));
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuPress(&m, &s));
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuBack(&m));
}

static void opening_lands_on_the_first_group(void) {
  Menu m;
  menuReset(&m);
  TEST_ASSERT_EQUAL_INT(MENU_OPENED, menuOpen(&m));
  TEST_ASSERT_TRUE(menuIsOpen(&m));
  TEST_ASSERT_EQUAL_UINT8(0, m.group);
  TEST_ASSERT_EQUAL_UINT8(0, m.row);

  /* Opening an open menu changes nothing, so a second press of the gesture
   * cannot put the cursor back to the top of a group being read. */
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuOpen(&m));
}

static void menu_open_alone_starts_at_the_top(void) {
  Menu m;
  MenuShape s = listOf(GROUPS, 4);
  menuReset(&m);
  menuOpen(&m);
  menuTurn(&m, 5, &s);
  TEST_ASSERT_EQUAL_UINT8(5, m.group);
  menuBack(&m);
  TEST_ASSERT_FALSE(menuIsOpen(&m));

  menuOpen(&m);
  TEST_ASSERT_EQUAL_UINT8(0, m.group);
}

static void the_group_list_stops_at_both_ends(void) {
  Menu m;
  MenuShape s = listOf(GROUPS, 4);
  menuReset(&m);
  menuOpen(&m);

  /* At the top already, so backwards is nothing at all. A list that wrapped
   * here would put the cursor on the last group, and then the only way to
   * know where the list ends is to have gone past it. */
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuTurn(&m, -1, &s));
  TEST_ASSERT_EQUAL_UINT8(0, m.group);

  TEST_ASSERT_EQUAL_INT(MENU_MOVED, menuTurn(&m, GROUPS - 1, &s));
  TEST_ASSERT_EQUAL_UINT8(GROUPS - 1, m.group);
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuTurn(&m, 1, &s));
  TEST_ASSERT_EQUAL_UINT8(GROUPS - 1, m.group);
}

static void a_fast_turn_stops_at_the_end(void) {
  Menu m;
  MenuShape s = listOf(GROUPS, 4);
  menuReset(&m);
  menuOpen(&m);

  /* Far more steps than there are groups, which an optical encoder hands
   * over on one fast turn. It lands on the last one and stays there. */
  menuTurn(&m, 23, &s);
  TEST_ASSERT_EQUAL_UINT8(GROUPS - 1, m.group);

  menuTurn(&m, -23, &s);
  TEST_ASSERT_EQUAL_UINT8(0, m.group);

  /* And a turn big enough to overflow a 32 bit add still stops at the end
   * rather than wrapping to the other one. */
  menuTurn(&m, 2147483647, &s);
  TEST_ASSERT_EQUAL_UINT8(GROUPS - 1, m.group);
  menuTurn(&m, -2147483647, &s);
  TEST_ASSERT_EQUAL_UINT8(0, m.group);
}

static void changing_group_puts_the_row_cursor_back(void) {
  Menu m;
  MenuShape s = listOf(GROUPS, 8);
  menuReset(&m);
  menuOpen(&m);
  menuPress(&m, &s);
  menuTurn(&m, 6, &s);
  TEST_ASSERT_EQUAL_UINT8(6, m.row);

  menuBack(&m);
  /* A group with fewer rows than the last one. Without the reset the cursor
   * would still be on row six and there is no row six here. */
  MenuShape small = listOf(GROUPS, 3);
  menuTurn(&m, 1, &small);
  TEST_ASSERT_EQUAL_UINT8(0, m.row);
}

static void an_empty_group_does_not_open(void) {
  Menu m;
  MenuShape none = listOf(GROUPS, 0);
  menuReset(&m);
  menuOpen(&m);
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuPress(&m, &none));
  TEST_ASSERT_EQUAL_INT(MENU_GROUPS, m.level);
}

static void a_press_opens_a_group_and_back_leaves_it(void) {
  Menu m;
  MenuShape s = listOf(GROUPS, 4);
  menuReset(&m);
  menuOpen(&m);
  menuTurn(&m, 2, &s);

  TEST_ASSERT_EQUAL_INT(MENU_ENTERED_GROUP, menuPress(&m, &s));
  TEST_ASSERT_EQUAL_INT(MENU_ROWS, m.level);
  TEST_ASSERT_EQUAL_UINT8(0, m.row);

  TEST_ASSERT_EQUAL_INT(MENU_LEFT_GROUP, menuBack(&m));
  TEST_ASSERT_EQUAL_INT(MENU_GROUPS, m.level);
  /* Back onto the group it came out of, not the top of the list. */
  TEST_ASSERT_EQUAL_UINT8(2, m.group);
}

static void the_row_list_stops_too(void) {
  Menu m;
  MenuShape s = listOf(GROUPS, 4);
  menuReset(&m);
  menuOpen(&m);
  menuPress(&m, &s);

  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuTurn(&m, -1, &s));
  TEST_ASSERT_EQUAL_UINT8(0, m.row);
  menuTurn(&m, 3, &s);
  TEST_ASSERT_EQUAL_UINT8(3, m.row);
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuTurn(&m, 1, &s));
  TEST_ASSERT_EQUAL_UINT8(3, m.row);
}

static void an_edit_runs_and_is_accepted(void) {
  Menu m;
  MenuShape s = valueRow(5, 100, 5);
  menuReset(&m);
  menuOpen(&m);
  menuPress(&m, &s);

  /* The caller puts what the setting is now into the menu before pressing. */
  m.value = 50;
  TEST_ASSERT_EQUAL_INT(MENU_EDIT_STARTED, menuPress(&m, &s));
  TEST_ASSERT_TRUE(menuIsEditing(&m));

  TEST_ASSERT_EQUAL_INT(MENU_EDIT_MOVED, menuTurn(&m, 2, &s));
  TEST_ASSERT_EQUAL_INT32(60, m.value);

  TEST_ASSERT_EQUAL_INT(MENU_EDIT_KEPT, menuPress(&m, &s));
  TEST_ASSERT_FALSE(menuIsEditing(&m));
  TEST_ASSERT_EQUAL_INT32(60, m.value);
  TEST_ASSERT_EQUAL_INT(MENU_ROWS, m.level);
}

static void cancelling_puts_the_old_value_back(void) {
  Menu m;
  MenuShape s = valueRow(5, 100, 5);
  menuReset(&m);
  menuOpen(&m);
  menuPress(&m, &s);
  m.value = 50;
  menuPress(&m, &s);
  menuTurn(&m, 6, &s);
  TEST_ASSERT_EQUAL_INT32(80, m.value);

  TEST_ASSERT_EQUAL_INT(MENU_EDIT_UNDONE, menuBack(&m));
  TEST_ASSERT_EQUAL_INT32(50, m.value);
  TEST_ASSERT_EQUAL_INT(MENU_ROWS, m.level);

  /* And a second cancel is not another undo. It leaves the group. */
  TEST_ASSERT_EQUAL_INT(MENU_LEFT_GROUP, menuBack(&m));
}

static void an_accepted_edit_cannot_be_undone_afterwards(void) {
  Menu m;
  MenuShape s = valueRow(0, 10, 1);
  menuReset(&m);
  menuOpen(&m);
  menuPress(&m, &s);
  m.value = 3;
  menuPress(&m, &s);
  menuTurn(&m, 4, &s);
  menuPress(&m, &s);
  TEST_ASSERT_EQUAL_INT32(7, m.value);

  /* Back out of the group and into it again. The accepted value stands. */
  menuBack(&m);
  menuPress(&m, &s);
  m.value = 7;
  menuPress(&m, &s);
  TEST_ASSERT_EQUAL_INT(MENU_EDIT_UNDONE, menuBack(&m));
  TEST_ASSERT_EQUAL_INT32(7, m.value);
}

static void a_value_stops_at_both_ends(void) {
  Menu m;
  MenuShape s = valueRow(5, 100, 5);
  menuReset(&m);
  menuOpen(&m);
  menuPress(&m, &s);
  m.value = 95;
  menuPress(&m, &s);

  TEST_ASSERT_EQUAL_INT(MENU_EDIT_MOVED, menuTurn(&m, 1, &s));
  TEST_ASSERT_EQUAL_INT32(100, m.value);
  /* At the top. Turning further changes nothing and says so, rather than
   * wrapping to 5 and leaving somebody looking at a black panel. */
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuTurn(&m, 1, &s));
  TEST_ASSERT_EQUAL_INT32(100, m.value);

  TEST_ASSERT_EQUAL_INT(MENU_EDIT_MOVED, menuTurn(&m, -100, &s));
  TEST_ASSERT_EQUAL_INT32(5, m.value);
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuTurn(&m, -1, &s));
  TEST_ASSERT_EQUAL_INT32(5, m.value);
}

static void a_fast_turn_of_a_big_step_cannot_overflow(void) {
  Menu m;
  /* A frequency in kHz, stepped by 1000, turned hard. The multiplication
   * overflows an int32_t long before it reaches the limit, and an overflow
   * would land the value at the far end of its range. */
  MenuShape s = valueRow(87500, 108000, 1000);
  menuReset(&m);
  menuOpen(&m);
  menuPress(&m, &s);
  m.value = 87500;
  menuPress(&m, &s);

  menuTurn(&m, 3000000, &s);
  TEST_ASSERT_EQUAL_INT32(108000, m.value);
  menuTurn(&m, -3000000, &s);
  TEST_ASSERT_EQUAL_INT32(87500, m.value);
}

static void a_step_of_zero_still_moves_by_one(void) {
  Menu m;
  MenuShape s = valueRow(0, 10, 0);
  menuReset(&m);
  menuOpen(&m);
  menuPress(&m, &s);
  m.value = 4;
  menuPress(&m, &s);
  menuTurn(&m, 1, &s);
  TEST_ASSERT_EQUAL_INT32(5, m.value);
}

static void a_row_that_cannot_change_is_not_edited(void) {
  Menu m;
  MenuShape s = valueRow(3, 3, 1);
  menuReset(&m);
  menuOpen(&m);
  menuPress(&m, &s);
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuPress(&m, &s));
  TEST_ASSERT_FALSE(menuIsEditing(&m));

  /* Same for a row that is only there to be read. */
  MenuShape info = valueRow(0, 100, 1);
  info.kind = MENU_ROW_INFO;
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuPress(&m, &info));
  TEST_ASSERT_FALSE(menuIsEditing(&m));
}

static void an_action_row_fires_and_stays_where_it_is(void) {
  Menu m;
  MenuShape s = valueRow(0, 100, 1);
  s.kind = MENU_ROW_ACTION;
  menuReset(&m);
  menuOpen(&m);
  menuPress(&m, &s);
  TEST_ASSERT_EQUAL_INT(MENU_FIRED, menuPress(&m, &s));
  /* Still on the row list, so a second restart is one press away and an
   * action never becomes an edit. */
  TEST_ASSERT_EQUAL_INT(MENU_ROWS, m.level);
  TEST_ASSERT_FALSE(menuIsEditing(&m));
}

static void back_walks_out_one_level_at_a_time(void) {
  Menu m;
  MenuShape s = valueRow(0, 10, 1);
  menuReset(&m);
  menuOpen(&m);
  menuPress(&m, &s);
  m.value = 5;
  menuPress(&m, &s);

  TEST_ASSERT_EQUAL_INT(MENU_EDIT_UNDONE, menuBack(&m));
  TEST_ASSERT_EQUAL_INT(MENU_LEFT_GROUP, menuBack(&m));
  TEST_ASSERT_EQUAL_INT(MENU_CLOSED_NOW, menuBack(&m));
  TEST_ASSERT_FALSE(menuIsOpen(&m));
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuBack(&m));
}

static void closing_from_outside_undoes_an_edit_in_flight(void) {
  Menu m;
  MenuShape s = valueRow(5, 100, 5);
  menuReset(&m);
  menuOpen(&m);
  menuPress(&m, &s);
  m.value = 40;
  menuPress(&m, &s);
  menuTurn(&m, 4, &s);
  TEST_ASSERT_EQUAL_INT32(60, m.value);

  /* A firmware update, or the gesture that opened it. The value was never
   * accepted, so it goes back. */
  TEST_ASSERT_EQUAL_INT(MENU_EDIT_UNDONE, menuClose(&m));
  TEST_ASSERT_EQUAL_INT32(40, m.value);
  TEST_ASSERT_FALSE(menuIsOpen(&m));
}

static void closing_from_outside_with_nothing_running_just_closes(void) {
  Menu m;
  MenuShape s = valueRow(0, 10, 1);
  menuReset(&m);
  menuOpen(&m);
  menuPress(&m, &s);
  TEST_ASSERT_EQUAL_INT(MENU_CLOSED_NOW, menuClose(&m));
  TEST_ASSERT_FALSE(menuIsOpen(&m));

  /* Closing a shut menu is not an event. */
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuClose(&m));
}

static void a_null_menu_asks_for_nothing(void) {
  MenuShape s = valueRow(0, 10, 1);
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuOpen(NULL));
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuTurn(NULL, 1, &s));
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuPress(NULL, &s));
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuBack(NULL));
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuClose(NULL));
  TEST_ASSERT_FALSE(menuIsOpen(NULL));
  TEST_ASSERT_FALSE(menuIsEditing(NULL));
  menuReset(NULL);

  /* And a turn with no shape, which is a caller that could not read its own
   * table. Doing nothing beats walking a cursor against numbers nobody
   * filled in. */
  Menu m;
  menuReset(&m);
  menuOpen(&m);
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuTurn(&m, 1, NULL));
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuPress(&m, NULL));
}

static void a_list_of_one_goes_nowhere(void) {
  Menu m;
  MenuShape s = listOf(1, 1);
  menuReset(&m);
  menuOpen(&m);
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuTurn(&m, 1, &s));
  TEST_ASSERT_EQUAL_UINT8(0, m.group);
  menuPress(&m, &s);
  TEST_ASSERT_EQUAL_INT(MENU_NOTHING, menuTurn(&m, -1, &s));
  TEST_ASSERT_EQUAL_UINT8(0, m.row);
}

static void a_short_list_never_scrolls(void) {
  /* Five rows visible, four in the group. The window stays at the top
   * whatever the cursor does and whatever the last group left behind. */
  TEST_ASSERT_EQUAL_UINT8(0, menuWindowTop(0, 4, 5, 0));
  TEST_ASSERT_EQUAL_UINT8(0, menuWindowTop(3, 4, 5, 0));
  TEST_ASSERT_EQUAL_UINT8(0, menuWindowTop(3, 4, 5, 2));
  TEST_ASSERT_EQUAL_UINT8(0, menuWindowTop(0, 5, 5, 3));
}

/* A page is a whole window on, or back, and stops with the last page full
 * or at the top. */
static void a_page_moves_a_whole_window_and_stops_at_the_ends(void) {
  TEST_ASSERT_EQUAL_UINT8(6, menuPageTop(0, 14, 6, 1));
  TEST_ASSERT_EQUAL_UINT8(8, menuPageTop(6, 14, 6, 1));
  TEST_ASSERT_EQUAL_UINT8(8, menuPageTop(8, 14, 6, 1));
  TEST_ASSERT_EQUAL_UINT8(2, menuPageTop(8, 14, 6, -1));
  TEST_ASSERT_EQUAL_UINT8(0, menuPageTop(2, 14, 6, -1));
  TEST_ASSERT_EQUAL_UINT8(0, menuPageTop(0, 6, 6, 1));
  TEST_ASSERT_EQUAL_UINT8(0, menuPageTop(3, 4, 6, -1));
}

static void the_window_moves_only_when_the_cursor_would_leave_it(void) {
  /* Nine rows, five visible. Walking down from the top: the window holds
   * still for the first five and then follows one row at a time. */
  uint8_t top = 0;
  for (uint8_t cursor = 0; cursor < 5; cursor++) {
    top = menuWindowTop(cursor, 9, 5, top);
    TEST_ASSERT_EQUAL_UINT8(0, top);
  }
  top = menuWindowTop(5, 9, 5, top);
  TEST_ASSERT_EQUAL_UINT8(1, top);
  top = menuWindowTop(6, 9, 5, top);
  TEST_ASSERT_EQUAL_UINT8(2, top);

  /* And back up. It holds until the cursor reaches the top row of the
   * window, then follows. */
  top = menuWindowTop(5, 9, 5, top);
  TEST_ASSERT_EQUAL_UINT8(2, top);
  top = menuWindowTop(2, 9, 5, top);
  TEST_ASSERT_EQUAL_UINT8(2, top);
  top = menuWindowTop(1, 9, 5, top);
  TEST_ASSERT_EQUAL_UINT8(1, top);
}

static void the_last_screen_of_a_list_is_full(void) {
  /* A fast turn to the end puts the cursor at 8 with the window still at 0.
   * The answer is 4, not 8, so the bottom of the list is a full screen rather
   * than one row and four blanks. */
  TEST_ASSERT_EQUAL_UINT8(4, menuWindowTop(8, 9, 5, 0));
  /* And a window that somehow sits past the end is pulled back. */
  TEST_ASSERT_EQUAL_UINT8(4, menuWindowTop(8, 9, 5, 7));
}

static void a_window_of_nothing_asks_for_nothing(void) {
  TEST_ASSERT_EQUAL_UINT8(0, menuWindowTop(3, 9, 0, 2));
  TEST_ASSERT_EQUAL_UINT8(0, menuWindowTop(0, 0, 5, 2));
}

static void the_whole_walk_runs_through(void) {
  /* Open, third group, second row, change it, keep it, and out. The order a
   * person actually uses, end to end. */
  Menu m;
  MenuShape s = listOf(GROUPS, 5);
  menuReset(&m);

  TEST_ASSERT_EQUAL_INT(MENU_OPENED, menuOpen(&m));
  TEST_ASSERT_EQUAL_INT(MENU_MOVED, menuTurn(&m, 2, &s));
  TEST_ASSERT_EQUAL_INT(MENU_ENTERED_GROUP, menuPress(&m, &s));
  TEST_ASSERT_EQUAL_INT(MENU_MOVED, menuTurn(&m, 1, &s));

  MenuShape row = valueRow(0, 240, 10);
  m.value = 100;
  TEST_ASSERT_EQUAL_INT(MENU_EDIT_STARTED, menuPress(&m, &row));
  TEST_ASSERT_EQUAL_INT(MENU_EDIT_MOVED, menuTurn(&m, -3, &row));
  TEST_ASSERT_EQUAL_INT32(70, m.value);
  TEST_ASSERT_EQUAL_INT(MENU_EDIT_KEPT, menuPress(&m, &row));

  TEST_ASSERT_EQUAL_INT(MENU_LEFT_GROUP, menuBack(&m));
  TEST_ASSERT_EQUAL_UINT8(2, m.group);
  TEST_ASSERT_EQUAL_INT(MENU_CLOSED_NOW, menuBack(&m));
  TEST_ASSERT_FALSE(menuIsOpen(&m));
}

/* Opened again, the menu is where it was left: a group's row, a sub-group's
 * row, an edit's row, or the group list; and only from a menu just opened. */
static void the_menu_opens_where_it_was_left(void) {
  Menu left;
  menuReset(&left);
  left.level = MENU_EDIT;
  left.group = 4;
  left.row = 7;
  left.inSub = true;
  left.sub = 2;

  Menu m;
  menuReset(&m);
  menuRestore(&m, &left);
  TEST_ASSERT_EQUAL(MENU_CLOSED, m.level); /* Not open: nothing. */
  TEST_ASSERT_EQUAL(MENU_OPENED, menuOpen(&m));
  menuRestore(&m, &left);
  TEST_ASSERT_EQUAL(MENU_ROWS, m.level);
  TEST_ASSERT_EQUAL_UINT8(4, m.group);
  TEST_ASSERT_EQUAL_UINT8(7, m.row);
  TEST_ASSERT_TRUE(m.inSub);
  TEST_ASSERT_EQUAL_UINT8(2, m.sub);

  left.level = MENU_GROUPS;
  left.inSub = false;
  menuReset(&m);
  menuOpen(&m);
  menuRestore(&m, &left);
  TEST_ASSERT_EQUAL(MENU_GROUPS, m.level);
  TEST_ASSERT_EQUAL_UINT8(4, m.group);

  menuRestore(NULL, &left);
  menuRestore(&m, NULL);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();

  RUN_TEST(it_starts_shut);
  RUN_TEST(opening_lands_on_the_first_group);
  RUN_TEST(menu_open_alone_starts_at_the_top);

  RUN_TEST(the_group_list_stops_at_both_ends);
  RUN_TEST(a_fast_turn_stops_at_the_end);
  RUN_TEST(changing_group_puts_the_row_cursor_back);
  RUN_TEST(an_empty_group_does_not_open);
  RUN_TEST(a_press_opens_a_group_and_back_leaves_it);
  RUN_TEST(the_row_list_stops_too);
  RUN_TEST(a_list_of_one_goes_nowhere);

  RUN_TEST(an_edit_runs_and_is_accepted);
  RUN_TEST(cancelling_puts_the_old_value_back);
  RUN_TEST(an_accepted_edit_cannot_be_undone_afterwards);
  RUN_TEST(a_value_stops_at_both_ends);
  RUN_TEST(a_fast_turn_of_a_big_step_cannot_overflow);
  RUN_TEST(a_step_of_zero_still_moves_by_one);
  RUN_TEST(a_row_that_cannot_change_is_not_edited);
  RUN_TEST(an_action_row_fires_and_stays_where_it_is);

  RUN_TEST(back_walks_out_one_level_at_a_time);
  RUN_TEST(a_row_opens_its_sub_group_and_back_returns_to_it);
  RUN_TEST(an_empty_sub_group_does_not_open);
  RUN_TEST(an_edit_inside_a_sub_group_stays_in_it);
  RUN_TEST(a_sub_group_does_not_open_inside_another);
  RUN_TEST(closing_and_opening_forget_the_sub_group);
  RUN_TEST(closing_from_outside_undoes_an_edit_in_flight);
  RUN_TEST(closing_from_outside_with_nothing_running_just_closes);

  RUN_TEST(a_short_list_never_scrolls);
  RUN_TEST(a_page_moves_a_whole_window_and_stops_at_the_ends);
  RUN_TEST(the_window_moves_only_when_the_cursor_would_leave_it);
  RUN_TEST(the_last_screen_of_a_list_is_full);
  RUN_TEST(a_window_of_nothing_asks_for_nothing);

  RUN_TEST(a_null_menu_asks_for_nothing);
  RUN_TEST(the_whole_walk_runs_through);
  RUN_TEST(the_menu_opens_where_it_was_left);

  return UNITY_END();
}
