# club-ctrl

Firmware for the Waveshare ESP32-S3(-POE)-ETH-8DI-8RO board, used as a
home-automation / venue controller for DMX512 lighting, buzzer audio,
relay outputs and digital inputs. Targets small venues, discos and clubs.

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

Drives two Thomann PicoSpot 45 moving heads (9-channel mode) over RS485 from
UART1, using the shared `dmx-z` driver (TX only). Per-song, time-keyframed
light shows are dispatched together with buzzer playback via the `party`
shell command. Remote control arrives over MQTT (`dmx/#`).

### Buzzer audio

Passive piezo buzzer driven by the LEDC PWM peripheral through Zephyr's
`pwm-buzzer` subsystem. Plays note-timed melodies from `songs.c`.

## Architecture

### Hardware

- **MCU**: ESP32-S3 (procpu).
- **DMX out**: UART1 → RS485 transceiver (TX = GPIO17 inverted, RX = GPIO18).
  TXD is inverted at the pin matrix to compensate for the A/B convention on the
  Waveshare transceiver vs. DMX XLR pin 2/3.
- **Buzzer**: LEDC channel 0 → GPIO46 (`pwm-buzzer`).
- **Ethernet**: W5500 over SPI (static IPv4 `192.168.2.133`, DHCP disabled).
- **I²C**: PCA9538 GPIO expander (relay outputs), PCF85063A RTC.
- **SD**: ESP32 SDHC (configuration, logs).

### Software modules

| Module | Purpose |
|---|---|
| `src/modules/output/dmx/` | DMX device registry + drivers: PicoSpot 45 moving head (`mh` shell), Cameo ROOT PAR 6 RGBW PAR (`par` shell). |
| `src/modules/output/sound/` | Buzzer playback thread, note tables (`songs.c`), startup chime, `sound` shell. |
| `src/modules/show/` | Time-keyframed light-show player (`shows.c`), `party` shell. |
| `src/modules/net/` | MQTT subscriber — connects to the broker and subscribes `dmx/#`. |
| `src/modules/config/` | SD-card configuration loader (`club-ctrl.json`, `creds.json`). |
| `src/modules/zbus/` | zbus channels that decouple networking, inputs and outputs. |
| `../dmx-z/` | Shared DMX512 driver (transmitter half, `dmx_z_tx.c`). |

### Moving-head DMX map (PicoSpot 45, 9-channel)

Head 0 starts at DMX address **1**, head 1 at address **10** (9 channels each,
no overlap). Channel offsets relative to the start address:

| Offset | Channel | Range |
|--------|---------|-------|
| 1 | Pan coarse | 0..255 → 0°..540° |
| 2 | Tilt coarse | 0..255 → 0°..200° |
| 3 | Pan fine | 16-bit LSB (held 0) |
| 4 | Tilt fine | 16-bit LSB (held 0) |
| 5 | Motor speed | 0 = fast .. 255 = slow |
| 6 | Color wheel | bands of 11 / 88..175 split / 176..255 rotate |
| 7 | Gobo | 0..124 select / 125..249 shake / 250..255 rotate |
| 8 | Dimmer | 0..255 = 0..100% |
| 9 | Shutter | 0..9 open / 10..255 strobe 1..25 Hz |

### RGBW PAR DMX map (Cameo ROOT PAR 6, 6-channel)

The PAR starts at DMX address **19** (after the two moving heads). Channel
offsets relative to the start address:

| Offset | Channel | Range |
|--------|---------|-------|
| 1 | Dimmer | 0..255 = 0..100% |
| 2 | Strobe | 0..5 open / 6..255 strobe slow → fast (<1Hz..20Hz) |
| 3 | Red | 0..255 |
| 4 | Green | 0..255 |
| 5 | Blue | 0..255 |
| 6 | White | 0..255 |

### Adding a new DMX device

Devices plug into a small registry (`src/modules/output/dmx/device.{h,c}`).
To add one:

1. Add an id to `enum dmx_device_id` in `device.h`.
2. Implement a module exposing a `struct dmx_device` (an `apply()` handler,
   a `get_addr()` accessor and a parameter name table) and register it from
   an `xxx_init()` function.
3. Call `xxx_init()` from `main()` and list the new files in
   `src/modules/output/dmx/CMakeLists.txt`.

The zbus listener and the MQTT topic parser (`dmx/<device>/<addr>/<param>`)
route through the registry, so no shared control code needs changing.

### Networking

- Ethernet (W5500) with static IPv4 `192.168.2.133/24`, gateway `192.168.2.1`
  (`prj.conf`, DHCP disabled). Wi-Fi is disabled (`CONFIG_WIFI=n`).
- MQTT subscriber: connects to the broker, subscribes `dmx/#`, retries every
  3 s until CONNACK.

