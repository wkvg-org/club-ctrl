# club-ctrl

Firmware for the Waveshare ESP32-S3(-POE)-ETH-8DI-8RO board, used as a
home-automation / venue controller for digital inputs, relay outputs, DMX512
lighting and buzzer audio. Targets small venues, discos and clubs.

## Use Cases

### Area-related alerting

Personnel in a defined area (bar, dance floor, entrance, ...) trigger an alarm
that is forwarded to event receivers — e.g. security-office alarm lights,
pager messages, etc.

### Monitoring of dry contacts

Equipment with dry contacts (e.g. leakage sensors) can be integrated for
alerting.

### Door-bell monitoring

Elevator, entry, exit.

### DMX512 lighting

Drive a moving-head fixture (currently a Thomann PicoSpot 45 in 9-channel mode)
over RS485 from UART1. Per-song, time-keyframed light shows are dispatched
together with buzzer playback via the `party` shell command.

### Buzzer audio

Passive piezo buzzer driven by the LEDC PWM peripheral through Zephyr's
`pwm-buzzer` subsystem. Plays note-timed melodies from `sound/songs.c`.

## Architecture

### Hardware

- **MCU**: ESP32-S3 (procpu).
- **DMX out**: UART1 → RS485 transceiver (TX = GPIO17, RX = GPIO18). TXD is
  inverted at the pin matrix to compensate for A/B convention on the
  Waveshare transceiver vs. DMX XLR pin 2/3.
- **Buzzer**: LEDC channel 0 → GPIO46 (pwm-buzzer).
- **Ethernet**: W5500 over SPI.
- **I²C**: PCA9538 GPIO expander (relays), PCF85063A RTC.
- **SD**: ESP32 SDHC.

### Software modules (`src/modules/`)

| Module | Purpose |
|---|---|
| `output/dmx/` | DMX512 TX driver, 40 Hz refresh, universe buffer, `mh` shell for the moving head. |
| `output/sound/` | Buzzer playback thread, note tables (`songs.c`), startup chime, `sound` shell. |
| `show/` | Time-keyframed lighting-show player. Synchronises with buzzer via a unified `party` shell. |
| `config/`, `diag/`, `input/`, `led/`, `logging/`, `net/`, `ota/`, `sd/`, `sec/`, `time/` | Scaffolding for the remaining board features (see individual dirs). |

### Networking (planned)

- Ethernet (W5500) with DHCP, static-IP fallback.
- DNS client.
- REST / MQTT server + web UI.
- Auth (device credentials from SD).
- Firmware update (OTA module).
- Heartbeat via webhook / MQTT.

### Storage / time

- **SD card**: logs, configuration, credentials.
- **RTC**: PCF85063A with battery backup, disciplined by NTP once network is up.

## Shell commands

Once flashed, connect the serial console. Commands available:

```
mh pan <deg>          # 0..540
mh tilt <deg>         # 0..270
mh dimmer <0-255>
mh color <0-255>      # PicoSpot colour-wheel index
mh gobo <0-255>
mh strobe <0-255>     # 0-9 = open, 10-255 = strobe
mh motor <0-255>      # movement speed / smoothing
mh home
mh blackout
mh raw <ch> <val>
mh flood

sound play <song>
sound stop

party <song>          # start song + matching light show
party stop
```

Songs currently wired: see `src/modules/output/sound/songs.c` and
`src/modules/show/shows.c`.

## SD Card Configuration

SD card config loading is planned but not yet implemented. When done,
configuration will follow the same pattern as club-display using JSON files
on a FAT32 SD card at `/SD:`.

Planned files:

### `/SD:/club-ctrl.json`

```json
{
  "device_name": "club-ctrl",
  "mqtt_server_ip": "192.168.2.128",
  "mqtt_server_domain": "mqtt.moai.sk",
  "mqtt_server_port": 1883,
  "dmx_start_addr": 1,
  "ntp_server_domain_1": "192.53.103.108",
  "ntp_server_domain_2": "192.53.103.104",
  "ntp_server_domain_3": "192.53.103.103",
  "ntp_server_port": 123
}
```

### `/SD:/creds.json`

```json
{
  "mqtt": {
    "username": "",
    "password": ""
  }
}
```

## Development

### Toolchain

Zephyr workspace under `~/zephyrproject`, Zephyr SDK, west, esptool.

```powershell
# Windows
%HOME%/zephyrproject/.venv/Scripts/Activate.ps1
```

```bash
# Linux
source ~/zephyrproject/.venv/bin/activate
```

### Fetch Espressif blobs (once)

WiFi and BLE stacks require binary blobs:

```bash
west blobs fetch hal_espressif
```

### Build

Keep the build-dir path short (Windows CLI length limits):

```bash
west build --board esp32s3_eth_8di_8ro/esp32s3/procpu \
           --build-dir /tmp/b \
           /path/to/club-ctrl
```

Windows equivalent:

```powershell
west build --board esp32s3_eth_8di_8ro/esp32s3/procpu --build-dir N:\b
```

### Flash

```bash
west flash --build-dir /tmp/b --esp-device /dev/ttyACM0   # Linux
west flash --build-dir N:\b  --esp-device COM4            # Windows
```

The serial monitor must be disconnected before flashing.

### Serial monitor

```bash
west espressif monitor -p /dev/ttyACM0    # Linux
west espressif monitor -p COM4            # Windows
```

Disconnect on Windows: `CTRL+T`, then `CTRL+X`.

## License

SPDX-License-Identifier: Apache-2.0

## Copyright

Copyright (C) 2025 - 2026 Wolfenbütteler Kultur- und Veranstaltungsgesellschaft
gemeinnützige UG (haftungsbeschränkt)
