/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Cameo ROOT PAR 6 (CLROOTPAR6) RGBW PAR spotlight, 6-channel DMX mode.
 *
 * DMX map (relative to start address, default 19):
 *   1  Dimmer   0..255 = 0..100%
 *   2  Strobe   0..5 open / 6..255 strobe slow -> fast (<1Hz..20Hz)
 *   3  Red      0..255
 *   4  Green    0..255
 *   5  Blue     0..255
 *   6  White    0..255
 */

#ifndef RGBW_PAR_H_
#define RGBW_PAR_H_

#include <stdint.h>

#define PAR_DEFAULT_START_ADDR 19
#define PAR_MAX_START_ADDR     (512 - 6)  /* must leave room for 6 channels */
#define PAR_MAX_INSTANCES      1

struct par_state {
	uint8_t dimmer;  /* 0..255 */
	uint8_t strobe;  /* 0..5 open, 6..255 slow->fast */
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t white;
};

/* Device-specific parameters (routed through the device registry). */
enum par_param {
	PAR_PARAM_DIMMER = 0,
	PAR_PARAM_STROBE,
	PAR_PARAM_RED,
	PAR_PARAM_GREEN,
	PAR_PARAM_BLUE,
	PAR_PARAM_WHITE,
	PAR_PARAM_ADDR,
	PAR_PARAM_HOME,
	PAR_PARAM_BLACKOUT,
};

void rgbw_par_init(void);

void par_set(uint8_t inst, const struct par_state *s);
void par_home(uint8_t inst);
void par_blackout(uint8_t inst);
void par_set_addr(uint8_t inst, uint16_t addr);

/* Shared state for the shell and the network server. */
extern struct par_state g_par_state[PAR_MAX_INSTANCES];

#endif /* RGBW_PAR_H_ */
