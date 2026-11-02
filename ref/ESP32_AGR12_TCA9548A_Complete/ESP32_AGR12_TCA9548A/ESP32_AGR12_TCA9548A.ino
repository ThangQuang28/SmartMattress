/*
 * ESP32 -> TCA9548A -> AGR12 I2C (ASAIR protocol, May 2024).
 * Arduino-ESP32; only the bundled Wire library is required.
 * Open this file from its ESP32_AGR12_TCA9548A folder.
 * Default wiring: ESP32-WROOM SDA=21, SCL=22. Edit for your board.
 * AGR12: 5V supply. TCA9548A: 3.3V supply; upstream pull-ups to 3.3V.
 */
#include <Arduino.h>
#include <Wire.h>
#include "Agr12Protocol.h"

#if !defined(ARDUINO_ARCH_ESP32)
#error "Select an ESP32 board in Arduino IDE."
#endif

// ---------------- USER CONFIGURATION ----------------
constexpr int I2C_SDA_PIN = 21;
constexpr int I2C_SCL_PIN = 22;
constexpr uint8_t TCA_ADDRESS = 0x70;     // A0=A1=A2=GND
constexpr uint8_t AGR12_ADDRESS = 0x50;   // 7-bit; datasheet 0xA0/0xA1
constexpr uint8_t ENABLED_CHANNELS = 0xFF; // All 8; CH0 only: 0x01
constexpr int TCA_RESET_PIN = -1;         // Optional GPIO to /RESET; -1=unused
constexpr uint32_t SAMPLE_PERIOD_MS = 1000;
constexpr uint32_t REDETECT_PERIOD_MS = 10000;
constexpr uint32_t CONVERSION_WAIT_MS = 85; // Datasheet requires >=80 ms
constexpr uint8_t READ_ATTEMPTS = 2;
static_assert(CONVERSION_WAIT_MS >= 80, "AGR12 requires at least 80 ms");
static_assert(READ_ATTEMPTS >= 1, "At least one read attempt is required");
// No automatic zero or filtering: output retains measured pressure.
// ----------------------------------------------------

static uint8_t onlineChannels = 0;
static bool busReady = false;
static uint32_t lastSampleMs = 0;
static uint32_t lastDetectMs = 0;

static bool writeMux(uint8_t mask) {
  Wire.beginTransmission(TCA_ADDRESS);
  Wire.write(mask);
  return Wire.endTransmission(true) == 0;
}

static bool selectChannel(uint8_t channel) {
  return channel < 8 && writeMux(static_cast<uint8_t>(1U << channel));
}

static bool pingAddress(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission(true) == 0;
}

static void pulseMuxReset() {
  if (TCA_RESET_PIN < 0) return;
  // Open-drain: the external pull-up is to TCA VCC (3.3V).
  pinMode(TCA_RESET_PIN, OUTPUT_OPEN_DRAIN);
  digitalWrite(TCA_RESET_PIN, LOW);
  delay(1);
  digitalWrite(TCA_RESET_PIN, HIGH);
  delay(2);
}

static bool initializeBus() {
  pulseMuxReset();
  const bool ok = Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 100000);
  if (ok) Wire.setTimeOut(50);
  return ok;
}

static void detectSensors() {
  onlineChannels = 0;
  if (!writeMux(0)) {
    pulseMuxReset();
    if (!writeMux(0)) {
      Serial.println("# MUX_ERROR: check 0x70 address, power, SDA/SCL, /RESET");
      return;
    }
  }
  for (uint8_t channel = 0; channel < 8; ++channel) {
    if ((ENABLED_CHANNELS & (1U << channel)) == 0) continue;
    if (!selectChannel(channel)) {
      Serial.printf("# CH%u MUX_ERROR\n", channel);
      pulseMuxReset();
      continue;
    }
    if (pingAddress(AGR12_ADDRESS)) {
      onlineChannels |= static_cast<uint8_t>(1U << channel);
      Serial.printf("# CH%u: device ACK at 0x%02X\n", channel, AGR12_ADDRESS);
    }
    if (!writeMux(0)) {
      Serial.printf("# CH%u: could not isolate bus; /RESET may be needed\n", channel);
      pulseMuxReset();
    }
  }
  Serial.printf("# Detected mask=0x%02X (ACK is not a sensor identity check)\n", onlineChannels);
  if (onlineChannels == 0) Serial.println("# No sensor. Send s for I2C scan, r to retry.");
}

