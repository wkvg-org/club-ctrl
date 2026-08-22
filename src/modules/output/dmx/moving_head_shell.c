/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Interactive shell for the 9-channel PicoSpot 45 moving head.
 * Commands are routed through the zbus as generic device commands.
 */

#include "moving_head.h"
#include "device.h"
#include "../../zbus/ctrl_zbus.h"

#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct color_name {
	const char *name;
	uint8_t     value;
};

static const struct color_name color_table[] = {
	{"white",  MH_COL_WHITE},
	{"red",    MH_COL_RED},
	{"orange", MH_COL_ORANGE},
	{"yellow", MH_COL_YELLOW},
	{"green",  MH_COL_GREEN},
	{"blue",   MH_COL_BLUE},
	{"cyan",   MH_COL_CYAN},
	{"purple", MH_COL_PURPLE},
};

static int parse_ulong(const char *s, unsigned long max, unsigned long *out)
{
	char *end;
	errno = 0;
	unsigned long v = strtoul(s, &end, 0);

	if (errno != 0 || *end != '\0' || v > max) {
		return -EINVAL;
	}
	*out = v;
	return 0;
}

static void mh_pub(uint16_t param, uint16_t val)
{
	struct dmx_ctrl_cmd cmd = {
		.type  = DMX_CTRL_DEV,
		.dev   = DMX_DEV_MOVING_HEAD,
		.inst  = 0,
		.param = param,
		.val   = val,
	};
	zbus_chan_pub(&dmx_ctrl_chan, &cmd, K_NO_WAIT);
}

static int cmd_pan(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long v;

	if (parse_ulong(argv[1], 540, &v) < 0) {
		shell_error(sh, "pan must be 0..540");
		return -EINVAL;
	}
	mh_pub(MH_PARAM_PAN, (uint16_t)v);
	shell_print(sh, "pan = %lu deg", v);
	return 0;
}

static int cmd_tilt(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long v;

	if (parse_ulong(argv[1], 200, &v) < 0) {
		shell_error(sh, "tilt must be 0..200");
		return -EINVAL;
	}
	mh_pub(MH_PARAM_TILT, (uint16_t)v);
	shell_print(sh, "tilt = %lu deg", v);
	return 0;
}

static int cmd_motor(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long v;

	if (parse_ulong(argv[1], 255, &v) < 0) {
		shell_error(sh, "motor must be 0..255");
		return -EINVAL;
	}
	mh_pub(MH_PARAM_MOTOR, (uint16_t)v);
	shell_print(sh, "motor = %lu (%s)", v, v < 128 ? "fast" : "slow");
	return 0;
}

static int cmd_color(const struct shell *sh, size_t argc, char **argv)
{
	for (size_t i = 0; i < ARRAY_SIZE(color_table); i++) {
		if (strcmp(argv[1], color_table[i].name) == 0) {
			mh_pub(MH_PARAM_COLOR, color_table[i].value);
			shell_print(sh, "color = %s (%u)",
				    color_table[i].name, color_table[i].value);
			return 0;
		}
	}

	unsigned long v;
	if (parse_ulong(argv[1], 255, &v) < 0) {
		shell_error(sh, "unknown color; use name or 0..255");
		return -EINVAL;
	}
	mh_pub(MH_PARAM_COLOR, (uint16_t)v);

	const char *kind =
		(v <= 87)  ? "fixed"   :
		(v <= 175) ? "split"   :
		             "rotate";
	shell_print(sh, "color = %lu (%s)", v, kind);
	return 0;
}

static int cmd_dimmer(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long v;

	if (parse_ulong(argv[1], 255, &v) < 0) {
		shell_error(sh, "dimmer must be 0..255");
		return -EINVAL;
	}
	mh_pub(MH_PARAM_DIMMER, (uint16_t)v);
	shell_print(sh, "dimmer = %lu (%lu%%)", v, (v * 100) / 255);
	return 0;
}

