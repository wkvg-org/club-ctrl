/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SD card configuration loader. Reads club-ctrl.json + creds.json
 * from the SD card at boot. Falls back to built-in defaults when
 * files are absent or parsing fails.
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>

void config_init(void);

const char *config_device_name(void);
const char *config_mqtt_broker_ip(void);
const char *config_mqtt_broker_domain(void);
uint16_t    config_mqtt_broker_port(void);
const char *config_mqtt_username(void);
const char *config_mqtt_password(void);
uint16_t    config_dmx_start_addr(void);
const char *config_ntp_server(uint8_t idx);

#endif /* CONFIG_H_ */
