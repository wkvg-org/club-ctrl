/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * MQTT subscriber for club-ctrl DMX control. Connects to the MQTT
 * broker at 192.168.2.128:1883, subscribes to dmx/#, and drives the
 * moving head via fixture.h on received messages.
 *
 * Topics (plain text payloads):
 *   dmx/color     <0..255>
 *   dmx/dimmer    <0..255>
 *   dmx/strobe    <0..255>
 *   dmx/pan       <0..540>
 *   dmx/tilt      <0..200>
 *   dmx/home      (ignored)
 *   dmx/blackout  (ignored)
 */

#ifndef MQTT_SUBSCRIBER_H_
#define MQTT_SUBSCRIBER_H_

int mqtt_subscriber_init(void);

#endif /* MQTT_SUBSCRIBER_H_ */
