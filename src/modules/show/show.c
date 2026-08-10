/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Show runner. Absolute-deadline scheduling anchored to show_play() start,
 * mirroring the pattern in sound.c so audio and lights stay in sync.
 */

#include "show.h"
#include "../output/dmx/fixture.h"

#include <zephyr/kernel.h>

#define SHOW_TICK_MS 40

static struct k_mutex play_lock;
static struct k_sem   play_sem;
static volatile bool  stop_flag;

static const struct show_frame *g_frames;
static size_t                    g_frames_len;

/**
 * Linear interpolation between two unsigned values.
 * Returns a + (b - a) * num / den, clamped to the source type range.
 */
static uint8_t lerp_u8(uint8_t a, uint8_t b, uint32_t num, uint32_t den)
{
	if (den == 0) {
		return a;
	}
	int32_t diff = (int32_t)b - (int32_t)a;
	return (uint8_t)((int32_t)a + (diff * (int32_t)num) / (int32_t)den);
}

static uint16_t lerp_u16(uint16_t a, uint16_t b, uint32_t num, uint32_t den)
{
	if (den == 0) {
		return a;
	}
	int32_t diff = (int32_t)b - (int32_t)a;
	return (uint16_t)((int32_t)a + (diff * (int32_t)num) / (int32_t)den);
}

/**
 * Interpolate between two keyframes at time t_ms.
 * Continuous fields (pan, tilt, dimmer) are linearly interpolated;
 * discrete fields (motor, colour, gobo, strobe) snap from the earlier frame.
 */
static void interpolate(const struct show_frame *a, const struct show_frame *b,
			uint32_t t_ms, struct mh_state *out)
{
	uint32_t num = t_ms - a->at_ms;
	uint32_t den = b->at_ms - a->at_ms;

	out->pan_deg  = lerp_u16(a->state.pan_deg,  b->state.pan_deg,  num, den);
	out->tilt_deg = lerp_u8 (a->state.tilt_deg, b->state.tilt_deg, num, den);
	out->dimmer   = lerp_u8 (a->state.dimmer,   b->state.dimmer,   num, den);

	out->motor_speed = a->state.motor_speed;
	out->color       = a->state.color;
	out->gobo        = a->state.gobo;
	out->strobe      = a->state.strobe;
}

/**
 * Light-show playback thread. Absolute-deadline scheduling anchored to
 * show_play() start time keeps the lights in sync with the buzzer
 * (which uses the same pattern in sound.c). Runs at ~25 Hz tick rate.
 */
static void show_thread_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);

	while (true) {
		k_sem_take(&play_sem, K_FOREVER);

		const struct show_frame *frames = g_frames;
		size_t len = g_frames_len;

		int64_t t0 = k_uptime_get();
		int64_t deadline = t0;
		uint32_t last_at = frames[len - 1].at_ms;

		while (!stop_flag) {
			uint32_t t = (uint32_t)(k_uptime_get() - t0);
			if (t >= last_at) {
				break;
			}

			size_t i = 0;
			while (i + 1 < len && frames[i + 1].at_ms <= t) {
				i++;
			}

			struct mh_state s;
			if (i + 1 < len) {
				interpolate(&frames[i], &frames[i + 1], t, &s);
			} else {
				s = frames[i].state;
			}
			mh_set(&s);

			deadline += SHOW_TICK_MS;
			k_sleep(K_TIMEOUT_ABS_MS(deadline));
		}

		mh_blackout();
		stop_flag = false;
	}
}

#define SHOW_STACK_SIZE 1536
#define SHOW_PRIO       K_PRIO_COOP(8)
K_THREAD_STACK_DEFINE(show_stack, SHOW_STACK_SIZE);
static struct k_thread show_thread_data;

int show_init(void)
{
	k_mutex_init(&play_lock);
	k_sem_init(&play_sem, 0, 1);

	k_tid_t tid = k_thread_create(&show_thread_data, show_stack,
				      K_THREAD_STACK_SIZEOF(show_stack),
				      show_thread_fn, NULL, NULL, NULL,
				      SHOW_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(tid, "light_show");
	return 0;
}

int show_play(const struct show_frame *frames, size_t n)
{
	if (frames == NULL || n < 2) {
		return -EINVAL;
	}

	k_mutex_lock(&play_lock, K_FOREVER);
	stop_flag = true;
	k_msleep(SHOW_TICK_MS + 5);

	g_frames = frames;
	g_frames_len = n;
	stop_flag = false;
	k_sem_give(&play_sem);
	k_mutex_unlock(&play_lock);
	return 0;
}

void show_stop(void)
{
	stop_flag = true;
}