static int cmd_gobo(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long v;

	if (parse_ulong(argv[1], 255, &v) < 0) {
		shell_error(sh, "gobo must be 0..255");
		return -EINVAL;
	}
	mh_pub(MH_PARAM_GOBO, (uint16_t)v);
	shell_print(sh, "gobo = %lu", v);
	return 0;
}

static int cmd_strobe(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long v;

	if (parse_ulong(argv[1], 255, &v) < 0) {
		shell_error(sh, "strobe must be 0..255 (0..9 open, 10..255 = 1..25 Hz)");
		return -EINVAL;
	}
	mh_pub(MH_PARAM_STROBE, (uint16_t)v);
	shell_print(sh, "strobe = %lu", v);
	return 0;
}

static int cmd_home(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc); ARG_UNUSED(argv);
	mh_pub(MH_PARAM_HOME, 0);
	shell_print(sh, "home");
	return 0;
}

static int cmd_blackout(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc); ARG_UNUSED(argv);
	mh_pub(MH_PARAM_BLACKOUT, 0);
	shell_print(sh, "blackout");
	return 0;
}

static int cmd_show(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc); ARG_UNUSED(argv);
	for (int h = 0; h < MH_MAX_HEADS; h++) {
		shell_print(sh,
			    "head %d: pan=%u tilt=%u motor=%u color=%u gobo=%u dimmer=%u strobe=%u",
			    h, g_mh_state[h].pan_deg, g_mh_state[h].tilt_deg,
			    g_mh_state[h].motor_speed, g_mh_state[h].color, g_mh_state[h].gobo,
			    g_mh_state[h].dimmer, g_mh_state[h].strobe);
	}
	return 0;
}

static int cmd_flood(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc); ARG_UNUSED(argv);
	for (uint16_t ch = 1; ch <= 16; ch++) {
		struct dmx_ctrl_cmd cmd = {
			.type = DMX_CTRL_RAW,
			.ch   = ch,
			.val  = 255,
		};
		zbus_chan_pub(&dmx_ctrl_chan, &cmd, K_NO_WAIT);
	}
	shell_print(sh, "slots 1..16 = 255");
	return 0;
}

static int cmd_addr(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long v;

	if (parse_ulong(argv[1], 504, &v) < 0) {
		shell_error(sh, "addr must be 1..504");
		return -EINVAL;
	}
	mh_pub(MH_PARAM_ADDR, (uint16_t)v);
	shell_print(sh, "dmx start addr = %lu", v);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(mh_subcmds,
	SHELL_CMD_ARG(pan,      NULL, "<0..540>",              cmd_pan,      2, 0),
	SHELL_CMD_ARG(tilt,     NULL, "<0..200>",              cmd_tilt,     2, 0),
	SHELL_CMD_ARG(motor,    NULL, "<0..255>",              cmd_motor,    2, 0),
	SHELL_CMD_ARG(color,    NULL, "<name|0..255>",         cmd_color,    2, 0),
	SHELL_CMD_ARG(dimmer,   NULL, "<0..255>",              cmd_dimmer,   2, 0),
	SHELL_CMD_ARG(gobo,     NULL, "<0..255>",              cmd_gobo,     2, 0),
	SHELL_CMD_ARG(strobe,   NULL, "<0..255>",              cmd_strobe,   2, 0),
	SHELL_CMD_ARG(addr,     NULL, "<1..504>",              cmd_addr,     2, 0),
	SHELL_CMD(    home,     NULL, "recenter, white, full", cmd_home),
	SHELL_CMD(    blackout, NULL, "dimmer=0",              cmd_blackout),
	SHELL_CMD(    show,     NULL, "print current state",   cmd_show),
	SHELL_CMD(    flood,    NULL, "set slots 1..16 = 255", cmd_flood),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(mh, &mh_subcmds, "Moving-head control", NULL);
