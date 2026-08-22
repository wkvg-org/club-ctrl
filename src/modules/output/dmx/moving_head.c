/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * PicoSpot 45 9-channel mode driver for two moving heads.
 * See moving_head.h for the DMX map.
 */

#include "moving_head.h"
#include "device.h"
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

static struct mh_state mh_home_state(void)
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
	return s;
}

void mh_home(uint8_t head)
{
	struct mh_state s = mh_home_state();
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

/* ── Device registry glue ───────────────────────────────────────────── */

static uint16_t mh_get_addr(uint8_t head)
{
	return g_start_addr[head];
}

static void mh_apply(uint8_t head, uint16_t param, uint16_t val)
{
	struct mh_state s;

	switch (param) {
	case MH_PARAM_PAN:
		g_mh_state[head].pan_deg = val;
		mh_set(head, &g_mh_state[head]);
		break;
	case MH_PARAM_TILT:
		g_mh_state[head].tilt_deg = (uint8_t)val;
		mh_set(head, &g_mh_state[head]);
		break;
	case MH_PARAM_COLOR:
		g_mh_state[head].color = (uint8_t)val;
		mh_set(head, &g_mh_state[head]);
		break;
	case MH_PARAM_DIMMER:
		g_mh_state[head].dimmer = (uint8_t)val;
		mh_set(head, &g_mh_state[head]);
		break;
	case MH_PARAM_GOBO:
		g_mh_state[head].gobo = (uint8_t)val;
		mh_set(head, &g_mh_state[head]);
		break;
	case MH_PARAM_STROBE:
		g_mh_state[head].strobe = (uint8_t)val;
		mh_set(head, &g_mh_state[head]);
		break;
	case MH_PARAM_MOTOR:
		g_mh_state[head].motor_speed = (uint8_t)val;
		mh_set(head, &g_mh_state[head]);
		break;
	case MH_PARAM_ADDR:
		mh_set_addr(head, val);
		break;
	case MH_PARAM_HOME:
		s = mh_home_state();
		mh_set(head, &s);
		g_mh_state[head] = s;
		break;
	case MH_PARAM_BLACKOUT:
		mh_blackout(head);
		g_mh_state[head].dimmer = 0;
		break;
	default:
		break;
	}
}

static const struct dmx_param mh_params[] = {
	{ "pan",      MH_PARAM_PAN },
	{ "tilt",     MH_PARAM_TILT },
	{ "color",    MH_PARAM_COLOR },
	{ "dimmer",   MH_PARAM_DIMMER },
	{ "gobo",     MH_PARAM_GOBO },
	{ "strobe",   MH_PARAM_STROBE },
	{ "motor",    MH_PARAM_MOTOR },
	{ "addr",     MH_PARAM_ADDR },
	{ "home",     MH_PARAM_HOME },
	{ "blackout", MH_PARAM_BLACKOUT },
};

static const struct dmx_device moving_head_device = {
	.name      = "mh",
	.id        = DMX_DEV_MOVING_HEAD,
	.max_inst  = MH_MAX_HEADS,
	.apply     = mh_apply,
	.get_addr  = mh_get_addr,
	.params    = mh_params,
	.num_params = ARRAY_SIZE(mh_params),
};

void moving_head_init(void)
{
	dmx_device_register(&moving_head_device);
}
