/*
  set_clock.cpp
  One-time utility — sets this board's DS3231 RTC clock to this build's
  compile time.

  This board has its own physically separate DS3231 from the BME280
  transmitter unit — setting that one's clock does NOT set this one's. Run
  this utility once per board, on this board, before deploying
  transmitter_am2301b.cpp.

  Usage:
    1. Build and upload ONLY this file via the `set_clock` PlatformIO
       environment:
         pio run -e set_clock -t upload
    2. Open the serial monitor (115200 baud) and confirm you see
       {"status":"clock_set"} followed by the time ticking once per second.
    3. Switch back to uploading the main transmitter firmware:
         pio run -e seeed_xiao_esp32c3 -t upload
       Do NOT leave this sketch running as production firmware — it never
       transmits sensor data and never sleeps.

  Wiring (shared I2C bus with the DHT20):
    DS3231 VCC -> 3.3V / battery rail (not GPIO-switched — see
      transmitter_am2301b.cpp header comment for the power-design rationale)
    DS3231 GND -> GND
    DS3231 SDA -> GPIO 8 (shared with DHT20 SDA)
    DS3231 SCL -> GPIO 9 (shared with DHT20 SCL)
    DS3231 default I2C address: 0x68

  Library required (declared in platformio.ini lib_deps): Adafruit RTClib.
*/

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;
//reset the clock to the time this sketch was compiled. This is a one-time utility,
 // run once per board before deploying the main firmware. change this line to recompile and reset the clock
 //to a new time.111

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin();

  if (!rtc.begin()) {
    Serial.println("{\"error\":\"ds3231_not_found\"}");
    while (true) delay(1000);
  }

  // Sets the RTC to the date/time this sketch was compiled.
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  Serial.println("{\"status\":\"clock_set\"}");
}

void loop() {
  DateTime now = rtc.now();
  Serial.printf("{\"time\":\"%04d-%02d-%02d %02d:%02d:%02d\"}\n",
    now.year(), now.month(), now.day(),
    now.hour(), now.minute(), now.second());
  delay(1000);
}
