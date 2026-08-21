/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Song player on top of Zephyr's buzzer subsystem. buzzer_tone() is
 * non-blocking (it programs the hardware and returns), so we own note
 * timing here via k_msleep() from a dedicated cooperative thread.
 */

#include "sound.h"
#include "../../zbus/ctrl_zbus.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/buzzer.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#include <string.h>

LOG_MODULE_REGISTER(sound, LOG_LEVEL_INF);

#define BUZZER_NODE DT_ALIAS(buzzer0)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(BUZZER_NODE),
	     "buzzer0 alias missing or disabled");

static const struct device *const buz = DEVICE_DT_GET(BUZZER_NODE);

#define SOUND_STACK_SIZE 1024
#define SOUND_PRIO       K_PRIO_COOP(9)

K_THREAD_STACK_DEFINE(sound_stack, SOUND_STACK_SIZE);
static struct k_thread sound_thread_data;
static k_tid_t sound_tid;

static struct k_mutex play_lock;
static struct k_sem play_sem;
static volatile bool stop_flag;

static const struct note *g_song;
static size_t g_song_len;

static void sound_thread_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);

	while (true) {
		k_sem_take(&play_sem, K_FOREVER);

		const struct note *song = g_song;
		size_t len = g_song_len;

		/*
		 * Absolute-deadline scheduling: every note's end-time is
		 * anchored to a single monotonic reference, so per-call
		 * driver overhead (LEDC reprogramming, k_work cancel/reschedule)
		 * cannot accumulate into audible drift.
		 */
		int64_t deadline_ms = k_uptime_get();

		for (size_t i = 0; i < len; i++) {
			if (stop_flag) {
				break;
			}
			uint32_t f = song[i].freq_hz;
			uint32_t ms = song[i].ms;

			if (f == 0U) {
				buzzer_stop(buz);
			} else {
				buzzer_tone(buz, f, BUZZER_DURATION_FOREVER);
			}
			deadline_ms += ms;
			k_sleep(K_TIMEOUT_ABS_MS(deadline_ms));
		}
		buzzer_stop(buz);
		stop_flag = false;
	}
}

int sound_init(void)
{
	if (!device_is_ready(buz)) {
		LOG_ERR("buzzer device not ready");
		return -ENODEV;
	}

	k_mutex_init(&play_lock);
	k_sem_init(&play_sem, 0, 1);

	int err = buzzer_set_volume(buz, 60);
	if (err) {
		LOG_WRN("buzzer_set_volume: %d", err);
	}

	sound_tid = k_thread_create(&sound_thread_data, sound_stack,
				    K_THREAD_STACK_SIZEOF(sound_stack),
				    sound_thread_fn, NULL, NULL, NULL,
				    SOUND_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(sound_tid, "sound");
	return 0;
}

int sound_play(const struct note *song, size_t n)
{
	if (song == NULL || n == 0) {
		return -EINVAL;
	}

	k_mutex_lock(&play_lock, K_FOREVER);
	stop_flag = true;
	buzzer_stop(buz);
	k_msleep(5);

	g_song = song;
	g_song_len = n;
	stop_flag = false;
	k_sem_give(&play_sem);
	k_mutex_unlock(&play_lock);
	return 0;
}

void sound_stop(void)
{
	stop_flag = true;
	buzzer_stop(buz);
}

/* ---------------- Shell ---------------- */

static int cmd_sound_play(const struct shell *sh, size_t argc, char **argv)
{
	struct sound_cmd cmd = { .type = SOUND_CMD_PLAY };

	if (argc < 2) {
		shell_error(sh, "usage: sound play <sandstorm|chime>");
		return -EINVAL;
	}
	if (strcmp(argv[1], "sandstorm") == 0) {
		cmd.song = sandstorm;
		cmd.len  = sandstorm_len;
	} else if (strcmp(argv[1], "chime") == 0) {
		cmd.song = startup_chime;
		cmd.len  = startup_chime_len;
	} else if (strcmp(argv[1], "rickroll") == 0 ||
		   strcmp(argv[1], "nggyu") == 0) {
		cmd.song = never_gonna;
		cmd.len  = never_gonna_len;
	} else {
		shell_error(sh, "unknown song '%s'", argv[1]);
		return -EINVAL;
	}
	zbus_chan_pub(&sound_chan, &cmd, K_NO_WAIT);
	return 0;
}

static int cmd_sound_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh); ARG_UNUSED(argc); ARG_UNUSED(argv);
	struct sound_cmd cmd = { .type = SOUND_CMD_STOP };
	zbus_chan_pub(&sound_chan, &cmd, K_NO_WAIT);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sound_cmds,
	SHELL_CMD_ARG(play, NULL, "sound play <sandstorm|chime>",
		      cmd_sound_play, 2, 0),
	SHELL_CMD_ARG(stop, NULL, "sound stop", cmd_sound_stop, 1, 0),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(sound, &sound_cmds, "Buzzer song player", NULL);
