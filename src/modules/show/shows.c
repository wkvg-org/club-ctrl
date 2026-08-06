/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Light shows for the two songs, plus a `party` shell that starts
 * buzzer + show together so audio and lights share a t=0 reference.
 */

#include "show.h"
#include "../output/sound/sound.h"

#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#include <string.h>

#define F(t, ...) { .at_ms = (t), .state = { __VA_ARGS__ } }

/*
 * Sandstorm (~29 s): intro fade-in, fast pan sweeps on the verses,
 * gobo + colour rotation on the variation, strobe climax on verse 3,
 * fade to black. Motor kept fast so pan sweeps track the 136 BPM feel.
 */
const struct show_frame show_sandstorm[] = {
	F(    0, .pan_deg=270, .tilt_deg=100, .dimmer=  0, .color=MH_COL_BLUE,   .motor_speed=200),
	F( 2000, .pan_deg=270, .tilt_deg=100, .dimmer=255, .color=MH_COL_BLUE,   .motor_speed=200),

	F( 4000, .pan_deg=  0, .tilt_deg= 80, .dimmer=255, .color=MH_COL_BLUE,   .motor_speed=  0),
	F( 5500, .pan_deg=540, .tilt_deg=120, .dimmer=255, .color=MH_COL_BLUE,   .motor_speed=  0),
	F( 7000, .pan_deg=  0, .tilt_deg= 80, .dimmer=255, .color=MH_COL_CYAN,   .motor_speed=  0),
	F( 8500, .pan_deg=540, .tilt_deg=120, .dimmer=255, .color=MH_COL_CYAN,   .motor_speed=  0),
	F(10000, .pan_deg=100, .tilt_deg=100, .dimmer=255, .color=MH_COL_PURPLE, .motor_speed=  0, .strobe=80),
	F(11500, .pan_deg=440, .tilt_deg=100, .dimmer=255, .color=MH_COL_PURPLE, .motor_speed=  0, .strobe=80),
	F(13000, .pan_deg=100, .tilt_deg=100, .dimmer=255, .color=MH_COL_RED,    .motor_speed=  0, .strobe=140),
	F(14500, .pan_deg=440, .tilt_deg=100, .dimmer=255, .color=MH_COL_RED,    .motor_speed=  0, .strobe=140),

	F(16000, .pan_deg=270, .tilt_deg= 50, .dimmer=255, .color=192, .gobo=200, .motor_speed= 60),
	F(18000, .pan_deg=270, .tilt_deg=150, .dimmer=255, .color=192, .gobo=200, .motor_speed= 60),
	F(20000, .pan_deg=270, .tilt_deg= 50, .dimmer=255, .color=200, .gobo=200, .motor_speed= 60),

	F(22000, .pan_deg=  0, .tilt_deg=100, .dimmer=255, .color=MH_COL_RED,    .gobo=60, .motor_speed=  0, .strobe=200),
	F(23500, .pan_deg=540, .tilt_deg=100, .dimmer=255, .color=MH_COL_ORANGE, .gobo=60, .motor_speed=  0, .strobe=200),
	F(25000, .pan_deg=  0, .tilt_deg=100, .dimmer=255, .color=MH_COL_YELLOW, .gobo=60, .motor_speed=  0, .strobe=200),
	F(26500, .pan_deg=540, .tilt_deg=100, .dimmer=255, .color=MH_COL_WHITE,  .gobo=60, .motor_speed=  0, .strobe=255),
	F(28000, .pan_deg=270, .tilt_deg=100, .dimmer=255, .color=MH_COL_WHITE,  .motor_speed=  0, .strobe=255),
	F(29500, .pan_deg=270, .tilt_deg=100, .dimmer=  0, .color=MH_COL_WHITE),
};
const size_t show_sandstorm_len = ARRAY_SIZE(show_sandstorm);

/*
 * Never Gonna Give You Up (~13 s): cheesy 80s vibe. Slow head sway
 * spanning the whole pan range, colour cycle red→orange→…→purple,
 * no strobe, dimmer stays full — until a wink blackout at the end.
 */
