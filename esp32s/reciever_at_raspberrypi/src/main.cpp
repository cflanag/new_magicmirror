/*
  receiver.cpp
  ESP32-C3 — ESP-NOW "many-to-one" receiver (standalone, no WiFi router needed)

  Role:
    - Receives sensor payloads from multiple ESP-NOW transmitters
      (BME280 unit and AM2301B unit)
    - Prints each reading to Serial as a single-line JSON object
    - Does NOT need router WiFi, does NOT need to know transmitter MACs
      in advance (ESP-NOW receive works for any sender on the same channel)

  THIS RECEIVER'S OWN MAC ADDRESS: AC:27:6E:7F:17:D8
    -> Each transmitter sketch must send to this address.
    -> Print it again at boot (below) so you can double check it hasn't
       changed (MAC can shift if you flash a different chip).

  Serial output format (one line per reading, 115200 baud):
    {"id":"bme280","temp_f":72.14,"humidity":41.2,"pressure":1013.2}
    {"id":"am2301b","temp_f":71.80,"humidity":39.5}

  IMPORTANT: The struct_message struct below MUST be byte-for-byte identical
  in receiver.cpp, transmitter_bme280.cpp, and transmitter_am2301b.cpp.
  Easiest way: after all three sketches are finalized, copy this struct
  into a shared header if you want to avoid drift. For now, keep the
  three copies in sync manually.

  Sensor identity travels as a numeric id (BOARD_ID) rather than a string,
  matching each transmitter's own #define BOARD_ID. sensorLabel() below
  maps id back to the string used in this receiver's JSON output.
  1111111
*/

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// Sensor identity by BOARD_ID (must match #define BOARD_ID in each transmitter .cpp)
#define BOARD_ID_AM2301B 1
#define BOARD_ID_BME280  2

// ---- Shared payload definition (must match transmitters exactly) ----
typedef struct struct_message {
    int id;
    float temperature;
    float humidity;
    float pressure;
} struct_message;

struct_message incomingData;

const char* sensorLabel(int id) {
  switch (id) {
    case BOARD_ID_AM2301B: return "am2301b";
    case BOARD_ID_BME280:  return "bme280";
    default:                return "unknown";
  }
}

// ---- Function prototype (required for a plain C++ translation unit) ----
void onDataRecv(const uint8_t *mac, const uint8_t *incomingBytes, int len);

// ---- ESP-NOW receive callback ----
// Signature matches ESP32 Arduino core 2.x. If you upgrade to core 3.x,
// see the fallback note at the bottom of this file.
void onDataRecv(const uint8_t *mac, const uint8_t *incomingBytes, int len) {
  (void)mac;

  if (len != sizeof(struct_message)) {
    Serial.print("{\"error\":\"bad_payload_size\",\"len\":");
    Serial.print(len);
    Serial.println("}");
    return;
  }

  memcpy(&incomingData, incomingBytes, sizeof(incomingData));

  // Build JSON line
  Serial.print("{\"id\":\"");
  Serial.print(sensorLabel(incomingData.id));
  Serial.print("\",\"temp_f\":");
  Serial.print(incomingData.temperature, 2);
  Serial.print(",\"humidity\":");
  Serial.print(incomingData.humidity, 1);

  if (incomingData.id == BOARD_ID_BME280) {
    Serial.print(",\"pressure\":");
    Serial.print(incomingData.pressure, 1);
  }

  Serial.println("}");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  // Disconnect from any AP; ESP-NOW only needs STA mode initialized, not connected.
  WiFi.disconnect();

  Serial.print("{\"status\":\"boot\",\"receiver_mac\":\"");
  Serial.print(WiFi.macAddress());
  Serial.println("\"}");

  if (esp_now_init() != ESP_OK) {
    Serial.println("{\"error\":\"esp_now_init_failed\"}");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("{\"status\":\"ready\"}");
}

void loop() {
  // Nothing to do here — all work happens in onDataRecv().
  delay(10);
}

/*
  ---- Fallback for ESP32 Arduino core 3.x ----
  If you upgrade to core 3.x, esp_now_register_recv_cb expects this
  callback signature instead:

    void onDataRecv(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingBytes, int len) {
      ... same body ...
    }
*/
