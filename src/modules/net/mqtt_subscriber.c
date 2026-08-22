/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mqtt_subscriber.h"
#include "../config/config.h"
#include "../zbus/ctrl_zbus.h"
#include "../output/dmx/device.h"

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
		LOG_INF("MQTT connected to %s:%u", config_mqtt_broker_ip(), config_mqtt_broker_port());

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
		char topic[40] = {0};

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
		topic[tlen] = '\0';

		long val = 0;
		if (plen > 0) {
			val = strtol(payload, NULL, 10);
		}

		/* ── dmx/fixture/<addr> — raw relay slot write ────────────── */
		if (strncmp(topic, "dmx/fixture/", 12) == 0) {
			long addr = strtol(topic + 12, NULL, 10);
			if (addr < 1 || addr > 504) {
				break;
			}
			struct dmx_ctrl_cmd relay_cmd = {
				.type = DMX_CTRL_RAW,
				.ch   = (uint16_t)addr,
				.val  = (uint16_t)val,
			};
			zbus_chan_pub(&dmx_ctrl_chan, &relay_cmd, K_NO_WAIT);
			printk("[MQTT] fixture addr %ld = %ld\n", addr, val);
			break;
		}

		/* ── dmx/<device>/<addr>/<param> — routed device control ──── */
		if (strncmp(topic, "dmx/", 4) == 0) {
			char *p = topic + 4;                  /* <device>/<addr>/<param> */
			char *slash1 = strchr(p, '/');
			char *slash2 = slash1 ? strchr(slash1 + 1, '/') : NULL;

			if (slash1 != NULL && slash2 != NULL) {
				*slash1 = '\0';
				*slash2 = '\0';

				const char *devname = p;
				long addr = strtol(slash1 + 1, NULL, 10);
				const char *param = slash2 + 1;

				const struct dmx_device *dev = dmx_device_by_name(devname);
				if (dev == NULL) {
					LOG_WRN("unknown DMX device %s", devname);
					break;
				}

				uint8_t inst;
				if (!dmx_device_find_inst(dev, (uint16_t)addr, &inst)) {
					LOG_WRN("unknown %s addr %ld", devname, addr);
					break;
				}

				uint16_t pid;
				if (!dmx_device_lookup_param(dev, param, &pid)) {
					LOG_WRN("unknown %s param %s", devname, param);
					break;
				}

				struct dmx_ctrl_cmd cmd = {
					.type  = DMX_CTRL_DEV,
					.dev   = dev->id,
					.inst  = inst,
					.param = pid,
					.val   = (uint16_t)val,
				};
				zbus_chan_pub(&dmx_ctrl_chan, &cmd, K_NO_WAIT);
				printk("[MQTT] %s addr %ld %s = %ld\n",
				       devname, addr, param, val);
				break;
			}
		}

		/* ── Legacy flat topics (dmx/color etc.) → moving head 0 ──── */
		{
			const struct dmx_device *mh = dmx_device_by_name("mh");
			const char *param = NULL;

			if (strstr(topic, "color")) {
				param = "color";
			} else if (strstr(topic, "dimmer")) {
				param = "dimmer";
			} else if (strstr(topic, "strobe")) {
				param = "strobe";
			} else if (strstr(topic, "pan")) {
				param = "pan";
			} else if (strstr(topic, "tilt")) {
				param = "tilt";
			} else if (strstr(topic, "home")) {
				param = "home";
			} else if (strstr(topic, "blackout")) {
				param = "blackout";
			}

			uint16_t pid;
			if (param != NULL && mh != NULL &&
			    dmx_device_lookup_param(mh, param, &pid)) {
				struct dmx_ctrl_cmd cmd = {
					.type  = DMX_CTRL_DEV,
					.dev   = mh->id,
					.inst  = 0,
					.param = pid,
					.val   = (uint16_t)val,
				};
				zbus_chan_pub(&dmx_ctrl_chan, &cmd, K_NO_WAIT);
			}
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
	broker4->sin_port = htons(config_mqtt_broker_port());
	inet_pton(AF_INET, config_mqtt_broker_ip(), &broker4->sin_addr);

	client.broker = &broker_addr;
	client.evt_cb = mqtt_evt_handler;
	client.client_id.utf8 = (uint8_t *)config_device_name();
	client.client_id.size = strlen(config_device_name());
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

	printk("[MQTT] subscriber starting (broker %s:%u)\n", config_mqtt_broker_ip(), config_mqtt_broker_port());

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