static void scanChannels() {
  Serial.println("# I2C address scan (7-bit):");
  if (!writeMux(0)) {
    Serial.println("# Cannot deselect TCA channels. Check wiring or /RESET.");
    pulseMuxReset();
    return;
  }
  for (uint8_t channel = 0; channel < 8; ++channel) {
    if ((ENABLED_CHANNELS & (1U << channel)) == 0) continue;
    if (!selectChannel(channel)) {
      Serial.printf("# CH%u MUX_ERROR\n", channel);
      pulseMuxReset();
      continue;
    }
    Serial.printf("# CH%u:", channel);
    for (uint8_t address = 0x08; address <= 0x77; ++address) {
      // TCA itself is on the upstream bus, so do not list it as a sensor.
      if (address != TCA_ADDRESS && pingAddress(address)) {
        Serial.printf(" 0x%02X", address);
      }
    }
    Serial.println();
    if (!writeMux(0)) pulseMuxReset();
    delay(1);
  }
}

static bool readPressure(uint8_t channel, float &pressureKpa,
                         uint8_t *frame, const char *&status) {
  for (uint8_t attempt = 0; attempt < READ_ATTEMPTS; ++attempt) {
    frame[0] = frame[1] = frame[2] = 0;
    if (!selectChannel(channel)) {
      status = "MUX_ERROR";
      pulseMuxReset();
      continue;
    }

    // A STOP must finish the TCA write before addressing the sensor.
    Wire.beginTransmission(AGR12_ADDRESS);
    Wire.write(static_cast<uint8_t>(0xAC));
    Wire.write(static_cast<uint8_t>(0x12));
    const uint8_t error = Wire.endTransmission(true);
    if (error != 0) {
      status = "COMMAND_ERROR";
      if (!writeMux(0)) pulseMuxReset();
      delay(5);
      continue;
    }

    delay(CONVERSION_WAIT_MS);
    const size_t count = Wire.requestFrom(AGR12_ADDRESS, static_cast<size_t>(3), true);
    const int available = Wire.available();
    const bool fullFrame = count == 3 && available == 3;
    if (fullFrame) {
      for (uint8_t i = 0; i < 3; ++i) frame[i] = static_cast<uint8_t>(Wire.read());
    }
    while (Wire.available() > 0) Wire.read();

    const bool isolated = writeMux(0);
    if (!isolated) {
      status = "ISOLATE_ERROR";
      pulseMuxReset();
      continue;
    }
    if (!fullFrame) {
      status = "SHORT_READ";
      continue;
    }
    if (!agr12::decode(frame, 3, pressureKpa)) {
      status = "CHECKSUM_ERROR";
      continue;
    }
    status = "OK";
    return true;
  }
  return false;
}

static void printHelp() {
  Serial.println("# ESP32 + TCA9548A + AGR12 (ASAIR I2C, May 2024)");
  Serial.printf("# SDA=%d SCL=%d I2C=100000 Hz TCA=0x%02X AGR12=0x%02X\n",
                I2C_SDA_PIN, I2C_SCL_PIN, TCA_ADDRESS, AGR12_ADDRESS);
  Serial.println("# Commands: s=scan, r=redetect, h=help. Ignore CR/LF.");
  Serial.println("# Pressure is unfiltered, no zero offset, gauge pressure.");
  Serial.println("# Errors leave pressure fields empty; never reuse an old reading.");
  Serial.println("time_ms,channel,pressure_kPa,pressure_bar,status,raw_hex");
}

void setup() {
  Serial.begin(115200);
  delay(500);
  printHelp();
  busReady = initializeBus();
  if (busReady) detectSensors();
  else Serial.println("# I2C_INIT_ERROR: check GPIO configuration");
  lastDetectMs = millis();
  lastSampleMs = millis() - SAMPLE_PERIOD_MS;
}

void loop() {
  if (!busReady) {
    delay(1000);
    busReady = initializeBus();
    if (busReady) {
      detectSensors();
      lastDetectMs = millis();
    }
    return;
  }

  while (Serial.available() > 0) {
    const char command = static_cast<char>(Serial.read());
    if (command == 's' || command == 'S') scanChannels();
    else if (command == 'r' || command == 'R') {
      detectSensors();
      lastDetectMs = millis();
    } else if (command == 'h' || command == 'H') printHelp();
  }

  if (static_cast<uint32_t>(millis() - lastDetectMs) >= REDETECT_PERIOD_MS) {
    detectSensors();
    lastDetectMs = millis();
  }
  if (static_cast<uint32_t>(millis() - lastSampleMs) < SAMPLE_PERIOD_MS) {
    delay(1);
    return;
  }
  lastSampleMs = millis();
  for (uint8_t channel = 0; channel < 8; ++channel) {
    if ((onlineChannels & (1U << channel)) == 0) continue;
    float pressureKpa = 0.0f;
    uint8_t frame[3] = {};
    const char *status = "UNKNOWN_ERROR";
    const bool ok = readPressure(channel, pressureKpa, frame, status);
    Serial.printf("%lu,%u,", static_cast<unsigned long>(millis()), channel);
    if (ok) Serial.printf("%.1f,%.4f,", pressureKpa, pressureKpa / 100.0f);
    else Serial.print(",,");
    Serial.printf("%s,%02X %02X %02X\n", status, frame[0], frame[1], frame[2]);
  }
}
