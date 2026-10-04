/*
 * Updates from GitHub releases: the check once the radio is on the network,
 * and the install when somebody says yes.
 *
 * The deciding is in core/update_check.h. This is the doing: the HTTPS
 * fetch of the manifest, and the download of the image into the free slot
 * through firmware_write.h, as a browser upload does. The fetch runs in a
 * short task of its own, so the screen and the knob go on answering; the
 * download runs on the loop task, which it blocks while the screen shows its
 * progress, as a browser upload does.
 *
 * Nothing here draws. The loop asks `updateCheckTakeOffer` and has the menu
 * task open the offer.
 */
#ifndef NET_UPDATE_CHECK_H
#define NET_UPDATE_CHECK_H

#include <stdbool.h>
#include <stdint.h>

#include "../core/settings.h"

typedef enum {
  UPDATE_STATE_OFF = 0,  /* The setting is off and nothing was found. */
  UPDATE_STATE_WAITING,  /* On, and not checked yet in this start. */
  UPDATE_STATE_CHECKING, /* The check is running now. */
  UPDATE_STATE_NONE,     /* Checked: nothing newer, or no release at all. */
  UPDATE_STATE_FOUND,    /* Checked: a newer release for this board. */
  UPDATE_STATE_FAILED    /* The check could not be made, or the manifest was
                           refused. Tried again at the next start. */
} UpdateState;

/* What asking for the install led to. */
typedef enum {
  UPDATE_INSTALL_STARTING = 0, /* It runs on the loop's next pass. */
  UPDATE_INSTALL_NOTHING,      /* No newer release was found. */
  UPDATE_INSTALL_ON_TRIAL,     /* A new firmware is still proving itself, and
                                  a restart now would undo it. */
  UPDATE_INSTALL_BUSY          /* A station scan is running. */
} UpdateInstallAsk;

/* Before the first updateCheckLoop. `settings` is the live struct, read for
 * the Check for Updates setting each pass. */
void updateCheckBegin(const Settings *settings);

/* Once a loop pass. Starts the check when it is due, takes its result when
 * it ends, and carries out an install that was asked for. `busy` is true
 * while a scan or a sweep runs, or the menu or another screen is up, and the
 * check waits for it. A good install restarts the radio and does not
 * return. */
void updateCheckLoop(bool busy);

UpdateState updateCheckState(void);

/* True while the check is out on the network. A band scan, a DX scan and a
 * level sweep are refused then, because the Wi-Fi transmitting raises the
 * level the tuner reads by up to 25 dB, which would show stations that are
 * not there. */
bool updateCheckRunning(void);

/* "off", "wait", "checking", "none", "found" or "failed", for
 * /api/state. */
const char *updateCheckStateName(void);

/* The newer version, such as "0.2.0", and its image size, while the state is
 * found. NULL and 0 otherwise. */
const char *updateCheckVersion(void);
uint32_t updateCheckSize(void);

/* True once, the first time it is asked after a newer release was found, so
 * the offer is shown once and Later means later. Asked only while the radio
 * screen is up on its own, so the offer never closes another screen. */
bool updateCheckTakeOffer(void);

/* Ask for the found update to be installed. The menu, the System page and
 * the API all come here. */
UpdateInstallAsk updateCheckInstall(void);

#endif /* NET_UPDATE_CHECK_H */
