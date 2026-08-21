/*
 * Copyright (c) 2026 Wolfenbütteler Kultur- und Veranstaltungsgesellschaft
 * SPDX-License-Identifier: Apache-2.0
 *
 * club-ctrl zbus channel definitions.  DMX commands are dispatched by a
 * listener in the publisher's context; sound and show commands use the
 * same pattern so that all external control flows through a single bus.
 */

#include "ctrl_zbus.h"

#include "../output/dmx/fixture.h"
#include <dmx_z.h>

/* exposed by the modules, not in headers but called directly */
void sound_play(const struct note *notes, size_t n);
void sound_stop(void);
void show_play(const struct show_frame *frames, size_t n);
void show_stop(void);

struct note;
struct show_frame;

#include <string.h>

/* ── DMX command listener ──────────────────────────────────────────── */

static void dmx_ctrl_listener(const struct zbus_channel *chan)
{
	const struct dmx_ctrl_cmd *cmd = zbus_chan_const_msg(chan);
	uint8_t head = cmd->head;

	if (head >= MH_MAX_HEADS) {
		return;
	}

	switch (cmd->type) {
	case DMX_CTRL_PAN:
		g_mh_state[head].pan_deg = cmd->val;
		mh_set(head, &g_mh_state[head]);
		break;
	case DMX_CTRL_TILT:
		g_mh_state[head].tilt_deg = (uint8_t)cmd->val;
		mh_set(head, &g_mh_state[head]);
		break;
	case DMX_CTRL_COLOR:
		g_mh_state[head].color = (uint8_t)cmd->val;
		mh_set(head, &g_mh_state[head]);
		break;
	case DMX_CTRL_DIMMER:
		g_mh_state[head].dimmer = (uint8_t)cmd->val;
		mh_set(head, &g_mh_state[head]);
		break;
	case DMX_CTRL_GOBO:
		g_mh_state[head].gobo = (uint8_t)cmd->val;
		mh_set(head, &g_mh_state[head]);
		break;
	case DMX_CTRL_STROBE:
		g_mh_state[head].strobe = (uint8_t)cmd->val;
		mh_set(head, &g_mh_state[head]);
		break;
	case DMX_CTRL_MOTOR:
		g_mh_state[head].motor_speed = (uint8_t)cmd->val;
		mh_set(head, &g_mh_state[head]);
		break;
	case DMX_CTRL_ADDR:
		mh_set_addr(head, cmd->val);
		break;
	case DMX_CTRL_HOME:
		mh_home(head);
		g_mh_state[head] = (struct mh_state){
			.pan_deg = 270, .tilt_deg = 100,
			.color = 5, .dimmer = 255,
		};
		break;
	case DMX_CTRL_BLACKOUT:
		mh_blackout(head);
		g_mh_state[head].dimmer = 0;
		break;
	case DMX_CTRL_RAW:
		dmx_tx_set_slot(cmd->ch, (uint8_t)cmd->val);
		break;
	}
}

ZBUS_LISTENER_DEFINE(dmx_ctrl_lis, dmx_ctrl_listener);
ZBUS_CHAN_DEFINE(dmx_ctrl_chan, struct dmx_ctrl_cmd, NULL, NULL,
		 ZBUS_OBSERVERS(dmx_ctrl_lis),
		 ZBUS_MSG_INIT(.type = 0, .head = 0, .val = 0, .ch = 0));

/* ── Sound command listener ────────────────────────────────────────── */

static void sound_cmd_listener(const struct zbus_channel *chan)
{
	const struct sound_cmd *cmd = zbus_chan_const_msg(chan);

	if (cmd->type == SOUND_CMD_PLAY) {
		sound_play(cmd->song, cmd->len);
	} else {
		sound_stop();
	}
}

ZBUS_LISTENER_DEFINE(sound_cmd_lis, sound_cmd_listener);
ZBUS_CHAN_DEFINE(sound_chan, struct sound_cmd, NULL, NULL,
		 ZBUS_OBSERVERS(sound_cmd_lis),
		 ZBUS_MSG_INIT(.type = 0, .song = NULL, .len = 0));

/* ── Light-show command listener ────────────────────────────────────── */

static void show_cmd_listener(const struct zbus_channel *chan)
{
	const struct show_cmd *cmd = zbus_chan_const_msg(chan);

	if (cmd->type == SHOW_CMD_PLAY) {
		show_play(cmd->frames, cmd->len);
	} else {
		show_stop();
	}
}

ZBUS_LISTENER_DEFINE(show_cmd_lis, show_cmd_listener);
ZBUS_CHAN_DEFINE(show_chan, struct show_cmd, NULL, NULL,
		 ZBUS_OBSERVERS(show_cmd_lis),
		 ZBUS_MSG_INIT(.type = 0, .frames = NULL, .len = 0));
