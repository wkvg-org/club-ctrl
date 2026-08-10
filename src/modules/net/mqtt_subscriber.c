/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mqtt_subscriber.h"
#include "../output/dmx/fixture.h"

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/poll.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

LOG_MODULE_REGISTER(mqtt_sub, LOG_LEVEL_INF);

#define BROKER_IP   "192.168.2.128"
#define BROKER_PORT 1883
#define CLIENT_ID   "club-ctrl"
#define KEEPALIVE   60
#define STACK_SIZE  4096
#define RX_BUF_SIZE 512
#define TX_BUF_SIZE 512

static K_THREAD_STACK_DEFINE(mqtt_stack, STACK_SIZE);
static struct k_thread mqtt_thread;

static struct mqtt_client client;
static struct sockaddr_storage broker_addr;
static uint8_t rx_buf[RX_BUF_SIZE];
static uint8_t tx_buf[TX_BUF_SIZE];
static bool connected;

static void mqtt_evt_handler(struct mqtt_client *c, const struct mqtt_evt *evt)
{
	ARG_UNUSED(c);

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT connect failed: %d", evt->result);
			break;
		}
		connected = true;
		LOG_INF("MQTT connected to %s:%u", BROKER_IP, BROKER_PORT);

		/* Subscribe to all DMX topics */
		struct mqtt_subscription_list sub_list = {
			.list = (struct mqtt_topic[]) {
				{
					.topic = {
						.utf8 = (uint8_t *)"dmx/#",
						.size = 5,
					},
					.qos = MQTT_QOS_0_AT_MOST_ONCE,
				},
			},
			.list_count = 1,
			.message_id = 1,
		};
		mqtt_subscribe(&client, &sub_list);
		break;

	case MQTT_EVT_DISCONNECT:
		LOG_INF("MQTT disconnected");
		connected = false;
		break;

	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *p = &evt->param.publish;
		char payload[32] = {0};
		char topic[32] = {0};

		uint16_t plen = p->message.payload.len;
		if (plen > sizeof(payload) - 1) {
			plen = sizeof(payload) - 1;
		}

		int err = mqtt_read_publish_payload(&client, payload, plen);
		if (err < 0) {
			return;
		}

		uint16_t tlen = p->message.topic.topic.size;
		if (tlen > sizeof(topic) - 1) {
			tlen = sizeof(topic) - 1;
		}
		memcpy(topic, p->message.topic.topic.utf8, tlen);

		long val = 0;
		if (plen > 0) {
			val = strtol(payload, NULL, 10);
		}

		/*
		 * Parse MQTT topic suffix (e.g. "dmx/color" → colour,
		 * "dmx/dimmer" → dimmer) and update g_mh_state accordingly.
		 * Topic wildcard "dmx/#" captures all sub-topics.
		 */
		if (strstr(topic, "color")) {
			g_mh_state.color = (uint8_t)val;
			mh_set(&g_mh_state);
			printk("[MQTT] color = %ld\n", val);
			LOG_WRN("club-ctrl: MQTT DMX color = %ld", val);
		} else if (strstr(topic, "dimmer")) {
			g_mh_state.dimmer = (uint8_t)val;
			mh_set(&g_mh_state);
			printk("[MQTT] dimmer = %ld\n", val);
			LOG_WRN("club-ctrl: MQTT DMX dimmer = %ld", val);
		} else if (strstr(topic, "strobe")) {
			g_mh_state.strobe = (uint8_t)val;
			mh_set(&g_mh_state);
			printk("[MQTT] strobe = %ld\n", val);
			LOG_WRN("club-ctrl: MQTT DMX strobe = %ld", val);
		} else if (strstr(topic, "pan")) {
			g_mh_state.pan_deg = (uint16_t)val;
			mh_set(&g_mh_state);
			printk("[MQTT] pan = %ld\n", val);
			LOG_WRN("club-ctrl: MQTT DMX pan = %ld", val);
		} else if (strstr(topic, "tilt")) {
			g_mh_state.tilt_deg = (uint8_t)val;
			mh_set(&g_mh_state);
			printk("[MQTT] tilt = %ld\n", val);
			LOG_WRN("club-ctrl: MQTT DMX tilt = %ld", val);
		} else if (strstr(topic, "home")) {
			mh_home();
			g_mh_state = (struct mh_state){
				.pan_deg = 270, .tilt_deg = 100,
				.motor_speed = 0, .color = MH_COL_WHITE,
				.dimmer = 255,
			};
			printk("[MQTT] home\n");
			LOG_WRN("club-ctrl: MQTT DMX home");
		} else if (strstr(topic, "blackout")) {
			mh_blackout();
			g_mh_state.dimmer = 0;
			printk("[MQTT] blackout\n");
			LOG_WRN("club-ctrl: MQTT DMX blackout");
		}
		break;
	}

	case MQTT_EVT_SUBACK:
		LOG_INF("MQTT subscribed to dmx/#");
		break;

	default:
		break;
	}
}

