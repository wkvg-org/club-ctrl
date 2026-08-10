/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * PicoSpot 45 moving-head, 9-channel DMX mode (16-bit).
 *
 * DMX map (relative to start address, default 1):
 *   1  Pan coarse   0..255 → 0°..540°
 *   2  Tilt coarse  0..255 → 0°..200°
 *   3  Pan fine     16-bit LSB (unused, held 0)
 *   4  Tilt fine    16-bit LSB (unused, held 0)
 *   5  Motor speed  0=fast .. 255=slow
 *   6  Color wheel  bands of 11 / 88..175 split / 176..255 rotate
 *   7  Gobo         0..124 select / 125..249 shake / 250..255 wheel rotate
 *   8  Dimmer       0..255 = 0..100%
 *   9  Shutter      0..9 open / 10..255 strobe 1..25 Hz
 */

#ifndef FIXTURE_H_
#define FIXTURE_H_

#include <stdint.h>

#define MH_DEFAULT_START_ADDR 1
#define MH_MAX_START_ADDR     (512 - 9)  /* must leave room for 9 channels */

enum mh_color {
	MH_COL_WHITE  = 5,
	MH_COL_RED    = 16,
	MH_COL_ORANGE = 27,
	MH_COL_YELLOW = 38,
	MH_COL_GREEN  = 49,
	MH_COL_BLUE   = 60,
	MH_COL_CYAN   = 71,
	MH_COL_PURPLE = 82,
};

struct mh_state {
	uint16_t pan_deg;      /* 0..540 */
	uint8_t  tilt_deg;     /* 0..200 */
	uint8_t  motor_speed;  /* 0=fast, 255=slow */
	uint8_t  color;        /* raw DMX ch6 */
	uint8_t  gobo;         /* raw DMX ch7 */
	uint8_t  dimmer;       /* 0..255 */
	uint8_t  strobe;       /* 0=open, 10..255 = 1..25 Hz */
};

void mh_set(const struct mh_state *s);
void mh_home(void);
void mh_blackout(void);
void mh_set_addr(uint16_t addr);

/* Shared state for the shell and the network server. */
extern struct mh_state g_mh_state;

#endif /* FIXTURE_H_ */
