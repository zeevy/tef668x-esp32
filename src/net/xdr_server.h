/*
 * The PC Link: XDR-GTK and FM-DX Webserver over TCP port 7373.
 *
 * Loop task only, like the web server.
 */
#ifndef NET_XDR_SERVER_H
#define NET_XDR_SERVER_H

#include <stddef.h>
#include <stdint.h>

#include "core/settings.h"

/* How many PCs may be connected at once. Sending one line to a PC holds the
 * loop about 2.6 ms, up to 4.2, so three cost up to about 9 ms every 66 ms:
 * XDR-GTK, FM-DX Webserver and one more. */
#define XDR_CLIENTS_MAX 3

/* Keep the live settings, which say whether the link is on. */
void xdrServerBegin(const Settings *settings);

/* Open or close the port as the settings say, take new PCs, read what they
 * sent, and send what is due. */
void xdrServerLoop(void);

/* Whether the port is open. */
bool xdrServerListening(void);

/* How many PCs have signed in with the PIN. */
uint8_t xdrServerClients(void);

/* The address of the `index`th signed in PC, as text. False past the last. */
bool xdrServerClientAddress(uint8_t index, char *out, size_t cap);

/* Close every PC's connection, so each has to sign in again: the access PIN
 * has changed. */
void xdrServerSignOutAll(void);

/* How many PCs were turned away because XDR_CLIENTS_MAX were connected. */
uint32_t xdrServerRefused(void);

#endif /* NET_XDR_SERVER_H */
