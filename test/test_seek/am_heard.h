/*
 * AM readings labelled by ear, for the AM seek, squelch and scan
 * tests.
 *
 * Recorded off the radio. Do not edit by hand.
 */
#ifndef TEST_AM_HEARD_H
#define TEST_AM_HEARD_H

#include <stdint.h>

/* What a person heard on the channel. */
typedef enum {
  AM_HEARD_CLEAR, /* A programme that can be followed. */
  AM_HEARD_WEAK,  /* A programme under the noise. */
  AM_HEARD_NOISE, /* No programme at all. */
} AmHeard;

/* Four readings of one channel, the first 50 ms after a retune from
 * the channel below, the rest 30 ms apart. */
#define AM_HEARD_READS 4
typedef struct {
  uint32_t khz;
  AmHeard heard;
  int16_t levelTenths[AM_HEARD_READS];
  uint16_t noiseTenths[AM_HEARD_READS];
  int16_t offsetTenths[AM_HEARD_READS];
} AmChannel;

static const AmChannel kAmHeard[] = {
    {738,
     AM_HEARD_CLEAR,
     {441, 441, 452, 453},
     {38, 144, 113, 25},
     {0, 0, 0, 0}},
    {522,
     AM_HEARD_NOISE,
     {384, 385, 383, 375},
     {881, 850, 906, 713},
     {-6, -3, -4, -4}},
    {1125,
     AM_HEARD_NOISE,
     {386, 372, 380, 392},
     {1281, 1413, 819, 925},
     {4, 4, 7, 4}},
    {1143,
     AM_HEARD_NOISE,
     {374, 384, 384, 376},
     {1106, 531, 613, 550},
     {4, 5, 4, 4}},
    {990,
     AM_HEARD_NOISE,
     {381, 384, 375, 378},
     {613, 638, 544, 519},
     {-4, -3, -2, -2}},
    {171,
     AM_HEARD_NOISE,
     {487, 484, 489, 484},
     {431, 613, 475, 500},
     {4, 1, 0, 2}},
    {7225,
     AM_HEARD_NOISE,
     {203, 201, 201, 202},
     {2344, 3775, 3888, 4356},
     {0, 1, 1, -3}},
    {9570,
     AM_HEARD_CLEAR,
     {305, 325, 349, 367},
     {94, 163, 175, 81},
     {5, 5, 5, 5}},
    {15680,
     AM_HEARD_CLEAR,
     {490, 476, 482, 483},
     {50, 100, 113, 50},
     {9, 9, 9, 9}},
    {13710,
     AM_HEARD_CLEAR,
     {493, 483, 468, 447},
     {94, 38, 31, 38},
     {8, 8, 8, 8}},
    {11710,
     AM_HEARD_NOISE,
     {322, 326, 319, 320},
     {188, 188, 144, 75},
     {7, 7, 7, 7}},
    {9820,
     AM_HEARD_NOISE,
     {123, 126, 121, 123},
     {1981, 1850, 2494, 2706},
     {3, 1, -1, -1}},
    {7335,
     AM_HEARD_NOISE,
     {225, 229, 226, 231},
     {1475, 1275, 1825, 1394},
     {0, -2, 2, 0}},
    {11700,
     AM_HEARD_CLEAR,
     {439, 439, 435, 428},
     {13, 25, 31, 75},
     {7, 7, 7, 7}},
    {9515,
     AM_HEARD_WEAK,
     {238, 223, 214, 204},
     {325, 394, 313, 313},
     {5, 5, 5, 5}},
    {7425,
     AM_HEARD_NOISE,
     {236, 226, 227, 224},
     {1200, 1038, 950, 863},
     {0, 1, 1, 0}},
    {9810,
     AM_HEARD_WEAK,
     {158, 150, 137, 132},
     {713, 556, 775, 750},
     {4, 3, 2, -2}},
};

#define AM_HEARD_COUNT (sizeof(kAmHeard) / sizeof(kAmHeard[0]))

#endif /* TEST_AM_HEARD_H */
