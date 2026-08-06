/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * PicoSpot 45 9-channel mode driver. See fixture.h for the DMX map.
 */

#include "fixture.h"
#include "dmx.h"

#include <zephyr/sys/util.h>

struct mh_state g_mh_state = {
	.pan_deg     = 270,
	.tilt_deg    = 100,
	.motor_speed = 0,
	.color       = MH_COL_WHITE,
	.gobo        = 0,
	.dimmer      = 0,
	.strobe      = 0,
};

#define MH_START_ADDR 1

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

void mh_set(const struct mh_state *s)
{
	if (s == NULL) {
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

	dmx_set_slots(MH_START_ADDR, buf, sizeof(buf));
}

void mh_home(void)
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
	mh_set(&s);
}

void mh_blackout(void)
{
	struct mh_state s = {0};
	mh_set(&s);
}
