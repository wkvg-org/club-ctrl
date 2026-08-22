/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generic DMX device registry implementation. Keeps a small static table
 * of registered device drivers and routes commands to them.
 */

#include "device.h"

#include <zephyr/sys/util.h>
#include <string.h>

static const struct dmx_device *g_devices[DMX_DEVICE_MAX];
static size_t g_count;

void dmx_device_register(const struct dmx_device *dev)
{
	if (dev == NULL || g_count >= ARRAY_SIZE(g_devices)) {
		return;
	}
	g_devices[g_count++] = dev;
}

const struct dmx_device *dmx_device_by_id(uint8_t id)
{
	for (size_t i = 0; i < g_count; i++) {
		if (g_devices[i]->id == id) {
			return g_devices[i];
		}
	}
	return NULL;
}

const struct dmx_device *dmx_device_by_name(const char *name)
{
	if (name == NULL) {
		return NULL;
	}
	for (size_t i = 0; i < g_count; i++) {
		if (strcmp(g_devices[i]->name, name) == 0) {
			return g_devices[i];
		}
	}
	return NULL;
}

void dmx_device_dispatch(uint8_t id, uint8_t inst, uint16_t param, uint16_t val)
{
	const struct dmx_device *dev = dmx_device_by_id(id);

	if (dev == NULL || inst >= dev->max_inst || dev->apply == NULL) {
		return;
	}
	dev->apply(inst, param, val);
}

uint16_t dmx_device_get_addr(const struct dmx_device *dev, uint8_t inst)
{
	if (dev == NULL || inst >= dev->max_inst || dev->get_addr == NULL) {
		return 0;
	}
	return dev->get_addr(inst);
}

bool dmx_device_find_inst(const struct dmx_device *dev, uint16_t addr,
			  uint8_t *inst)
{
	if (dev == NULL) {
		return false;
	}
	for (uint8_t i = 0; i < dev->max_inst; i++) {
		if (dmx_device_get_addr(dev, i) == addr) {
			if (inst != NULL) {
				*inst = i;
			}
			return true;
		}
	}
	return false;
}

bool dmx_device_lookup_param(const struct dmx_device *dev, const char *name,
			     uint16_t *id)
{
	if (dev == NULL || name == NULL) {
		return false;
	}
	for (size_t i = 0; i < dev->num_params; i++) {
		if (strcmp(dev->params[i].name, name) == 0) {
			if (id != NULL) {
				*id = dev->params[i].id;
			}
			return true;
		}
	}
	return false;
}
