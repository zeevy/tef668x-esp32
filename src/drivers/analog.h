/**
 * @file analog.h
 * @brief The two analogue things on this board: the pot in, the meter out.
 *
 * The pot is on ADC1, which is the half of the ESP32's converter that keeps
 * working while Wi-Fi is on. ADC2 does not, and this radio is never without
 * Wi-Fi, so that is not a detail to lose.
 *
 * How a reading becomes a volume is in core/input.h, where it can be tested.
 * This file only reads the pin.
 */
#ifndef DRIVERS_ANALOG_H
#define DRIVERS_ANALOG_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Set the pot pin and the meter up.
 *
 * The meter is left reading zero, so a needle does not sit wherever it was
 * left while the radio works out what to show.
 */
void analogBegin(void);

/**
 * Read the pot, averaged.
 *
 * Several reads averaged, because a single one of this converter moves by a
 * few counts with nothing touching the knob. The deadband that decides
 * whether that is real movement is in core/input.h.
 *
 * @return 0 to 4095.
 */
uint16_t potRead(void);

/**
 * Point the analogue S-meter at a signal level.
 *
 * The scale is the working PE5PVB firmware's: `51 * (10^(dBuV/100) - 1)`,
 * held at the top above 100 dBuV, and off when the level is not positive.
 * That curve is not decoration. A meter driven straight from the level in dB
 * sits near the bottom for everything a person actually listens to, because
 * dB are already logarithmic and the needle then compresses the interesting
 * part into a few degrees of travel.
 *
 * @param levelTenths  The signal level in tenths of a dBuV.
 */
void smeterShow(int16_t levelTenths);

/**
 * Whether this board appears to have a meter fitted.
 *
 * Nothing can be read back from a meter, so this only says whether the pin
 * has been driven. It exists so the test checklist has something to state.
 *
 * @return true once smeterShow has been called.
 */
bool smeterDriven(void);

#endif /* DRIVERS_ANALOG_H */
