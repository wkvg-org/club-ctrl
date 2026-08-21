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

/* ── DMX fixture command ───────────────────────────────────────────── */

enum dmx_ctrl_cmd_type {
	DMX_CTRL_PAN = 0,
	DMX_CTRL_TILT,
	DMX_CTRL_COLOR,
	DMX_CTRL_DIMMER,
	DMX_CTRL_GOBO,
	DMX_CTRL_STROBE,
	DMX_CTRL_MOTOR,
	DMX_CTRL_ADDR,
	DMX_CTRL_HOME,
	DMX_CTRL_BLACKOUT,
	DMX_CTRL_RAW,
};

struct dmx_ctrl_cmd {
	uint8_t  type;
	uint8_t  head;   /* moving-head index (0 or 1) */
	uint16_t val;    /* primary argument */
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
