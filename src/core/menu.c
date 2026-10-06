/* Where the menu cursor is. No drawing, no settings, no hardware. */
#include "menu.h"

#include <stddef.h>

/*
 * Walk a cursor along a list, and stop at the ends.
 *
 * It does not wrap, although the dial does at a band edge. A list that wraps
 * has no bottom, so turning the knob to find the last row takes you past it
 * and back to the top, and the only way to know you have reached the end is
 * to overshoot it. The dial is a circle and a list is not.
 *
 * Worked out in 64 bits, because the knob reports what it counted since the
 * last look and a fast turn hands over tens of steps: `at + steps` in 32 bits
 * would wrap round to the far end of the list, which is the one thing this is
 * here to stop.
 */
static uint8_t walk(uint8_t at, int32_t steps, uint8_t count) {
  if (count == 0) {
    return 0;
  }
  int64_t to = (int64_t)at + (int64_t)steps;
  if (to < 0) {
    to = 0;
  }
  if (to > (int64_t)count - 1) {
    to = (int64_t)count - 1;
  }
  return (uint8_t)to;
}

/* Move a value inside its limits, and say whether it actually moved. */
static bool nudge(int32_t *value, int32_t steps, const MenuShape *shape) {
  int32_t step = shape->step != 0 ? shape->step : 1;
  /*
   * Worked out in 64 bits and then brought back.
   *
   * A fast turn of an optical encoder hands over tens of steps, and a step
   * size can be a thousand for a frequency, so the multiplication overflows
   * an int32_t long before anything reaches the limits below. An overflow
   * here would wrap the value to the far end of its range, which is exactly
   * the jump to black this file says a value must never make.
   */
  int64_t wanted = (int64_t)*value + (int64_t)steps * (int64_t)step;
  if (wanted < shape->min) {
    wanted = shape->min;
  }
  if (wanted > shape->max) {
    wanted = shape->max;
  }
  if ((int32_t)wanted == *value) {
    return false;
  }
  *value = (int32_t)wanted;
  return true;
}

void menuReset(Menu *m) {
  if (m == NULL) {
    return;
  }
  m->level = MENU_CLOSED;
  m->group = 0;
  m->row = 0;
  m->inSub = false;
  m->sub = 0;
  m->value = 0;
  m->was = 0;
}

MenuResult menuOpen(Menu *m) {
  if (m == NULL) {
    return MENU_NOTHING;
  }
  if (m->level != MENU_CLOSED) {
    return MENU_NOTHING;
  }
  m->level = MENU_GROUPS;
  m->group = 0;
  m->row = 0;
  m->inSub = false;
  m->sub = 0;
  return MENU_OPENED;
}

MenuResult menuTurn(Menu *m, int32_t steps, const MenuShape *shape) {
  if (m == NULL || shape == NULL || steps == 0) {
    return MENU_NOTHING;
  }
  switch (m->level) {
    case MENU_GROUPS: {
      uint8_t to = walk(m->group, steps, shape->groupCount);
      if (to == m->group) {
        return MENU_NOTHING;
      }
      m->group = to;
      /* The row cursor goes back to the top with it. Rows belong to a group,
       * so carrying the old position into the next group would land the
       * cursor on a row nobody chose, and on a shorter group, off the end. */
      m->row = 0;
      return MENU_MOVED;
    }
    case MENU_ROWS: {
      uint8_t to = walk(m->row, steps, shape->rowCount);
      if (to == m->row) {
        return MENU_NOTHING;
      }
      m->row = to;
      return MENU_MOVED;
    }
    case MENU_EDIT:
      if (!nudge(&m->value, steps, shape)) {
        return MENU_NOTHING;
      }
      return MENU_EDIT_MOVED;
    case MENU_CLOSED:
    default:
      return MENU_NOTHING;
  }
}

MenuResult menuPress(Menu *m, const MenuShape *shape) {
  if (m == NULL || shape == NULL) {
    return MENU_NOTHING;
  }
  switch (m->level) {
    case MENU_GROUPS:
      if (shape->rowCount == 0) {
        /* A group with nothing in it does not open. It can happen while a
         * group is being built, and a menu that opens onto an empty list
         * leaves a person pressing a knob that does nothing. */
        return MENU_NOTHING;
      }
      m->level = MENU_ROWS;
      m->row = 0;
      return MENU_ENTERED_GROUP;
    case MENU_ROWS:
      if (shape->kind == MENU_ROW_ACTION) {
        return MENU_FIRED;
      }
      if (shape->kind == MENU_ROW_OPENS) {
        /* An empty sub-group does not open, for the same reason an empty
         * group does not. A sub-group inside a sub-group is not offered: the
         * caller never describes one, and this would only stack one level. */
        if (shape->opensCount == 0 || m->inSub) {
          return MENU_NOTHING;
        }
        m->sub = m->row;
        m->row = 0;
        m->inSub = true;
        return MENU_ENTERED_SUB;
      }
      if (shape->kind == MENU_ROW_INFO || shape->min >= shape->max) {
        /* Nothing to change. Reading it is the whole of what it does. */
        return MENU_NOTHING;
      }
      m->level = MENU_EDIT;
      /* The caller has put what the setting is now into `value` before this
       * call. Both are kept, so a cancel has something to go back to. */
      m->was = m->value;
      return MENU_EDIT_STARTED;
    case MENU_EDIT:
      m->level = MENU_ROWS;
      m->was = m->value;
      return MENU_EDIT_KEPT;
    case MENU_CLOSED:
    default:
      return MENU_NOTHING;
  }
}