const struct show_frame show_rickroll[] = {
	F(    0, .pan_deg=180, .tilt_deg= 90, .dimmer=255, .color=MH_COL_WHITE,  .motor_speed=180),
	F( 1500, .pan_deg=360, .tilt_deg=110, .dimmer=255, .color=MH_COL_RED,    .motor_speed=180),
	F( 3000, .pan_deg=180, .tilt_deg= 90, .dimmer=255, .color=MH_COL_ORANGE, .motor_speed=180),
	F( 4500, .pan_deg=360, .tilt_deg=110, .dimmer=255, .color=MH_COL_YELLOW, .motor_speed=180),
	F( 6000, .pan_deg=180, .tilt_deg= 90, .dimmer=255, .color=MH_COL_GREEN,  .motor_speed=180),
	F( 7500, .pan_deg=360, .tilt_deg=110, .dimmer=255, .color=MH_COL_CYAN,   .motor_speed=180),
	F( 9000, .pan_deg=180, .tilt_deg= 90, .dimmer=255, .color=MH_COL_BLUE,   .motor_speed=180),
	F(10500, .pan_deg=360, .tilt_deg=110, .dimmer=255, .color=MH_COL_PURPLE, .motor_speed=180),
	F(12000, .pan_deg=270, .tilt_deg=100, .dimmer=255, .color=MH_COL_WHITE,  .motor_speed=180),
	F(13500, .pan_deg=270, .tilt_deg=100, .dimmer=  0, .color=MH_COL_WHITE,  .motor_speed=180),
};
const size_t show_rickroll_len = ARRAY_SIZE(show_rickroll);

/*
 * Blue (Da Ba Dee) — ~8 s. Everything blue, obviously. Slow head sway,
 * dimmer pulses on the "da-ba-dee" phrases, cyan flash between verses.
 */
const struct show_frame show_blue[] = {
	F(   0, .pan_deg=270, .tilt_deg= 90, .dimmer=100, .color=MH_COL_BLUE, .motor_speed=180),
	F(1000, .pan_deg=180, .tilt_deg=110, .dimmer=255, .color=MH_COL_BLUE, .motor_speed=180),
	F(2000, .pan_deg=360, .tilt_deg= 90, .dimmer=100, .color=MH_COL_BLUE, .motor_speed=180),
	F(2500, .pan_deg=360, .tilt_deg= 90, .dimmer=255, .color=MH_COL_CYAN, .motor_speed=180),
	F(4000, .pan_deg=180, .tilt_deg=110, .dimmer=100, .color=MH_COL_BLUE, .motor_speed=180),
	F(5000, .pan_deg=360, .tilt_deg= 90, .dimmer=255, .color=MH_COL_BLUE, .motor_speed=180),
	F(6000, .pan_deg=180, .tilt_deg=110, .dimmer=100, .color=MH_COL_CYAN, .motor_speed=180),
	F(7000, .pan_deg=270, .tilt_deg=100, .dimmer=255, .color=MH_COL_BLUE, .motor_speed=180),
	F(8000, .pan_deg=270, .tilt_deg=100, .dimmer=  0, .color=MH_COL_BLUE, .motor_speed=180),
};
const size_t show_blue_len = ARRAY_SIZE(show_blue);

/*
 * L'Amour Toujours — ~8 s. Warm italo-disco palette (orange/red/purple),
 * romantic slow sweeps, dimmer breathes with the phrases. No strobe.
 */
const struct show_frame show_lamour[] = {
	F(   0, .pan_deg=270, .tilt_deg=100, .dimmer=120, .color=MH_COL_RED,    .motor_speed=200),
	F(1500, .pan_deg=200, .tilt_deg= 80, .dimmer=255, .color=MH_COL_ORANGE, .motor_speed=200),
	F(3000, .pan_deg=340, .tilt_deg=120, .dimmer=180, .color=MH_COL_PURPLE, .motor_speed=200),
	F(4500, .pan_deg=200, .tilt_deg= 80, .dimmer=255, .color=MH_COL_RED,    .motor_speed=200),
	F(6000, .pan_deg=340, .tilt_deg=120, .dimmer=180, .color=MH_COL_ORANGE, .motor_speed=200),
	F(7500, .pan_deg=270, .tilt_deg=100, .dimmer=255, .color=MH_COL_PURPLE, .motor_speed=200),
	F(8500, .pan_deg=270, .tilt_deg=100, .dimmer=  0, .color=MH_COL_RED,    .motor_speed=200),
};
const size_t show_lamour_len = ARRAY_SIZE(show_lamour);

/*
 * Rhythm Is A Dancer — ~8 s. Driving eurodance: fast sweeps that flip
 * direction on each "rhythm is a dancer", colour cycle, mild strobe.
 */
