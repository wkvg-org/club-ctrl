/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Cameo ROOT PAR 6 (CLROOTPAR6) RGBW PAR spotlight, 6-channel DMX mode.
 * See rgbw_par.h for the DMX map.
 */

#include "rgbw_par.h"
#include "device.h"
#include <dmx_z.h>

#include <zephyr/sys/util.h>

struct par_state g_par_state[PAR_MAX_INSTANCES] = {
	{
		.dimmer = 0,
		.strobe = 0,
		.red    = 0,
		.green  = 0,
		.blue   = 0,
		.white  = 0,
	},
};

/* DMX start address: PAR sits after the two moving heads (1..18). */
static uint16_t g_start_addr[PAR_MAX_INSTANCES] = { PAR_DEFAULT_START_ADDR };

enum {
	CH_DIMMER = 0,
	CH_STROBE,
	CH_RED,
	CH_GREEN,
	CH_BLUE,
	CH_WHITE,
	PAR_CH_COUNT
};

/**
 * Convert the logical PAR state to a 6-channel DMX buffer and write it to
 * the universe at the PAR's configured start address.
 */
void par_set(uint8_t inst, const struct par_state *s)
{
	if (inst >= PAR_MAX_INSTANCES || s == NULL) {
		return;
	}

	uint8_t buf[PAR_CH_COUNT] = {
		[CH_DIMMER] = s->dimmer,
		[CH_STROBE] = s->strobe,
		[CH_RED]    = s->red,
		[CH_GREEN]  = s->green,
		[CH_BLUE]   = s->blue,
		[CH_WHITE]  = s->white,
	};

	dmx_tx_set_slots(g_start_addr[inst], buf, sizeof(buf));
}

static struct par_state par_home_state(void)
{
	struct par_state s = {
		.dimmer = 255,
		.strobe = 0,
		.red    = 0,
		.green  = 0,
		.blue   = 0,
		.white  = 255,
	};
	return s;
}

void par_home(uint8_t inst)
{
	struct par_state s = par_home_state();
	par_set(inst, &s);
}

void par_blackout(uint8_t inst)
{
	struct par_state s = {0};
	par_set(inst, &s);
}

void par_set_addr(uint8_t inst, uint16_t addr)
{
	if (inst >= PAR_MAX_INSTANCES) {
		return;
	}
	if (addr < 1 || addr > PAR_MAX_START_ADDR) {
		return;
	}
	g_start_addr[inst] = addr;
}

/* ── Device registry glue ───────────────────────────────────────────── */

static uint16_t par_get_addr(uint8_t inst)
{
	return g_start_addr[inst];
}

static void par_apply(uint8_t inst, uint16_t param, uint16_t val)
{
	struct par_state s;

	switch (param) {
	case PAR_PARAM_DIMMER:
		g_par_state[inst].dimmer = (uint8_t)val;
		par_set(inst, &g_par_state[inst]);
		break;
	case PAR_PARAM_STROBE:
		g_par_state[inst].strobe = (uint8_t)val;
		par_set(inst, &g_par_state[inst]);
		break;
	case PAR_PARAM_RED:
		g_par_state[inst].red = (uint8_t)val;
		par_set(inst, &g_par_state[inst]);
		break;
	case PAR_PARAM_GREEN:
		g_par_state[inst].green = (uint8_t)val;
		par_set(inst, &g_par_state[inst]);
		break;
	case PAR_PARAM_BLUE:
		g_par_state[inst].blue = (uint8_t)val;
		par_set(inst, &g_par_state[inst]);
		break;
	case PAR_PARAM_WHITE:
		g_par_state[inst].white = (uint8_t)val;
		par_set(inst, &g_par_state[inst]);
		break;
	case PAR_PARAM_ADDR:
		par_set_addr(inst, val);
		break;
	case PAR_PARAM_HOME:
		s = par_home_state();
		par_set(inst, &s);
		g_par_state[inst] = s;
		break;
	case PAR_PARAM_BLACKOUT:
		par_blackout(inst);
		g_par_state[inst] = (struct par_state){0};
		break;
	default:
		break;
	}
}

static const struct dmx_param par_params[] = {
	{ "dimmer",   PAR_PARAM_DIMMER },
	{ "strobe",   PAR_PARAM_STROBE },
	{ "red",      PAR_PARAM_RED },
	{ "green",    PAR_PARAM_GREEN },
	{ "blue",     PAR_PARAM_BLUE },
	{ "white",    PAR_PARAM_WHITE },
	{ "addr",     PAR_PARAM_ADDR },
	{ "home",     PAR_PARAM_HOME },
	{ "blackout", PAR_PARAM_BLACKOUT },
};

static const struct dmx_device rgbw_par_device = {
	.name      = "par",
	.id        = DMX_DEV_RGBW_PAR,
	.max_inst  = PAR_MAX_INSTANCES,
	.apply     = par_apply,
	.get_addr  = par_get_addr,
	.params    = par_params,
	.num_params = ARRAY_SIZE(par_params),
};

void rgbw_par_init(void)
{
	dmx_device_register(&rgbw_par_device);
}
