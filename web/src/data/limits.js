// The radio's own limits, as its firmware has them.

// Preset slots, and the letters a preset's name keeps.
export const PRESET_SLOTS = 99;
export const PRESET_NAME_MAX = 16;
// Station log entries, and the letters an entry's name and note keep.
export const LOG_MAX = 250;
export const LOG_NAME_MAX = 8;
export const LOG_NOTE_MAX = 32;
// The volume, in dB.
export const VOL_MIN = -60;
export const VOL_MAX = 0;
// The battery's empty and full, in mV: the radio draws the per cent in a
// straight line between the two.
export const BATTERY_EMPTY_MV = 3000;
export const BATTERY_FULL_MV = 4200;
// Web pages it sends the live state to at once, and PCs on the PC Link.
export const LIVE_PAGES_MAX = 2;
export const PC_LINK_MAX = 3;