## MQTT topics

All DMX control arrives via MQTT. club-ctrl subscribes `dmx/#` and routes:

| Topic | Action |
|-------|--------|
| `dmx/mh/<addr>/<param>` | Moving head at `<addr>` (1 = head 0, 10 = head 1) — `<param>` = `color`, `dimmer`, `strobe`, `pan`, `tilt`, `gobo`, `motor`, `home`, `blackout`, `addr` |
| `dmx/par/<addr>/<param>` | RGBW PAR at `<addr>` (default 19) — `<param>` = `dimmer`, `strobe`, `red`, `green`, `blue`, `white`, `home`, `blackout`, `addr` |
| `dmx/fixture/<addr>` | Raw DMX slot write (`<addr>` = channel 1..504) for the relay fixture |
| legacy flat topics (`dmx/color`, `dmx/dimmer`, …) | Mapped to moving head 0 (addr 1) |

Topic routing is generic: `dmx/<device>/<addr>/<param>` is resolved against
the device registry (`mh`, `par`), the device's current start addresses, and
its parameter table. Adding a device needs no MQTT changes.

## Startup & system dependencies

Boot order in `src/main.c`:

| # | Step | Depends on |
|---|------|-----------|
| 1 | USB-serial console (`usb_serial`) | — (hard requirement; aborts if absent) |
| 2 | Boot button (GPIO interrupt) + WS2812 LED strip + piezo buzzer | on-board peripherals |
| 3 | `config_init()` — reads `club-ctrl.json` / `creds.json` from SD | SD card (optional; falls back to compiled defaults) |
| 4 | `dmx_tx_init()` + register DMX devices + `mh_set_addr()` | RS485 bus (downstream: MH #1 → MH #2 → dmx-fixture) |
| 5 | `sound_init()` + startup chime | buzzer (LEDC PWM) |
| 6 | `show_init()` | — |
| 7 | `mqtt_subscriber_init()` — connects to broker, subscribes `dmx/#` | MQTT broker `192.168.2.128:1883` (anonymous) |
| 8 | Ethernet W5500 | network + MQTT broker reachability |
| 9 | PCF85063A RTC + PCA9538 relay expander | on-board I²C bus |

**Network identity**: static IPv4 `192.168.2.133` (`prj.conf`, DHCP disabled).

**Key external dependencies**:
- **MQTT broker** at `192.168.2.128:1883` (mosquitto, anonymous access). The
  `mqtt_sub` thread keeps retrying every 3 s until CONNACK.
- **DMX512 bus**: club-ctrl is the RS485 transmitter. Its UART1 TX drives the
  daisy-chain; the moving heads and the relay fixture are downstream receivers.

## Shell commands

```
mh pan <0..540>
mh tilt <0..200>
mh motor <0..255>         # 0 = fast, 255 = slow
mh color <name|0..255>    # name = white red orange yellow green blue cyan purple
mh dimmer <0..255>
mh gobo <0..255>
mh strobe <0..255>        # 0..9 open, 10..255 = 1..25 Hz
mh addr <1..504>          # set DMX start address (head 0)
mh home                   # recenter, white, full
mh blackout
mh show                   # print current state
mh flood                  # set slots 1..16 = 255

par dimmer <0..255>
par strobe <0..255>       # 0..5 open, 6..255 = strobe slow -> fast
par red <0..255>
par green <0..255>
par blue <0..255>
par white <0..255>
par addr <1..504>         # set DMX start address
par home                  # white, full
par blackout
par show                  # print current state

dmx raw <ch> <val>        # write a raw DMX slot

sound play <sandstorm|chime|rickroll|nggyu>
sound stop

party <sandstorm|song|rickroll|nggyu|blue|lamour|rhythm|dota>
party stop
```

Songs and light shows are defined in `src/modules/output/sound/songs.c` and
`src/modules/show/shows.c`.

## SD Card Configuration

At boot the firmware reads two optional JSON files from the FAT32 SD card
(`/SD:`); absent or unparseable files fall back to compiled defaults.

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

```bash
source ~/zephyrproject/.venv/bin/activate
```

### Fetch Espressif HAL (once)

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

> Note: club-ctrl references the shared `dmx-z` module as a sibling directory
> (`add_subdirectory(../dmx-z ...)`). Check out `dmx-z` next to `club-ctrl`.

### Flash

```bash
west flash --build-dir /tmp/b --esp-device /dev/ttyACM0
```

The serial monitor must be disconnected before flashing.

### Serial monitor

```bash
west espressif monitor -p /dev/ttyACM0
```

## License

SPDX-License-Identifier: Apache-2.0

## Copyright

Copyright (C) 2025 - 2026 Wolfenbütteler Kultur- und Veranstaltungsgesellschaft
gemeinnützige UG (haftungsbeschränkt)