const struct show_frame show_rhythm[] = {
	F(   0, .pan_deg=  0, .tilt_deg= 90, .dimmer=255, .color=MH_COL_RED,    .motor_speed= 20, .strobe=60),
	F(1900, .pan_deg=540, .tilt_deg=110, .dimmer=255, .color=MH_COL_YELLOW, .motor_speed= 20, .strobe=60),
	F(3800, .pan_deg=  0, .tilt_deg= 90, .dimmer=255, .color=MH_COL_GREEN,  .motor_speed= 20, .strobe=60),
	F(5700, .pan_deg=540, .tilt_deg=110, .dimmer=255, .color=MH_COL_CYAN,   .motor_speed= 20, .strobe=100),
	F(7600, .pan_deg=270, .tilt_deg=100, .dimmer=255, .color=MH_COL_PURPLE, .motor_speed= 20, .strobe=180),
	F(8200, .pan_deg=270, .tilt_deg=100, .dimmer=  0, .color=MH_COL_PURPLE, .motor_speed= 20),
};
const size_t show_rhythm_len = ARRAY_SIZE(show_rhythm);

/*
 * DotA — ~7 s. Melodic trance vibe: gobo rotation throughout, colour
 * rotation mode, medium sweeps, dimmer pulses on melodic peaks.
 */
const struct show_frame show_dota[] = {
	F(   0, .pan_deg=180, .tilt_deg= 80, .dimmer=200, .color=192, .gobo=200, .motor_speed= 40),
	F(1700, .pan_deg=360, .tilt_deg=120, .dimmer=255, .color=200, .gobo=200, .motor_speed= 40),
	F(3400, .pan_deg=180, .tilt_deg= 80, .dimmer=200, .color=210, .gobo=200, .motor_speed= 40),
	F(5100, .pan_deg=360, .tilt_deg=120, .dimmer=255, .color=220, .gobo=200, .motor_speed= 40, .strobe=80),
	F(6800, .pan_deg=270, .tilt_deg=100, .dimmer=255, .color=230, .gobo=200, .motor_speed= 40, .strobe=140),
	F(7500, .pan_deg=270, .tilt_deg=100, .dimmer=  0, .color=230, .motor_speed= 40),
};
const size_t show_dota_len = ARRAY_SIZE(show_dota);

/*
 * "song" (~238 s / ~4 min). Generic driving-techno arc since we don't
 * know the section boundaries of the transcription: intro fade-in,
 * verse sweeps, build, drop with strobe + gobo rotation, verse 2,
 * climax with max strobe, fade-out.
 */
