/*
  i2c_scan.cpp
  Diagnostic utility — scans the I2C bus and prints every address that acks.

  Usage:
    pio run -e i2c_scan -t upload
  Expected on this board: 0x38 (DHT20) and 0x68 (DS3231). Anything missing
  or unexpected points at a wiring problem (SDA/SCL swapped, loose
  connection, missing pull-ups, wrong power pin) rather than a firmware bug.
*/

#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin();
  Serial.println("{\"status\":\"scanning\"}");
}

void loop() {
  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.print("{\"found_address\":\"0x");
      if (addr < 16) Serial.print("0");
      Serial.print(addr, HEX);
      Serial.println("\"}");
      found++;
    }
  }
  if (found == 0) {
    Serial.println("{\"status\":\"no_devices_found\"}");
  } else {
    Serial.print("{\"status\":\"scan_done\",\"count\":");
    Serial.print(found);
    Serial.println("}");
  }
  delay(3000);
}
