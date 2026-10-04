/*
 * Where the menu cursor is, and what each gesture does to it.
 *
 * All of the deciding and none of the drawing, for the reason
 * `core/update_screen.h` and `core/wifi_join.h` give: this is a state machine
 * with four states and eleven ways out of them, and every one of its mistakes
 * is silent. A cancel that does not restore, a cursor that walks off the end
 * of a list, an edit that stays running after the menu closes: none of them
 * fail, none of them look wrong in a build, and all of them can be tested on
 * a PC in a millisecond.
 *
 * A group's list can hold a row that opens a list of its own, a sub-group,
 * one level deep and no more: the panel's header names the list a person is
 * in and the one it came from, and a third level would need a path the
 * header has no room for.
 *
 * **It knows nothing about what is in the menu.** No names, no settings, no
 * units, no LVGL. The caller says how many groups there are, how many rows
 * are in the group being looked at, and what kind of row the cursor is on,
 * and this answers with where the cursor went and what the caller should do
 * about it. That is what keeps the menu's contents a table in the layer that
 * knows about radios, the same way the screens are built from a table of
 * panels: describe what goes where, never hard code the picture.
 *
 * The value being edited is a plain `int32_t` here. Every setting on this
 * radio is a number, a choice out of a list, or a switch, and all three are
 * one number as far as a cursor is concerned. What that number means is the
 * caller's business.
 */
#ifndef CORE_MENU_H
#define CORE_MENU_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Where the cursor is. */
typedef enum {
  MENU_CLOSED = 0, /* Not up. The radio screen owns the panel. */
  MENU_GROUPS,     /* Choosing which group to open. */
  MENU_ROWS,       /* Moving down the rows of one group. */
  MENU_EDIT,       /* Changing the value on one row. */
} MenuLevel;

/* What sort of row the cursor is on. */
typedef enum {
  MENU_ROW_VALUE = 0, /* A number, or a choice out of a list. Editable. */
  MENU_ROW_ACTION,    /* Does something when pressed. Save a preset, scan FM. */
  MENU_ROW_INFO,      /* Read only. The version, the address. */
  MENU_ROW_OPENS,     /* Opens a sub-group: a list of rows of its own. */
} MenuRowKind;

/*
 * What the caller must tell the menu before every call.
 *
 * Filled in from the caller's own table. It is passed on each call rather
 * than stored, because the answer changes as the cursor moves: the row count
 * belongs to whichever group is open, and the limits belong to whichever row
 * the cursor is on.
 *
 * `min`, `max` and `step` are only read for a `MENU_ROW_VALUE`. A row with
 * `min` equal to `max` cannot be edited, which is how a setting that is not
 * available on this band is left in place and inert rather than removed from
 * the list. A list that rearranges itself is one nobody can learn.
 */
typedef struct {
  uint8_t groupCount; /* How many groups there are. */
  uint8_t rowCount;   /* How many rows in the group the cursor is in. */
  MenuRowKind kind;   /* What the row under the cursor is. */
  int32_t min;        /* The lowest the value may go. */
  int32_t max;        /* The highest. */
  int32_t step;       /* What one click of the knob changes it by. */
  /* For a MENU_ROW_OPENS row, how many rows the sub-group holds. */
  uint8_t opensCount;
} MenuShape;

/*
 * What the caller should do about what just happened.
 *
 * One answer per call, never a set of flags, because two things never happen
 * on one gesture and a caller that has to check five booleans in the right
 * order is a caller that will check them in the wrong one.
 */
typedef enum {
  MENU_NOTHING = 0,   /* Nothing moved. Redraw nothing. */
  MENU_MOVED,         /* The cursor moved. Redraw. */
  MENU_OPENED,        /* The menu is now up. Build the screen. */
  MENU_CLOSED_NOW,    /* The menu is gone. Give the panel back. */
  MENU_ENTERED_GROUP, /* A group was opened. */
  MENU_LEFT_GROUP,    /* Back to the group list. */
  MENU_ENTERED_SUB,   /* A sub-group was opened from a row of its group. */
  MENU_LEFT_SUB,      /* Back to the group, on the row the sub-group is. */
  MENU_EDIT_STARTED,  /* An edit began. `value` is what it was. */
  MENU_EDIT_MOVED,    /* The value changed. Apply it now, so it can be seen. */
  MENU_EDIT_KEPT,     /* The edit was accepted. The value stands. */
  MENU_EDIT_UNDONE,   /* Cancelled. `value` is what to put back. */
  MENU_FIRED,         /* An action row was pressed. */
} MenuResult;

