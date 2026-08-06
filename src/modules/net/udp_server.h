/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * UDP server that receives moving-head commands and forwards them to the
 * DMX fixture driver. Protocol: "C:<dmx_value>\n" (e.g. "C:60\n" for blue).
 */

#ifndef UDP_SERVER_H_
#define UDP_SERVER_H_

int udp_server_init(void);

#endif /* UDP_SERVER_H_ */