MenuResult menuBack(Menu *m) {
  if (m == NULL) {
    return MENU_NOTHING;
  }
  switch (m->level) {
    case MENU_EDIT:
      m->level = MENU_ROWS;
      m->value = m->was;
      /* The caller writes `value` back to the radio. It is the old one
       * again, because most rows apply the new one as the knob turns. */
      return MENU_EDIT_UNDONE;
    case MENU_ROWS:
      if (m->inSub) {
        /* Back on the row that opened it, so a person who looked inside and
         * came out is where they were, not at the top of the group. */
        m->inSub = false;
        m->row = m->sub;
        return MENU_LEFT_SUB;
      }
      m->level = MENU_GROUPS;
      return MENU_LEFT_GROUP;
    case MENU_GROUPS:
      m->level = MENU_CLOSED;
      return MENU_CLOSED_NOW;
    case MENU_CLOSED:
    default:
      return MENU_NOTHING;
  }
}

MenuResult menuClose(Menu *m) {
  if (m == NULL || m->level == MENU_CLOSED) {
    return MENU_NOTHING;
  }
  bool undoing = m->level == MENU_EDIT;
  m->level = MENU_CLOSED;
  m->inSub = false;
  if (undoing) {
    m->value = m->was;
    return MENU_EDIT_UNDONE;
  }
  return MENU_CLOSED_NOW;
}

void menuRestore(Menu *m, const Menu *left) {
  if (m == NULL || left == NULL || m->level != MENU_GROUPS) {
    return;
  }
  m->group = left->group;
  m->row = left->row;
  m->inSub = left->inSub;
  m->sub = left->sub;
  m->level = left->level == MENU_GROUPS ? MENU_GROUPS : MENU_ROWS;
}

uint8_t menuWindowTop(uint8_t cursor, uint8_t count, uint8_t visible,
                      uint8_t top) {
  if (visible == 0 || count == 0) {
    return 0;
  }
  if (count <= visible) {
    /* It all fits, so there is nothing to scroll and the window is the top of
     * the list. Without this a window left over from a longer group would
     * hold a short one part way down with blank rows under it. */
    return 0;
  }
  if (cursor < top) {
    top = cursor;
  } else if (cursor >= (uint8_t)(top + visible)) {
    top = (uint8_t)(cursor - visible + 1);
  }
  /* A window that would run past the end is pulled back, so the last screen
   * of a list is full rather than padded with nothing. */
  if (top > (uint8_t)(count - visible)) {
    top = (uint8_t)(count - visible);
  }
  return top;
}

uint8_t menuPageTop(uint8_t top, uint8_t count, uint8_t visible, int dir) {
  if (count <= visible) {
    return 0;
  }
  /* Back to the page boundary at or before the row above the window, so
   * the pages back are the pages forward: from a last page pulled up to be
   * full, back lands on the page it was pulled up from. */
  const int to = dir < 0 ? ((int)top - 1) / visible * visible : top + visible;
  const int last = (int)count - visible;
  return (uint8_t)(to < 0 ? 0 : to > last ? last : to);
}

int32_t menuBarValue(int32_t at, int32_t width, int32_t stub, int32_t low,
                     int32_t high) {
  if (width <= 0 || at <= stub || high <= low) {
    return low;
  }
  if (at >= width) {
    return high;
  }
  return low + (int32_t)(((int64_t)(high - low) * at + width / 2) / width);
}

int32_t menuBarClicks(int32_t now, int32_t want, int32_t step, int32_t low,
                      int32_t high) {
  if (step <= 0) {
    step = 1;
  }
  const int32_t off = want - now;
  /* Rounded away from zero at an end, so the turn reaches past it and is
   * held there; to the nearest step elsewhere. */
  const int32_t round = want <= low || want >= high ? step - 1 : step / 2;
  return (off + (off < 0 ? -round : round)) / step;
}

bool menuIsOpen(const Menu *m) {
  return m != NULL && m->level != MENU_CLOSED;
}

bool menuIsEditing(const Menu *m) {
  return m != NULL && m->level == MENU_EDIT;
}
