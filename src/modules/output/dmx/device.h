/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generic DMX device registry.
 *
 * A "device" is a self-contained driver for one physical DMX device type
 * (moving head, RGBW PAR, ...). It owns a number of instances, each mapped
 * to a contiguous block of DMX channels starting at a configurable address,
 * and translates generic commands into slot writes.
 *
 * To add a device:
 *   1. Give it an id in enum dmx_device_id below.
 *   2. Implement a module with a `struct dmx_device` (apply + get_addr +
 *      param table) and register it from an init function.
 *   3. Call that init function from main() (or your own init path).
 *
 * The zbus listener and the MQTT subscriber only ever talk to this
 * registry, so new devices need no changes to shared control code.
 */

#ifndef DMX_DEVICE_H_
#define DMX_DEVICE_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Registry capacity (number of distinct device types). */
#define DMX_DEVICE_MAX 8

/* Device types known to the firmware. Extend as devices are added. */
enum dmx_device_id {
	DMX_DEV_MOVING_HEAD = 0,
	DMX_DEV_RGBW_PAR,
	DMX_DEV_COUNT,
};

/* A named, device-specific parameter ("pan", "red", ...). */
struct dmx_param {
	const char *name;
	uint16_t    id;
};

/*
 * Device driver interface. `apply` receives an instance index, a
 * device-specific parameter id and a value; home/blackout/address are
 * ordinary parameters (e.g. MH_PARAM_HOME) so the interface stays uniform.
 */
struct dmx_device {
	const char *name;      /* short name used in MQTT topics / shells */
	uint8_t     id;        /* enum dmx_device_id */
	uint8_t     max_inst;  /* number of instances */

	void     (*apply)(uint8_t inst, uint16_t param, uint16_t val);
	uint16_t (*get_addr)(uint8_t inst);  /* current DMX start address */

	const struct dmx_param *params;
	size_t                  num_params;
};

/* Registration + lookup. */
void dmx_device_register(const struct dmx_device *dev);
const struct dmx_device *dmx_device_by_id(uint8_t id);
const struct dmx_device *dmx_device_by_name(const char *name);

/* Route a generic command to the owning device. */
void dmx_device_dispatch(uint8_t id, uint8_t inst, uint16_t param, uint16_t val);

/* Helpers used by the MQTT topic parser. */
uint16_t dmx_device_get_addr(const struct dmx_device *dev, uint8_t inst);
bool dmx_device_find_inst(const struct dmx_device *dev, uint16_t addr,
			  uint8_t *inst);
bool dmx_device_lookup_param(const struct dmx_device *dev, const char *name,
			     uint16_t *id);

#endif /* DMX_DEVICE_H_ */
