/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * UDP server for the club-ctrl. Listens on port 9090 for simple text
 * commands that control the moving head. The main use is receiving colour
 * changes from the club-display dashboard over Ethernet.
 *
 * Protocol (line-oriented, ASCII, '\n' terminator):
 *   C:<dmx_value>     set the colour wheel (0..255), e.g. C:60 for blue
 *   D:<0..255>        set dimmer
 *   S:<0..255>        set strobe
 *   P:<0..540>        set pan
 *   T:<0..200>        set tilt
 *   HOME              recenter, white, full brightness
 *   BLACK             blackout (dimmer=0)
 */

#include "udp_server.h"
#include "../output/dmx/fixture.h"

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(udp_srv, LOG_LEVEL_INF);

#define UDP_PORT    9090
#define BUF_SIZE    64
#define STACK_SIZE  2048

static K_THREAD_STACK_DEFINE(udp_stack, STACK_SIZE);
static struct k_thread udp_thread;

static int parse_byte(const char *s, uint8_t *out)
{
	char *end;
	long v = strtol(s, &end, 10);

	if (end == s || *end != '\n' || v < 0 || v > 255) {
		return -1;
	}
	*out = (uint8_t)v;
	return 0;
}

static void udp_server_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1); ARG_UNUSED(arg2); ARG_UNUSED(arg3);

	int sock;
	struct sockaddr bind_addr = {
		.sa_family = AF_INET,
	};

	/* bind to port 9090 on any interface */
	net_sin(&bind_addr)->sin_port = htons(UDP_PORT);
	net_sin(&bind_addr)->sin_addr.s_addr = INADDR_ANY;

	sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("UDP socket create failed: %d", sock);
		return;
	}

	if (zsock_bind(sock, &bind_addr, sizeof(struct sockaddr_in)) < 0) {
		LOG_ERR("UDP bind failed");
		zsock_close(sock);
		return;
	}

	printk("[DMX] UDP server listening on port %u\n", UDP_PORT);
	LOG_WRN("club-ctrl: UDP server running on port %u", UDP_PORT);

	char buf[BUF_SIZE];

	while (1) {
		struct sockaddr src_addr;
		socklen_t src_len = sizeof(struct sockaddr_in);

		ssize_t n = zsock_recvfrom(sock, buf, BUF_SIZE - 1, 0,
					  &src_addr, &src_len);
		if (n <= 0) {
			continue;
		}
		buf[n] = '\0';

		uint8_t val;

		switch (buf[0]) {
		case 'C':
			if (buf[1] == ':' && parse_byte(buf + 2, &val) == 0) {
				g_mh_state.color = val;
				mh_set(&g_mh_state);
				printk("[DMX] color = %u\n", val);
				LOG_WRN("club-ctrl: UDP DMX color = %u", val);
			}
			break;
		case 'D':
			if (buf[1] == ':' && parse_byte(buf + 2, &val) == 0) {
				g_mh_state.dimmer = val;
				mh_set(&g_mh_state);
				printk("[DMX] dimmer = %u\n", val);
				LOG_WRN("club-ctrl: UDP DMX dimmer = %u", val);
			}
			break;
		case 'S':
			if (buf[1] == ':' && parse_byte(buf + 2, &val) == 0) {
				g_mh_state.strobe = val;
				mh_set(&g_mh_state);
				printk("[DMX] strobe = %u\n", val);
				LOG_WRN("club-ctrl: UDP DMX strobe = %u", val);
			}
			break;
		case 'P':
			if (buf[1] == ':' && n > 2) {
				char *end;
				long pv = strtol(buf + 2, &end, 10);

				if (*end == '\n' && pv >= 0 && pv <= 540) {
					g_mh_state.pan_deg = (uint16_t)pv;
					mh_set(&g_mh_state);
					printk("[DMX] pan = %ld\n", pv);
					LOG_WRN("club-ctrl: UDP DMX pan = %ld", pv);
				}
			}
			break;
		case 'T':
			if (buf[1] == ':' && n > 2) {
				char *end;
				long tv = strtol(buf + 2, &end, 10);

				if (*end == '\n' && tv >= 0 && tv <= 200) {
					g_mh_state.tilt_deg = (uint8_t)tv;
					mh_set(&g_mh_state);
					printk("[DMX] tilt = %ld\n", tv);
					LOG_WRN("club-ctrl: UDP DMX tilt = %ld", tv);
				}
			}
			break;
		case 'H':
			mh_home();
			g_mh_state = (struct mh_state){
				.pan_deg = 270, .tilt_deg = 100,
				.motor_speed = 0, .color = MH_COL_WHITE,
				.dimmer = 255,
			};
			printk("[DMX] home\n");
			LOG_WRN("club-ctrl: UDP DMX home");
			break;
		case 'B':
			mh_blackout();
			g_mh_state.dimmer = 0;
			printk("[DMX] blackout\n");
			LOG_WRN("club-ctrl: UDP DMX blackout");
			break;
		default:
			break;
		}
	}
}

int udp_server_init(void)
{
	k_tid_t tid = k_thread_create(&udp_thread, udp_stack,
				      K_THREAD_STACK_SIZEOF(udp_stack),
				      udp_server_thread,
				      NULL, NULL, NULL,
				      5, 0, K_NO_WAIT);
	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		k_thread_name_set(tid, "udp_srv");
	}
	return 0;
}