/**
 * Attempt a single MQTT connection + CONNACK handshake (5 s timeout).
 * Subscribes to dmx/# and waits for the broker to acknowledge.
 */
static int try_connect(void)
{
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker_addr;

	mqtt_client_init(&client);

	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(BROKER_PORT);
	inet_pton(AF_INET, BROKER_IP, &broker4->sin_addr);

	client.broker = &broker_addr;
	client.evt_cb = mqtt_evt_handler;
	client.client_id.utf8 = (uint8_t *)CLIENT_ID;
	client.client_id.size = strlen(CLIENT_ID);
	client.protocol_version = MQTT_VERSION_3_1_1;
	client.keepalive = KEEPALIVE;
	client.rx_buf = rx_buf;
	client.rx_buf_size = sizeof(rx_buf);
	client.tx_buf = tx_buf;
	client.tx_buf_size = sizeof(tx_buf);
	client.transport.type = MQTT_TRANSPORT_NON_SECURE;

		int ret = mqtt_connect(&client);
	if (ret < 0) {
		LOG_ERR("mqtt_connect failed: %d", ret);
		return ret;
	}

	struct pollfd fds[1];
	fds[0].fd = client.transport.tcp.sock;
	fds[0].events = POLLIN;

	/* Wait for CONNACK */
	int64_t start = k_uptime_get();
	while (!connected && (k_uptime_get() - start) < 5000) {
		if (poll(fds, 1, 100) > 0) {
			mqtt_input(&client);
		}
	}

	if (!connected) {
		mqtt_abort(&client);
		return -1;
	}

	return 0;
}

static void mqtt_thread_fn(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1); ARG_UNUSED(arg2); ARG_UNUSED(arg3);

	printk("[MQTT] subscriber starting (broker %s:%u)\n", BROKER_IP, BROKER_PORT);

	struct pollfd fds[1];

	while (1) {
		if (!connected) {
			try_connect();
			k_sleep(K_SECONDS(3));
			continue;
		}

		fds[0].fd = client.transport.tcp.sock;
		fds[0].events = POLLIN;

		int ret = poll(fds, 1, 1000);
		if (ret > 0) {
			ret = mqtt_input(&client);
			if (ret < 0) {
				LOG_WRN("mqtt_input error: %d", ret);
				connected = false;
				continue;
			}
		}

		ret = mqtt_live(&client);
		if (ret < 0 && ret != -EAGAIN) {
			LOG_WRN("mqtt_live error: %d", ret);
			connected = false;
		}
	}
}

int mqtt_subscriber_init(void)
{
	k_tid_t tid = k_thread_create(&mqtt_thread, mqtt_stack,
				      K_THREAD_STACK_SIZEOF(mqtt_stack),
				      mqtt_thread_fn,
				      NULL, NULL, NULL,
				      5, 0, K_NO_WAIT);
	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		k_thread_name_set(tid, "mqtt_sub");
	}
	return 0;
}
