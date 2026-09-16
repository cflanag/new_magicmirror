#!/usr/bin/env python3
"""
serial_to_mqtt.py

Reads JSON sensor lines from the ESP32-C3 ESP-NOW receiver (connected via
USB serial) and publishes each value to MQTT topics that MMM-MQTT can
subscribe to.

Expected input from the receiver's serial output (one JSON object per line):
    {"id":"bme280","temp_f":72.14,"humidity":41.2,"pressure":1013.2}
    {"id":"am2301b","temp_f":71.80,"humidity":39.5}
Status/error lines from the receiver (e.g. {"status":"ready"}) are logged
and ignored -- they don't get published to MQTT.

Published MQTT topics (adjust TOPIC_PREFIX below if you want a different
scheme, and match these exact strings in your MagicMirror config.js
`subscriptions` list):
    sensors/bme280/temperature      (Fahrenheit, plain number payload)
    sensors/bme280/humidity         (% RH)
    sensors/bme280/pressure         (hPa)
    sensors/am2301b/temperature     (Fahrenheit)
    sensors/am2301b/humidity        (% RH)

Example MMM-MQTT config.js subscription entry to match:
    {
      topic: 'sensors/bme280/temperature',
      label: 'Indoor Temp',
      suffix: '°F',
      decimals: 1,
      maxAgeSeconds: 120
    }

Requirements:
    pip3 install pyserial paho-mqtt

Run manually to test:
    python3 serial_to_mqtt.py

Run as a systemd service (optional, for auto-start on boot):
    See the commented unit file template at the bottom of this script.
"""

import json
import logging
import time

import serial
import paho.mqtt.client as mqtt

# ---- Configuration ----
SERIAL_PORT = "/dev/ttyUSB0"   # Confirm with: ls /dev/ttyUSB* or /dev/ttyACM*
BAUD_RATE = 115200

MQTT_BROKER = "localhost"
MQTT_PORT = 1883
MQTT_USER = None                # Set to a string if your broker requires auth
MQTT_PASSWORD = None
TOPIC_PREFIX = "sensors"        # Topics become: sensors/<id>/<field>

RECONNECT_DELAY_SECONDS = 5

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
)
log = logging.getLogger("serial_to_mqtt")


def build_mqtt_client():
    client = mqtt.Client()
    if MQTT_USER:
        client.username_pw_set(MQTT_USER, MQTT_PASSWORD)

    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            log.info("Connected to MQTT broker %s:%s", MQTT_BROKER, MQTT_PORT)
        else:
            log.error("MQTT connect failed, return code %s", rc)

    def on_disconnect(client, userdata, rc):
        log.warning("Disconnected from MQTT broker (rc=%s)", rc)

    client.on_connect = on_connect
    client.on_disconnect = on_disconnect

    client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
    client.loop_start()  # runs MQTT network loop in a background thread,
                          # handles automatic reconnects
    return client


def publish_reading(client, reading):
    """
    reading: dict parsed from one JSON line, e.g.
        {"id": "bme280", "temp_f": 72.14, "humidity": 41.2, "pressure": 1013.2}
    """
    sensor_id = reading.get("id")
    if not sensor_id:
        return  # not a sensor reading line (e.g. a status/error line)

    field_map = {
        "temp_f": "temperature",
        "humidity": "humidity",
        "pressure": "pressure",
    }

    for json_key, topic_field in field_map.items():
        if json_key in reading:
            topic = f"{TOPIC_PREFIX}/{sensor_id}/{topic_field}"
            payload = str(reading[json_key])
            result = client.publish(topic, payload, retain=True)
            if result.rc != mqtt.MQTT_ERR_SUCCESS:
                log.warning("Publish failed for %s: rc=%s", topic, result.rc)
            else:
                log.debug("Published %s = %s", topic, payload)


def open_serial():
    while True:
        try:
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            log.info("Opened serial port %s at %d baud", SERIAL_PORT, BAUD_RATE)
            return ser
        except serial.SerialException as e:
            log.error("Could not open serial port %s: %s", SERIAL_PORT, e)
            log.info("Retrying in %d seconds...", RECONNECT_DELAY_SECONDS)
            time.sleep(RECONNECT_DELAY_SECONDS)


def main():
    mqtt_client = build_mqtt_client()
    ser = open_serial()

    log.info("Listening for sensor readings...")

    while True:
        try:
            raw_line = ser.readline()
            if not raw_line:
                continue  # timeout with no data, loop again

            line = raw_line.decode("utf-8", errors="replace").strip()
            if not line:
                continue

            try:
                reading = json.loads(line)
            except json.JSONDecodeError:
                log.warning("Skipping unparseable line: %r", line)
                continue

            if "error" in reading:
                log.warning("Receiver reported error: %s", reading)
                continue

            if "status" in reading:
                log.info("Receiver status: %s", reading)
                continue

            publish_reading(mqtt_client, reading)
            log.info("Reading: %s", reading)

        except serial.SerialException as e:
            log.error("Serial error: %s -- reconnecting...", e)
            try:
                ser.close()
            except Exception:
                pass
            ser = open_serial()

        except KeyboardInterrupt:
            log.info("Shutting down.")
            break

    mqtt_client.loop_stop()
    mqtt_client.disconnect()
    ser.close()


if __name__ == "__main__":
    main()

"""
---- Optional: run as a systemd service ----

Create /etc/systemd/system/serial-to-mqtt.service with:

[Unit]
Description=ESP32 sensor serial-to-MQTT bridge
After=network.target mosquitto.service

[Service]
Type=simple
ExecStart=/usr/bin/python3 /home/pi/serial_to_mqtt.py
Restart=always
RestartSec=5
User=pi

[Install]
WantedBy=multi-user.target

Then enable and start it:
    sudo systemctl daemon-reload
    sudo systemctl enable serial-to-mqtt.service
    sudo systemctl start serial-to-mqtt.service
    sudo systemctl status serial-to-mqtt.service
    journalctl -u serial-to-mqtt.service -f     # to watch live logs
"""