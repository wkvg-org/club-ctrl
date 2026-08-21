/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Raw DMX universe control shell.
 */

#include <dmx_z.h>
#include "../../zbus/ctrl_zbus.h"

#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>

#include <stdlib.h>
#include <errno.h>

static int cmd_dmx_raw(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 3) {
		shell_error(sh, "usage: dmx raw <ch> <val>");
		return -EINVAL;
	}

	struct dmx_ctrl_cmd cmd = {
		.type = DMX_CTRL_RAW,
		.ch   = (uint16_t)atoi(argv[1]),
		.val  = (uint16_t)atoi(argv[2]),
	};
	zbus_chan_pub(&dmx_ctrl_chan, &cmd, K_NO_WAIT);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(dmx_subcmds,
	SHELL_CMD_ARG(raw, NULL, "<ch> <val>", cmd_dmx_raw, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(dmx, &dmx_subcmds, "Raw DMX universe control", NULL);
