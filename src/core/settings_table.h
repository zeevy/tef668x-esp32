/*
 * One row for every stored setting a person can change by number: the name
 * the API uses, where it sits in Settings and how wide it is, its range, and
 * when the radio acts on it.
 *
 * The API reads and writes through this table, so a setting is added as a
 * field in Settings and one row here, and the place it is stored and the
 * name it is given cannot drift apart. The tests check every row against
 * the struct and against settingsValid.
 */
#ifndef CORE_SETTINGS_TABLE_H
#define CORE_SETTINGS_TABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "settings.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  SETTING_ACTS_NOW = 0,
  /* Read only when the radio starts, so a change needs a restart. */
  SETTING_ACTS_AT_START,
  /* Fixed when a DX scan or DX mode starts, so a change waits for the next. */
  SETTING_ACTS_NEXT_DX,
} SettingActs;

typedef struct {
  const char *key; /* The name the API uses. */
  uint16_t offset; /* Where it sits in Settings. */
  uint8_t size;    /* 1 or 2 bytes. */
  bool isSigned;
  int32_t low;
  /* The highest the field may hold. For the two themes this is the widest a
   * byte holds: the caller bounds them by how many themes ship, the saved
   * indexes 0 to PALETTE_COUNT of core/palette.h. */
  int32_t high;
  SettingActs acts;
} SettingRow;

size_t settingsTableCount(void);

/* Row `i`, or NULL past the end. */
const SettingRow *settingsTableAt(size_t i);

/* The row with this key, or NULL. */
const SettingRow *settingsTableFind(const char *key);

/* The value of `row` in `s`, read at its own width and sign. */
int32_t settingsTableGet(const Settings *s, const SettingRow *row);

/* Write `value` into `row` in `s`, at its own width. The caller has checked
 * it against the row's range. */
void settingsTableSet(Settings *s, const SettingRow *row, int32_t value);

#ifdef __cplusplus
}
#endif

#endif /* CORE_SETTINGS_TABLE_H */
