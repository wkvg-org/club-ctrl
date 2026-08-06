/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DMX_H_
#define DMX_H_

#include <stddef.h>
#include <stdint.h>

#define DMX_UNIVERSE_SIZE 512

int dmx_init(void);
void dmx_set_slot(uint16_t ch, uint8_t value);
void dmx_set_slots(uint16_t start, const uint8_t *buf, size_t n);
void dmx_blackout(void);

#endif
