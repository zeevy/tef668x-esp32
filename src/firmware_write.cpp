#include "firmware_write.h"

#include "memory_store.h"
#include "radio_task.h"
#include "screen_task.h"
#include "settings_task.h"

void firmwareWriteBegin(void) {
  /*
   * What the radio is set to goes into flash first, while there is still a
   * radio to read it off, and a preset change still settling with it. The
   * automatic save waits ten seconds after the last thing moved, so a station
   * tuned just before a new image is sent would otherwise be lost at the
   * reboot. First, because the hush below parks the radio task and a parked
   * radio publishes nothing.
   */
  settingsTaskSaveNow();
  memoryStoreSaveNow();
  /*
   * Then the radio stops. It ramps the audio down, mutes, and stops the radio
   * task writing to the tuner at all, so the update does not play a station
   * right up to the reboot and end in a click.
   *
   * Nothing is lost by it. `radioHush` does not touch what the radio is set
   * to, only what the tuner is holding, so `radioResume` hands control back
   * and the task puts the station, the volume and a mute somebody set on
   * purpose back as they were.
   *
   * Before the panel, because the panel closes DX mode, which stops a DX scan
   * and lifts the scan's mute, and the channel it was on would be heard
   * until the hush. The panel says why the radio went quiet once the ramp
   * has run, a fraction of a second later.
   */
  radioHush();
  /* The panel goes over to the write. The loop task is inside the transfer
   * for the whole of it, so nothing else is going to draw anything until it
   * is over. The hold after it is so a failure stays on the screen long
   * enough to be read. */
  screenTaskUpdateBegin();
}

void firmwareWriteProgress(int percent) {
  screenTaskUpdateProgress(percent);
}

void firmwareWriteEnd(bool ok) {
  screenTaskUpdateEnd(ok);
  if (ok) {
    return;
  }
  /* The radio was hushed when the write started and there is now no reboot
   * coming, so it has to be let go again. Without this a write that breaks
   * halfway leaves a radio that answers every request, reports a station
   * and a good signal, and makes no sound. */
  radioResume();
}