const struct show_frame show_song[] = {
	F(     0, .pan_deg=270, .tilt_deg=100, .dimmer=  0, .color=MH_COL_BLUE,   .motor_speed=0),
	F(  4000, .pan_deg=270, .tilt_deg=100, .dimmer=200, .color=MH_COL_BLUE,   .motor_speed=0),
	F(  8000, .pan_deg=  0, .tilt_deg= 40, .dimmer=255, .color=MH_COL_BLUE,   .motor_speed=0),
	F( 12000, .pan_deg=540, .tilt_deg=170, .dimmer=255, .color=MH_COL_BLUE,   .motor_speed=0),
	F( 16000, .pan_deg=  0, .tilt_deg= 40, .dimmer=255, .color=MH_COL_CYAN,   .motor_speed=0),
	F( 20000, .pan_deg=540, .tilt_deg=170, .dimmer=255, .color=MH_COL_CYAN,   .motor_speed=0),
	F( 24000, .pan_deg=100, .tilt_deg= 80, .dimmer=255, .color=MH_COL_CYAN,   .motor_speed=0, .strobe=40),
	F( 28000, .pan_deg=440, .tilt_deg=130, .dimmer=255, .color=MH_COL_BLUE,   .motor_speed=0, .strobe=40),
	F( 32000, .pan_deg=100, .tilt_deg= 60, .dimmer=255, .color=MH_COL_PURPLE, .motor_speed=0, .strobe=80),
	F( 36000, .pan_deg=440, .tilt_deg=150, .dimmer=255, .color=MH_COL_PURPLE, .motor_speed=0, .strobe=80),
	F( 40000, .pan_deg=  0, .tilt_deg= 40, .dimmer=255, .color=MH_COL_PURPLE, .motor_speed=0, .strobe=120),
	F( 44000, .pan_deg=540, .tilt_deg=170, .dimmer=255, .color=MH_COL_RED,    .motor_speed=0, .strobe=120),
	F( 48000, .pan_deg=  0, .tilt_deg= 40, .dimmer=255, .color=MH_COL_RED,    .motor_speed=0, .strobe=150),
	F( 52000, .pan_deg=540, .tilt_deg=170, .dimmer=255, .color=MH_COL_RED,    .motor_speed=0, .strobe=150),
	F( 56000, .pan_deg=  0, .tilt_deg= 40, .dimmer=255, .color=MH_COL_ORANGE, .gobo=200, .motor_speed=0, .strobe=180),
	F( 60000, .pan_deg=540, .tilt_deg=170, .dimmer=255, .color=MH_COL_ORANGE, .gobo=200, .motor_speed=0, .strobe=180),
	F( 64000, .pan_deg=100, .tilt_deg= 60, .dimmer=255, .color=MH_COL_YELLOW, .gobo=200, .motor_speed=0, .strobe=180),
	F( 68000, .pan_deg=440, .tilt_deg=150, .dimmer=255, .color=MH_COL_YELLOW, .gobo=200, .motor_speed=0, .strobe=180),
	F( 72000, .pan_deg=  0, .tilt_deg= 40, .dimmer=255, .color=192, .gobo=220, .motor_speed=0, .strobe=200),
	F( 76000, .pan_deg=540, .tilt_deg=170, .dimmer=255, .color=200, .gobo=220, .motor_speed=0, .strobe=200),
	F( 80000, .pan_deg=  0, .tilt_deg= 40, .dimmer=255, .color=210, .gobo=220, .motor_speed=0, .strobe=200),
	F( 84000, .pan_deg=540, .tilt_deg=170, .dimmer=255, .color=220, .gobo=220, .motor_speed=0, .strobe=200),
	F( 88000, .pan_deg=270, .tilt_deg= 30, .dimmer=255, .color=230, .gobo=220, .motor_speed=0, .strobe=220),
	F( 92000, .pan_deg=270, .tilt_deg=170, .dimmer=255, .color=240, .gobo=220, .motor_speed=0, .strobe=220),
	F( 96000, .pan_deg=100, .tilt_deg= 40, .dimmer=255, .color=MH_COL_CYAN,   .gobo= 60, .motor_speed=0, .strobe=100),
	F(100000, .pan_deg=440, .tilt_deg=160, .dimmer=255, .color=MH_COL_BLUE,   .gobo= 60, .motor_speed=0, .strobe=100),
	F(104000, .pan_deg=100, .tilt_deg= 40, .dimmer=255, .color=MH_COL_PURPLE, .gobo= 60, .motor_speed=0, .strobe=140),
	F(108000, .pan_deg=440, .tilt_deg=160, .dimmer=255, .color=MH_COL_PURPLE, .gobo= 60, .motor_speed=0, .strobe=140),
	F(112000, .pan_deg=  0, .tilt_deg= 40, .dimmer=255, .color=MH_COL_RED,    .gobo=100, .motor_speed=0, .strobe=180),
	F(116000, .pan_deg=540, .tilt_deg=170, .dimmer=255, .color=MH_COL_RED,    .gobo=100, .motor_speed=0, .strobe=180),
	F(120000, .pan_deg=  0, .tilt_deg=170, .dimmer=255, .color=MH_COL_ORANGE, .gobo=100, .motor_speed=0, .strobe=180),
	F(124000, .pan_deg=540, .tilt_deg= 40, .dimmer=255, .color=MH_COL_YELLOW, .gobo=100, .motor_speed=0, .strobe=180),
	F(128000, .pan_deg=100, .tilt_deg= 80, .dimmer=255, .color=MH_COL_GREEN,  .gobo=100, .motor_speed=0, .strobe=200),
	F(132000, .pan_deg=440, .tilt_deg=130, .dimmer=255, .color=MH_COL_CYAN,   .gobo=100, .motor_speed=0, .strobe=200),
	F(136000, .pan_deg=  0, .tilt_deg= 40, .dimmer=255, .color=MH_COL_BLUE,   .gobo=100, .motor_speed=0, .strobe=200),
	F(140000, .pan_deg=540, .tilt_deg=170, .dimmer=255, .color=MH_COL_PURPLE, .gobo=100, .motor_speed=0, .strobe=220),
	F(144000, .pan_deg=270, .tilt_deg= 30, .dimmer=255, .color=192, .gobo=250, .motor_speed=0, .strobe=220),
	F(148000, .pan_deg=270, .tilt_deg=170, .dimmer=255, .color=200, .gobo=250, .motor_speed=0, .strobe=220),
	F(152000, .pan_deg=  0, .tilt_deg= 40, .dimmer=255, .color=210, .gobo=250, .motor_speed=0, .strobe=220),
	F(156000, .pan_deg=540, .tilt_deg=170, .dimmer=255, .color=220, .gobo=250, .motor_speed=0, .strobe=220),
	F(160000, .pan_deg=  0, .tilt_deg= 40, .dimmer=255, .color=MH_COL_RED,    .gobo= 60, .motor_speed=0, .strobe=240),
	F(164000, .pan_deg=540, .tilt_deg=170, .dimmer=255, .color=MH_COL_ORANGE, .gobo= 60, .motor_speed=0, .strobe=240),
	F(168000, .pan_deg=  0, .tilt_deg=170, .dimmer=255, .color=MH_COL_YELLOW, .gobo= 60, .motor_speed=0, .strobe=240),
	F(172000, .pan_deg=540, .tilt_deg= 40, .dimmer=255, .color=MH_COL_WHITE,  .gobo= 60, .motor_speed=0, .strobe=240),
	F(176000, .pan_deg=  0, .tilt_deg= 40, .dimmer=255, .color=MH_COL_WHITE,  .gobo= 60, .motor_speed=0, .strobe=255),
	F(180000, .pan_deg=540, .tilt_deg=170, .dimmer=255, .color=MH_COL_WHITE,  .gobo= 60, .motor_speed=0, .strobe=255),
	F(184000, .pan_deg=  0, .tilt_deg=170, .dimmer=255, .color=MH_COL_WHITE,  .gobo= 60, .motor_speed=0, .strobe=255),
	F(188000, .pan_deg=540, .tilt_deg= 40, .dimmer=255, .color=MH_COL_WHITE,  .gobo= 60, .motor_speed=0, .strobe=255),
	F(192000, .pan_deg=270, .tilt_deg= 30, .dimmer=255, .color=MH_COL_WHITE,  .motor_speed=0, .strobe=255),
	F(196000, .pan_deg=270, .tilt_deg=170, .dimmer=255, .color=MH_COL_WHITE,  .motor_speed=0),
	F(199000, .pan_deg=270, .tilt_deg=100, .dimmer=  0, .color=MH_COL_WHITE,  .motor_speed=0),
};
const size_t show_song_len = ARRAY_SIZE(show_song);

