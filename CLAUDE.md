# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository overview

This repo is a home MagicMirror² display plus a small wireless outdoor/indoor
weather-sensor network built on ESP32-C3 boards. There is no root build
system tying the pieces together — each of the four subdirectories below is
independent, with its own toolchain.

```
esp32s/
  transmitter_bme280/       PlatformIO project — ESP32-C3, BME280 sensor
  transmitter_am2301b/      PlatformIO project — ESP32-C3, AM2301B/DHT20 sensor
  reciever_at_raspberrypi/  PlatformIO project — ESP32-C3, ESP-NOW receiver (stub, see below)
magicmirror/                Config overrides for an external MagicMirror² install
```

## Data flow architecture

1. **Two battery-powered ESP32-C3 transmitters** (`transmitter_bme280`,
   `transmitter_am2301b`) each wake from deep sleep on a DS3231 RTC-scheduled
   timer, take one sensor reading, and push it over **ESP-NOW** (not Wi-Fi/IP)
   to a fixed receiver MAC address, then go back to deep sleep. Both boards
   share the exact same `struct_message { int id; float temperature,
   humidity, pressure; }` wire struct — **any change to this struct must be
   applied identically in both transmitter files (and the receiver) or the
   link breaks silently**, since ESP-NOW has no schema negotiation.
2. A third ESP32-C3, wired to a Raspberry Pi over USB serial, receives those
   ESP-NOW packets and is expected to print one JSON line per reading to
   serial, e.g. `{"id":"bme280","temp_f":72.14,"humidity":41.2,"pressure":1013.2}`,
   plus status/error JSON lines (`{"status":"boot",...}`, `{"error":"..."}`).
   **`esp32s/reciever_at_raspberrypi/src/main.cpp` is currently unmodified
   PlatformIO stub code** — the receiver firmware implementing this JSON
   protocol has not been written yet. When implementing it, mirror the
   `struct_message` layout from the transmitters exactly and match the JSON
   shape documented in `magicmirror/serial_to_mqtt.py`'s docstring.
3. `magicmirror/serial_to_mqtt.py` runs on the Raspberry Pi, reads those JSON
   lines off `/dev/ttyUSB0`, and republishes each field to MQTT topics
   (`sensors/<id>/<field>`, e.g. `sensors/bme280/temperature`).
4. The MagicMirror² module `MMM-MQTT` (configured in `magicmirror/config.js`)
   subscribes to those exact topic strings and renders the values on the
   mirror. **Topic strings, the `id` field transmitters/receiver use, and the
   MMM-MQTT `subscriptions` list in `config.js` must all stay in sync.**

The `magicmirror/` directory is *not* a full MagicMirror² checkout — it only
holds the override files (`config.js`, `custom.css`, `main.css`,
`serial_to_mqtt.py`) that get copied/symlinked into a separately-installed
MagicMirror² instance on the Pi. There's no `package.json` here and no way to
run the mirror UI from this repo alone.

## Per-transmitter power/scheduling design

Both transmitters follow the same pattern (see the header comments in each
`.cpp` for full rationale):
- Read the DS3231 RTC on wake.
- Active window 6:00 AM–11:00 PM: take one reading, ESP-NOW send it, deep
  sleep 10 minutes (`ACTIVE_INTERVAL_SECONDS`), repeat.
- Quiet window 11:00 PM–6:00 AM: skip sensing entirely, deep sleep straight
  through until 6:00 AM in one shot.
- Deep sleep is software-only (`esp_deep_sleep_start`); the DS3231 and sensor
  are wired directly to the battery rail, not GPIO-power-switched.
- `BOARD_ID` (`1` for AM2301B, `2` for BME280) must stay unique per
  transmitter and match whatever the receiver uses to distinguish senders.
- Each transmitter hardcodes the receiver's MAC address in `receiverAddress[]`
  — update it if the receiver board is ever re-flashed/replaced.

## Build and flash commands (PlatformIO)

Each `esp32s/*` directory is its own PlatformIO project; run these from
inside the relevant directory (or with `-d <path>`):

```bash
pio run                              # build the default env (main firmware)
pio run -t upload                    # build and flash the main firmware
pio device monitor -b 115200         # watch JSON serial output
```

Both transmitter projects additionally define extra environments in their
`platformio.ini`, gated out of the default build via `build_src_filter` so
only one `.cpp` is ever compiled per env:

```bash
pio run -e set_clock -t upload       # one-time: set that board's DS3231 clock, then re-upload main env
pio run -e i2c_scan -t upload        # diagnostic: print all I2C addresses found on the bus
```

`set_clock` and `i2c_scan` are utilities, not production firmware — never
leave a board running them; always switch back to the default
`seeed_xiao_esp32c3` env afterward.

### Flashing gotcha (Seeed XIAO ESP32-C3)

These boards have no separate USB-serial chip and spend almost all their time
in deep sleep, so the bootloader is very hard to catch. To flash: hold BOOT,
click RESET, release BOOT — the `/dev/ttyACM*` port then appears briefly for
upload. (See `esp32s/transmitter_am2301b/Important you cant connect to the
xaio .md`.)

To force a DS3231 clock resync via `set_clock`, you must change something in
`set_clock.cpp` (even a comment) so PlatformIO actually recompiles — the
`__DATE__`/`__TIME__` macros it relies on are baked in at compile time.

## Python bridge (`magicmirror/serial_to_mqtt.py`)

```bash
pip3 install pyserial paho-mqtt
python3 serial_to_mqtt.py
```

Reads newline-delimited JSON off `SERIAL_PORT` (`/dev/ttyUSB0` by default —
confirm with `ls /dev/ttyUSB*` or `/dev/ttyACM*`) and publishes to MQTT with
`retain=True`. Lines containing `"status"` or `"error"` are logged, not
published. A commented-out systemd unit template is included at the bottom
of the file for running it as a boot-time service on the Pi.
