/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SD card configuration loader. Reads club-ctrl.json + creds.json
 * from /SD:/ at boot. Falls back to built-in defaults.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <zephyr/data/json.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(config, LOG_LEVEL_INF);

/* ── Defaults ───────────────────────────────────────────────────────── */

#define DEFAULT_DEVICE_NAME    "club-ctrl"
#define DEFAULT_MQTT_BROKER_IP "192.168.2.128"
#define DEFAULT_MQTT_PORT      1883
#define DEFAULT_DMX_START_ADDR 1

static const char *DEFAULT_NTP[] = {
	"192.53.103.108",
	"192.53.103.104",
	"192.53.103.103",
};

/* ── Global state ───────────────────────────────────────────────────── */

static char     g_device_name[64] = DEFAULT_DEVICE_NAME;
static char     g_mqtt_broker_ip[16] = DEFAULT_MQTT_BROKER_IP;
static uint16_t g_mqtt_port = DEFAULT_MQTT_PORT;
static uint16_t g_dmx_addr = DEFAULT_DMX_START_ADDR;
static char     g_ntp[3][128];
static char     g_mqtt_user[64];
static char     g_mqtt_pass[64];

/* ── JSON: club-ctrl.json ───────────────────────────────────────────── */

static const char *CONFIG_PATH  = "/SD:/club-ctrl.json";
static const char *CREDS_PATH   = "/SD:/creds.json";

struct ctrl_config {
	char     device_name[64];
	char     mqtt_broker_ip[16];
	uint16_t mqtt_broker_port;
	uint16_t dmx_start_addr;
	char     ntp_server_1[128];
	char     ntp_server_2[128];
	char     ntp_server_3[128];
};

static const struct json_obj_descr cfg_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct ctrl_config, device_name,     JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct ctrl_config, mqtt_broker_ip,  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct ctrl_config, mqtt_broker_port, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ctrl_config, dmx_start_addr,  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ctrl_config, ntp_server_1,    JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct ctrl_config, ntp_server_2,    JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct ctrl_config, ntp_server_3,    JSON_TOK_STRING),
};

/* ── JSON: creds.json ───────────────────────────────────────────────── */

struct creds_mqtt {
	char username[64];
	char password[64];
};

static const struct json_obj_descr creds_mqtt_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct creds_mqtt, username, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct creds_mqtt, password, JSON_TOK_STRING),
};

struct creds {
	struct creds_mqtt mqtt;
};

static const struct json_obj_descr creds_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct creds, mqtt, creds_mqtt_descr),
};

/* ── Helpers ────────────────────────────────────────────────────────── */

static bool read_json_file(const char *path, char *buf, size_t buf_sz)
{
	struct fs_file_t f;
	ssize_t n;

	fs_file_t_init(&f);
	if (fs_open(&f, path, FS_O_READ) < 0) {
		return false;
	}
	n = fs_read(&f, buf, buf_sz - 1);
	fs_close(&f);
	if (n <= 0) {
		return false;
	}
	buf[n] = '\0';
	return true;
}

static void copy_str(char *dst, size_t dst_sz, const char *src)
{
	if (src != NULL && src[0] != '\0') {
		strncpy(dst, src, dst_sz - 1);
		dst[dst_sz - 1] = '\0';
	}
}

/* ── Loaders ────────────────────────────────────────────────────────── */

static void load_club_ctrl_json(void)
{
	char buf[1024];
	struct ctrl_config cfg;

	if (!read_json_file(CONFIG_PATH, buf, sizeof(buf))) {
		LOG_INF("no %s on SD, using defaults", CONFIG_PATH);
		return;
	}

	memset(&cfg, 0, sizeof(cfg));
	if (json_obj_parse(buf, strlen(buf), cfg_descr,
			   ARRAY_SIZE(cfg_descr), &cfg) < 0) {
		LOG_WRN("failed to parse %s", CONFIG_PATH);
		return;
	}

	copy_str(g_device_name, sizeof(g_device_name), cfg.device_name);
	copy_str(g_mqtt_broker_ip, sizeof(g_mqtt_broker_ip), cfg.mqtt_broker_ip);
	if (cfg.mqtt_broker_port != 0) {
		g_mqtt_port = cfg.mqtt_broker_port;
	}
	if (cfg.dmx_start_addr >= 1 && cfg.dmx_start_addr <= 504) {
		g_dmx_addr = cfg.dmx_start_addr;
	}
	copy_str(g_ntp[0], sizeof(g_ntp[0]), cfg.ntp_server_1);
	copy_str(g_ntp[1], sizeof(g_ntp[1]), cfg.ntp_server_2);
	copy_str(g_ntp[2], sizeof(g_ntp[2]), cfg.ntp_server_3);

	LOG_INF("config loaded from %s", CONFIG_PATH);
}

static void load_creds(void)
{
	char buf[512];
	struct creds c;

	if (!read_json_file(CREDS_PATH, buf, sizeof(buf))) {
		LOG_INF("no %s on SD", CREDS_PATH);
		return;
	}

	memset(&c, 0, sizeof(c));
	if (json_obj_parse(buf, strlen(buf), creds_descr,
			   ARRAY_SIZE(creds_descr), &c) < 0) {
		LOG_WRN("failed to parse %s", CREDS_PATH);
		return;
	}

	if (c.mqtt.username[0]) {
		strncpy(g_mqtt_user, c.mqtt.username, sizeof(g_mqtt_user) - 1);
	}
	if (c.mqtt.password[0]) {
		strncpy(g_mqtt_pass, c.mqtt.password, sizeof(g_mqtt_pass) - 1);
	}

	LOG_INF("credentials loaded from %s", CREDS_PATH);
}

/* ── Public API ─────────────────────────────────────────────────────── */

void config_init(void)
{
	struct fs_dir_t dir;

	fs_dir_t_init(&dir);
	if (fs_opendir(&dir, "/SD:") < 0) {
		LOG_WRN("no SD card present, using defaults");
		return;
	}
	fs_closedir(&dir);
	LOG_INF("SD card present");

	load_club_ctrl_json();
	load_creds();

	for (int i = 0; i < 3; i++) {
		if (g_ntp[i][0] == '\0') {
			copy_str(g_ntp[i], sizeof(g_ntp[i]), DEFAULT_NTP[i]);
		}
	}
}

const char *config_device_name(void)    { return g_device_name; }
const char *config_mqtt_broker_ip(void) { return g_mqtt_broker_ip; }
uint16_t    config_mqtt_broker_port(void){ return g_mqtt_port; }
const char *config_mqtt_username(void)  { return g_mqtt_user; }
const char *config_mqtt_password(void)  { return g_mqtt_pass; }
uint16_t    config_dmx_start_addr(void) { return g_dmx_addr; }

const char *config_ntp_server(uint8_t idx)
{
	if (idx >= 3) idx = 0;
	return g_ntp[idx];
}
