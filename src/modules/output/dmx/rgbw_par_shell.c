/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Interactive shell for the Cameo ROOT PAR 6 RGBW PAR spotlight.
 * Commands are routed through the zbus as generic device commands.
 */

#include "rgbw_par.h"
#include "device.h"
#include "../../zbus/ctrl_zbus.h"

#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

static void par_pub(uint16_t param, uint16_t val)
{
	struct dmx_ctrl_cmd cmd = {
		.type  = DMX_CTRL_DEV,
		.dev   = DMX_DEV_RGBW_PAR,
		.inst  = 0,
		.param = param,
		.val   = val,
	};
	zbus_chan_pub(&dmx_ctrl_chan, &cmd, K_NO_WAIT);
}

static int cmd_channel(const struct shell *sh, size_t argc, char **argv,
		       uint16_t param, const char *what)
{
	unsigned long v;

	if (parse_ulong(argv[1], 255, &v) < 0) {
		shell_error(sh, "%s must be 0..255", what);
		return -EINVAL;
	}
	par_pub(param, (uint16_t)v);
	shell_print(sh, "%s = %lu", what, v);
	return 0;
}

static int cmd_dimmer(const struct shell *sh, size_t argc, char **argv)
{
	return cmd_channel(sh, argc, argv, PAR_PARAM_DIMMER, "dimmer");
}

static int cmd_strobe(const struct shell *sh, size_t argc, char **argv)
{
	return cmd_channel(sh, argc, argv, PAR_PARAM_STROBE, "strobe");
}

static int cmd_red(const struct shell *sh, size_t argc, char **argv)
{
	return cmd_channel(sh, argc, argv, PAR_PARAM_RED, "red");
}

static int cmd_green(const struct shell *sh, size_t argc, char **argv)
{
	return cmd_channel(sh, argc, argv, PAR_PARAM_GREEN, "green");
}

static int cmd_blue(const struct shell *sh, size_t argc, char **argv)
{
	return cmd_channel(sh, argc, argv, PAR_PARAM_BLUE, "blue");
}

static int cmd_white(const struct shell *sh, size_t argc, char **argv)
{
	return cmd_channel(sh, argc, argv, PAR_PARAM_WHITE, "white");
}

static int cmd_home(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc); ARG_UNUSED(argv);
	par_pub(PAR_PARAM_HOME, 0);
	shell_print(sh, "home (white, full)");
	return 0;
}

static int cmd_blackout(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc); ARG_UNUSED(argv);
	par_pub(PAR_PARAM_BLACKOUT, 0);
	shell_print(sh, "blackout");
	return 0;
}

static int cmd_show(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc); ARG_UNUSED(argv);
	struct par_state *s = &g_par_state[0];

	shell_print(sh,
		    "par: dimmer=%u strobe=%u red=%u green=%u blue=%u white=%u",
		    s->dimmer, s->strobe, s->red, s->green, s->blue, s->white);
	return 0;
}

static int cmd_addr(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long v;

	if (parse_ulong(argv[1], 504, &v) < 0) {
		shell_error(sh, "addr must be 1..504");
		return -EINVAL;
	}
	par_pub(PAR_PARAM_ADDR, (uint16_t)v);
	shell_print(sh, "dmx start addr = %lu", v);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(par_subcmds,
	SHELL_CMD_ARG(dimmer,   NULL, "<0..255>",        cmd_dimmer,   2, 0),
	SHELL_CMD_ARG(strobe,   NULL, "<0..255>",        cmd_strobe,   2, 0),
	SHELL_CMD_ARG(red,      NULL, "<0..255>",        cmd_red,      2, 0),
	SHELL_CMD_ARG(green,    NULL, "<0..255>",        cmd_green,    2, 0),
	SHELL_CMD_ARG(blue,     NULL, "<0..255>",        cmd_blue,     2, 0),
	SHELL_CMD_ARG(white,    NULL, "<0..255>",        cmd_white,    2, 0),
	SHELL_CMD_ARG(addr,     NULL, "<1..504>",        cmd_addr,     2, 0),
	SHELL_CMD(    home,     NULL, "white, full",     cmd_home),
	SHELL_CMD(    blackout, NULL, "dimmer=0",        cmd_blackout),
	SHELL_CMD(    show,     NULL, "print state",     cmd_show),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(par, &par_subcmds, "RGBW PAR spotlight control", NULL);
