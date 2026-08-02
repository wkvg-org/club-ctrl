# club-ctrl

Firmware for Waveshare ESP32S3(-POE)-ETH-8DI-8RO being used as home automation controller for digital inputs and relay outputs.
This project is realizing typical use cases seen in small venues, discos and clubs. 

## Use Cases

### Area related altering 

Personnel in a defined area (e.g. bar, dance floor, entrance, ...) triggering an alarm which can be forwarded to different event receivers.
E.g. switching on alarm lights in a security office, sending pager messages or similar.

### Monitoring of dry contacts

Equipment using dry contacts e.g. leakage sensors can be integrated for alerting.

### Door Bell Monitoring

Elevator, Entry, Exit



## Architecture

- Ethernet Interface
  - DHCP client
  - DNS client - needed?
  - web server interface & rest api / mqtt
  - authentication & authorization
  - web client interface / webhooks/mqtt
  - firmware update
  - heartbeat via webhook/mqtt
- SD card
  - logging
  - configuration
  - credentials?
- RTC
  - Battery
  - NTP
- 

## Development

### building

in %HOME/zephyrproject/
`.venv\Scripts\Activate.ps1`

ensure that the base build path is as short as possible.

`west build --board esp32s3_eth_8di_8ro/esp32s3/procpu --build-dir N:\b`

### flash (rebuilding if necessary)

`west flash --esp-device COM4 --build-dir N:\b`

### connect terminal

`west espressif monitor -p COM4`

### disconnect terminal - Windows

`CTRL+T + CTRL+X`

## License

SPDX-License-Identifier: GPL-2.0-only

## Copyright

Copyright (C) 2025 - 2026 Wolfenbütteler Kultur- und Veranstaltungsgesellschaft gemeinnützige UG (haftungsbeschränkt)