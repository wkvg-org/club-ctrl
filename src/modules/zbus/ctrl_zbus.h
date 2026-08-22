/*
 * Copyright (c) 2026 Wolfenbütteler Kultur- und Veranstaltungsgesellschaft
 * SPDX-License-Identifier: Apache-2.0
 *
 * club-ctrl zbus channels — decouple networking, inputs, and outputs.
 */

#ifndef CTRL_ZBUS_H_
#define CTRL_ZBUS_H_

#include <zephyr/zbus/zbus.h>
#include <stdint.h>

/* ── DMX device command ────────────────────────────────────────────── */

enum dmx_ctrl_cmd_type {
	DMX_CTRL_DEV = 0,  /* route to a device via the device registry */
	DMX_CTRL_RAW,       /* raw universe slot write (ch + val) */
};

/*
 * Generic command carried on the bus. For DMX_CTRL_DEV the message is
 * routed by dmx_device_dispatch(dev, inst, param, val); `param` is a
 * device-specific parameter id (see each device's enum, e.g. mh_param).
 */
struct dmx_ctrl_cmd {
	uint8_t  type;   /* enum dmx_ctrl_cmd_type */
	uint8_t  dev;    /* enum dmx_device_id (DEV only) */
	uint8_t  inst;   /* instance index within the device (DEV only) */
	uint16_t param;  /* device-specific parameter id (DEV only) */
	uint16_t val;    /* argument */
	uint16_t ch;     /* channel number (RAW only) */
};

ZBUS_CHAN_DECLARE(dmx_ctrl_chan);

/* ── Sound command ──────────────────────────────────────────────────── */

struct note;

enum sound_cmd_type {
	SOUND_CMD_PLAY = 0,
	SOUND_CMD_STOP,
};

struct sound_cmd {
	uint8_t          type;
	const struct note *song;
	size_t           len;
};

ZBUS_CHAN_DECLARE(sound_chan);

/* ── Light-show command ──────────────────────────────────────────────── */

struct show_frame;

enum show_cmd_type {
	SHOW_CMD_PLAY = 0,
	SHOW_CMD_STOP,
};

struct show_cmd {
	uint8_t                 type;
	const struct show_frame *frames;
	size_t                  len;
};

ZBUS_CHAN_DECLARE(show_chan);

#endif /* CTRL_ZBUS_H_ */
