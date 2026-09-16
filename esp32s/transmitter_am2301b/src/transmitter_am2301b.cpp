/*
  transmitter_am2301b.cpp
  ESP32-C3 — ESP-NOW transmitter for an AM2301B / DHT20 sensor (temp/humidity)
  with a DS3231 RTC-scheduled deep-sleep duty cycle for long battery life.

  Role:
    - Wakes from deep sleep, reads the DS3231 RTC to find the current time.
    - Between 6:00 AM and 11:00 PM: takes one DHT20 reading (temp/humidity),
      sends it via ESP-NOW to the receiver, then deep sleeps for 10 minutes
      and repeats.
    - From 11:00 PM to 6:00 AM: skips sensing/transmitting entirely and goes
      into a single deep sleep lasting until 6:00 AM, then resumes the
      10-minute cycle. This repeats every day, indefinitely.
    - Uses the DFRobot_DHT20 library. Unlike the BME280, the AHT20/DHT20
      sensor chip has no continuous "Normal mode" to disable in the first
      place: every getTemperature()/getHumidity() call already sends its own
      one-shot trigger-measurement command (0xAC 0x33 0x00), waits for the
      busy bit to clear, then reads the result and returns to idle on its
      own — confirmed by reading DFRobot_DHT20.cpp's implementation. So
      there is no forced-mode equivalent to configure here; the library is
      already single-shot by design.

  Power design (battery: 5000 mAh LiPo on the board's back BAT pads, target
  >1 year runtime — same target as the companion transmitter_bme280 unit):
    This design is software-only deep sleep (esp_deep_sleep_start) — there is
    no MOSFET/hardware power-cutoff circuit. Both the DS3231 and the DHT20
    stay wired directly to the battery/3.3V rail instead of being GPIO-
    switched. This was a deliberate choice for reliability and simplicity:
      - The DS3231 draws only ~1-3 uA even when active, so leaving it
        powered continuously costs negligible current. Its coin-cell backup
        exists purely as a fail-safe against total main-battery loss, not as
        something this firmware duty-cycles.
      - The DHT20/AM2301B is already a low-current on-demand sensor: it
        idles at roughly 0.3-1 uA between measurements and only draws
        current (a few hundred uA) for the ~80ms a measurement takes, so
        there is no added benefit to GPIO-switching its power for this duty
        cycle.
      - ESP32 deep sleep itself draws roughly 10-40 uA. Combined with ~102
        brief ESP-NOW transmit bursts/day, a 5000 mAh LiPo comfortably clears
        1000+ days of runtime.

  Wiring (this is a separate physical ESP32-C3 board from the BME280 unit —
  it has its own DS3231 module wired to its own I2C bus; the two boards do
  not and cannot share an RTC):
    DHT20 VCC   -> 3.3V / battery rail
    DHT20 GND   -> GND
    DHT20 SDA   -> GPIO 8   (check your specific ESP32-C3 dev board silkscreen)
    DHT20 SCL   -> GPIO 9
    DHT20 default I2C address: 0x38 (DFRobot_DHT20 library default)

    DS3231 VCC  -> 3.3V / battery rail (not GPIO-switched, see Power design
      above)
    DS3231 GND  -> GND
    DS3231 SDA  -> GPIO 8   (shared bus with the DHT20)
    DS3231 SCL  -> GPIO 9   (shared bus with the DHT20)
    DS3231 default I2C address: 0x68

  Setting the clock:
    This board's DS3231 must be set once before deploying this firmware —
    it is a physically separate RTC from the BME280 unit's and cannot be
    pre-set by setting that one. Build and upload set_clock.cpp via the
    `set_clock` PlatformIO environment first (see that file's header
    comment), then come back and upload this environment
    (`seeed_xiao_esp32c3`).

  Libraries required (declared in platformio.ini lib_deps):
    - "DFRobot_DHT20" (provides DFRobot_DHT20.h)
    - "Adafruit RTClib" (provides RTClib.h)

  IMPORTANT: The struct_message struct below MUST be byte-for-byte identical
  to the one in transmitter_bme280.cpp and receiver.cpp. This sensor has no
  pressure output, so the pressure field is always sent as 0.
*/

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <DFRobot_DHT20.h>
#include <RTClib.h>

// Set your Board ID (ESP32 Sender #1 = BOARD_ID 1, ESP32 Sender #2 = BOARD_ID 2, etc)
#define BOARD_ID 1

// Active window: 10-minute duty cycle from 6:00 AM up to (not including) 11:00 PM.
// Outside that window, sleep straight through until 6:00 AM instead.
#define QUIET_START_HOUR 23
#define QUIET_END_HOUR 6
#define ACTIVE_INTERVAL_SECONDS 600UL

DFRobot_DHT20 dht20;
RTC_DS3231 rtc;

//MAC Address of the receiver
uint8_t receiverAddress[] = {0xAC, 0x27, 0x6E, 0x7F, 0x17, 0xD8};

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    int id;
    float temperature;
    float humidity;
    float pressure;
} struct_message;

