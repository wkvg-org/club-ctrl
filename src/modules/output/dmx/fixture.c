/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * PicoSpot 45 9-channel mode driver for two moving heads.
 * See fixture.h for the DMX map.
 */

#include "fixture.h"
#include <dmx_z.h>

#include <zephyr/sys/util.h>

struct mh_state g_mh_state[MH_MAX_HEADS] = {
	{
		.pan_deg     = 270,
		.tilt_deg    = 100,
		.motor_speed = 0,
		.color       = MH_COL_WHITE,
		.gobo        = 0,
		.dimmer      = 0,
		.strobe      = 0,
	},
	{
		.pan_deg     = 270,
		.tilt_deg    = 100,
		.motor_speed = 0,
		.color       = MH_COL_WHITE,
		.gobo        = 0,
		.dimmer      = 0,
		.strobe      = 0,
	},
};

/* DMX start addresses: head 0 → 1, head 1 → 10 (9-channel mode, no overlap) */
static uint16_t g_start_addr[MH_MAX_HEADS] = { 1, 10 };

#define PAN_MAX_DEG   540u
#define TILT_MAX_DEG  200u

enum {
	CH_PAN_COARSE = 0,
	CH_TILT_COARSE,
	CH_PAN_FINE,
	CH_TILT_FINE,
	CH_MOTOR_SPD,
	CH_COLOR,
	CH_GOBO,
	CH_DIMMER,
	CH_SHUTTER,
	MH_CH_COUNT
};

/**
 * Convert the logical moving-head state to a 9-channel DMX buffer and
 * write it to the universe at the head's configured start address.
 * Pan (0–540°) and tilt (0–200°) are mapped linearly to 0–255 range.
 * 16-bit fine channels (ch3, ch4) are held at zero.
 */
void mh_set(uint8_t head, const struct mh_state *s)
{
	if (head >= MH_MAX_HEADS || s == NULL) {
		return;
	}

	uint8_t buf[MH_CH_COUNT] = {0};

	buf[CH_PAN_COARSE]  = (uint8_t)((MIN(s->pan_deg,  PAN_MAX_DEG)  * 255u) / PAN_MAX_DEG);
	buf[CH_TILT_COARSE] = (uint8_t)((MIN(s->tilt_deg, TILT_MAX_DEG) * 255u) / TILT_MAX_DEG);
	buf[CH_MOTOR_SPD]   = s->motor_speed;
	buf[CH_COLOR]       = s->color;
	buf[CH_GOBO]        = s->gobo;
	buf[CH_DIMMER]      = s->dimmer;
	buf[CH_SHUTTER]     = s->strobe;

	dmx_tx_set_slots(g_start_addr[head], buf, sizeof(buf));
}

void mh_home(uint8_t head)
{
	struct mh_state s = {
		.pan_deg     = PAN_MAX_DEG / 2,
		.tilt_deg    = TILT_MAX_DEG / 2,
		.motor_speed = 0,
		.color       = MH_COL_WHITE,
		.gobo        = 0,
		.dimmer      = 255,
		.strobe      = 0,
	};
	mh_set(head, &s);
}

void mh_blackout(uint8_t head)
{
	struct mh_state s = {0};
	mh_set(head, &s);
}

void mh_set_addr(uint8_t head, uint16_t addr)
{
	if (head >= MH_MAX_HEADS) {
		return;
	}
	if (addr < 1 || addr > MH_MAX_START_ADDR) {
		return;
	}
	g_start_addr[head] = addr;
}