/* Everything the menu remembers between gestures. */
typedef struct {
  MenuLevel level;
  uint8_t group; /* Which group the cursor is on, or which is open. */
  /* Which row, inside the list being looked at: the open group's, or the
   * open sub-group's while `inSub` is set. */
  uint8_t row;
  bool inSub;  /* The list being looked at is a sub-group. */
  uint8_t sub; /* The group's row that opened it, where back returns to. */
  /*
   * The value as it stands now, and as it was before the edit began.
   *
   * Both are kept because a value on this radio is applied as the knob turns
   * rather than when the edit is accepted: a brightness can only be chosen by
   * looking at the panel set to it. So cancelling has to put the old one back
   * on the radio, not merely stop editing, and nothing else remembers it.
   */
  int32_t value;
  int32_t was;
} Menu;

/* Shut, with the cursor at the top. Call it once before anything else. */
void menuReset(Menu *m);

/*
 * Open the menu at the group list.
 *
 * The cursor goes back to the first group every time rather than resuming
 * where it was left. A menu that opens somewhere different depending on what
 * was done last cannot be learned by muscle memory, and finding your way back
 * to the top costs one gesture.
 */
MenuResult menuOpen(Menu *m);

/*
 * Turn the knob.
 *
 * `steps` is signed and may be more than one, because the encoder reports
 * what it has counted since the last look rather than one click at a time.
 *
 * Nothing wraps. A list stops at its first and last row, and a value stops at
 * its limits.
 *
 * The dial wraps at a band edge and a list does not, because a list that
 * wraps has no bottom: finding the last row means going past it and coming
 * back to the top, and the only way to know you have reached the end is to
 * overshoot. The dial is a circle and a list is not.
 *
 * A value that wrapped would take the brightness from 100 to 5 on one click,
 * which is how somebody ends up looking at a black panel wondering what they
 * broke.
 */
MenuResult menuTurn(Menu *m, int32_t steps, const MenuShape *shape);

/*
 * A short press of the knob.
 *
 * In the group list it opens the group. On a row that opens a sub-group it
 * opens it, unless it is empty. On a value row it starts the edit, and on the
 * row being edited it accepts. On an action row it fires.
 */
MenuResult menuPress(Menu *m, const MenuShape *shape);

/*
 * A long press of the knob.
 *
 * It is the way back, at every level: cancel the edit, leave the sub-group
 * for the row that opened it, leave the group, close the menu. One gesture
 * that always means "not this", so nobody has to remember which button goes
 * back from where.
 */
MenuResult menuBack(Menu *m);

/*
 * Close the menu from outside, wherever it is.
 *
 * For the gesture that opened it and for anything else that needs the panel,
 * a firmware update among them. An edit in flight is cancelled rather than
 * kept: the value was never accepted, and a radio that quietly keeps a
 * half turned setting because something else interrupted is worse than one
 * that puts it back.
 *
 * Answers `MENU_EDIT_UNDONE` when there was an edit to undo, so the caller
 * still writes the old value back, and `MENU_CLOSED_NOW` otherwise.
 */
MenuResult menuClose(Menu *m);

/*
 * Put a menu just opened back where `left` was when it last closed: the same
 * group, and the same row of it or of its sub-group. An edit that was open
 * comes back as its row, since it was undone when the menu closed. Nothing
 * happens unless `m` is open on its group list, as menuOpen leaves it. The
 * caller checks the row still exists, since a list can have shrunk.
 */
void menuRestore(Menu *m, const Menu *left);

/*
 * Which row the list should start drawing at.
 *
 * The panel holds six rows and a group can have more, so the list scrolls.
 * The rule is the least surprising one: the window does not move until the
 * cursor would leave it, and then it moves by exactly as much as it has to.
 * A list that recentres on every click makes the whole screen move when one
 * line of it changed, and reading it becomes work.
 *
 * `top` is where the window is now, and the answer is where it should be.
 * Kept here rather than in the screen because it is arithmetic with three
 * edge cases, and every one of them is invisible until somebody has a group
 * with seven rows in it.
 */
uint8_t menuWindowTop(uint8_t cursor, uint8_t count, uint8_t visible,
                      uint8_t top);

/* Whether the menu owns the panel. */
bool menuIsOpen(const Menu *m);

/* Whether a value is being changed this moment. */
bool menuIsEditing(const Menu *m);

#ifdef __cplusplus
}
#endif

#endif /* CORE_MENU_H */