esp_now_peer_info_t peerInfo;

//Create a struct_message called myData
struct_message myData;

bool dhtOk = false;
bool rtcOk = false;

// Function prototypes (required for a plain C++ translation unit)
float readDHTTemperature();
float readDHTHumidity();
bool takeDHTReading(float &t, float &h);
long secondsUntilSixAM(const DateTime &now);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void goToSleep(unsigned long sleepSeconds);

float readDHTTemperature() {
  float c = dht20.getTemperature();
  float t = c * 9.0 / 5.0 + 32.0;
  return roundf(t * 10.0) / 10.0;
}

float readDHTHumidity() {
  return dht20.getHumidity() * 100.0F;
}

// The DHT20 has no forced/continuous mode switch to set (see header comment)
// — every getTemperature()/getHumidity() call already performs one on-demand
// trigger-measure-read cycle. This helper just wraps both calls with a NaN
// check so setup() has a single success/failure result to act on.
bool takeDHTReading(float &t, float &h) {
  t = readDHTTemperature();
  h = readDHTHumidity();
  if (isnan(t) || isnan(h)) return false;
  return true;
}

// Seconds from `now` until the next 6:00 AM. Handles both the pre-11PM case
// (target is 6:00 AM tomorrow) and the post-midnight-before-6AM case
// (target is 6:00 AM today) with one formula.
long secondsUntilSixAM(const DateTime &now) {
  DateTime target(now.year(), now.month(), now.day(), QUIET_END_HOUR, 0, 0);
  if (now.hour() >= QUIET_END_HOUR) {
    target = target + TimeSpan(1, 0, 0, 0); // roll to tomorrow 6am
  }
  return target.unixtime() - now.unixtime();
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void goToSleep(unsigned long sleepSeconds) {
  Serial.print("{\"status\":\"sleeping\",\"seconds\":");
  Serial.print(sleepSeconds);
  Serial.println("}");
  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)sleepSeconds * 1000000ULL);
  esp_deep_sleep_start();
}

void setup() {
  //Init Serial Monitor
  Serial.begin(115200);
  delay(1000);

  Wire.begin(); // uses default SDA/SCL for your board; pass (sda, scl) if needed

  rtcOk = rtc.begin();
  if (!rtcOk) {
    Serial.println("{\"error\":\"ds3231_not_found\"}");
  }

  dhtOk = (dht20.begin() == 0);
  if (!dhtOk) {
    Serial.println("{\"error\":\"dht20_not_found\"}");
  }

  unsigned long sleepSeconds = ACTIVE_INTERVAL_SECONDS;
  bool shouldTransmit = dhtOk;

  if (rtcOk) {
    DateTime now = rtc.now();
    Serial.print("{\"status\":\"boot\",\"time\":\"");
    Serial.print(now.timestamp(DateTime::TIMESTAMP_FULL));
    Serial.println("\"}");

    if (now.hour() >= QUIET_START_HOUR || now.hour() < QUIET_END_HOUR) {
      sleepSeconds = secondsUntilSixAM(now);
      shouldTransmit = false;
    }
  }

  if (!shouldTransmit) {
    goToSleep(sleepSeconds);
    return; // unreachable — esp_deep_sleep_start() never returns
  }

  //Set values to send
  float temperature = 0, humidity = 0;
  if (!takeDHTReading(temperature, humidity)) {
    Serial.println("{\"error\":\"dht20_read_failed\"}");
    goToSleep(sleepSeconds);
    return;
  }

  // Set device as a Wi-Fi Station and set channel
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.print("{\"status\":\"boot\",\"my_mac\":\"");
  Serial.print(WiFi.macAddress());
  Serial.println("\"}");

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    goToSleep(sleepSeconds);
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Transmitted packet
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

  // Register peer
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    goToSleep(sleepSeconds);
    return;
  }

  myData.id = BOARD_ID;
  myData.temperature = temperature;
  myData.humidity = humidity;
  myData.pressure = 0; // no pressure sensor on this board; keep field defined for wire compatibility

  //Send message via ESP-NOW
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *) &myData, sizeof(myData));

  // Local debug echo so you can watch this transmitter's own serial
  // monitor too, independent of the receiver's output.
  Serial.print("{\"local_debug\":true,\"id\":");
  Serial.print(myData.id);
  Serial.print(",\"temperature\":");
  Serial.print(myData.temperature, 2);
  Serial.print(",\"humidity\":");
  Serial.print(myData.humidity, 1);
  Serial.print(",\"pressure\":");
  Serial.print(myData.pressure, 1);
  Serial.print(",\"esp_now_send_result\":");
  Serial.print(result == ESP_OK ? "\"queued_ok\"" : "\"queue_failed\"");
  Serial.println("}");

  // Give the send callback / serial output time to flush before sleeping.
  delay(200);

  goToSleep(sleepSeconds);
}

void loop() {
  // Unreachable: esp_deep_sleep_start() in setup() resets the chip each
  // cycle, so control never returns here in normal operation.
}
