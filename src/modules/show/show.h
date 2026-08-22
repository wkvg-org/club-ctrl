/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Time-keyframed light show player. Frames carry an absolute start time
 * (from show_play() reference) and a target mh_state. Continuous fields
 * (pan/tilt/dimmer) are linearly interpolated between adjacent frames;
 * discrete fields (motor/color/gobo/strobe) step from the earlier frame.
 */

#ifndef SHOW_H_
#define SHOW_H_

#include <stddef.h>
#include <stdint.h>

#include "../output/dmx/moving_head.h"

struct show_frame {
	uint32_t        at_ms;
	struct mh_state state;
};

int  show_init(void);
int  show_play(const struct show_frame *frames, size_t n);
void show_stop(void);

extern const struct show_frame show_sandstorm[];
extern const size_t            show_sandstorm_len;

extern const struct show_frame show_rickroll[];
extern const size_t            show_rickroll_len;

extern const struct show_frame show_blue[];
extern const size_t            show_blue_len;

extern const struct show_frame show_lamour[];
extern const size_t            show_lamour_len;

extern const struct show_frame show_rhythm[];
extern const size_t            show_rhythm_len;

extern const struct show_frame show_dota[];
extern const size_t            show_dota_len;

extern const struct show_frame show_song[];
extern const size_t            show_song_len;

#endif
