/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * MQTT subscriber for club-ctrl DMX control. Connects to the MQTT
 * broker, subscribes to dmx/#, and routes messages to registered DMX
 * devices via the device registry.
 *
 * Topics (plain text payloads):
 *   dmx/<device>/<addr>/<param>   routed device control, e.g.
 *     dmx/mh/1/color  dmx/mh/10/pan  dmx/par/19/red
 *   dmx/fixture/<addr>            raw DMX slot write (relay fixture)
 *   legacy flat topics (dmx/color, dmx/dimmer, ...) → moving head 0
 */

#ifndef MQTT_SUBSCRIBER_H_
#define MQTT_SUBSCRIBER_H_

int mqtt_subscriber_init(void);

#endif /* MQTT_SUBSCRIBER_H_ */