struct song_pair {
	const char              *name;
	const struct note       *song;
	const size_t            *song_len;
	const struct show_frame *show;
	const size_t            *show_len;
};

static const struct song_pair pairs[] = {
	{"sandstorm", sandstorm,          &sandstorm_len,          show_sandstorm, &show_sandstorm_len},
	{"song",      song,               &song_len,               show_song,      &show_song_len     },
	{"rickroll",  never_gonna,        &never_gonna_len,        show_rickroll,  &show_rickroll_len },
	{"nggyu",     never_gonna,        &never_gonna_len,        show_rickroll,  &show_rickroll_len },
	{"blue",      blue_dabadee,       &blue_dabadee_len,       show_blue,      &show_blue_len     },
	{"lamour",    lamour_toujours,    &lamour_toujours_len,    show_lamour,    &show_lamour_len   },
	{"rhythm",    rhythm_is_a_dancer, &rhythm_is_a_dancer_len, show_rhythm,    &show_rhythm_len   },
	{"dota",      dota,               &dota_len,               show_dota,      &show_dota_len     },
};

static int cmd_party(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(sh, "usage: party <sandstorm|rickroll>");
		return -EINVAL;
	}
	for (size_t i = 0; i < ARRAY_SIZE(pairs); i++) {
		if (strcmp(argv[1], pairs[i].name) == 0) {
			show_play(pairs[i].show, *pairs[i].show_len);
			sound_play(pairs[i].song, *pairs[i].song_len);
			shell_print(sh, "party: %s", pairs[i].name);
			return 0;
		}
	}
	shell_error(sh, "unknown '%s'", argv[1]);
	return -EINVAL;
}

static int cmd_party_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh); ARG_UNUSED(argc); ARG_UNUSED(argv);
	sound_stop();
	show_stop();
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(party_cmds,
	SHELL_CMD(stop, NULL, "stop song + show", cmd_party_stop),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_ARG_REGISTER(party, &party_cmds, "party <sandstorm|song|rickroll|blue|lamour|rhythm|dota> | party stop",
		       cmd_party, 2, 0);
