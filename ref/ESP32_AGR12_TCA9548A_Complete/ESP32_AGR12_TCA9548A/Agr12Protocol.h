#pragma once
#include <stddef.h>
#include <stdint.h>

namespace agr12 {
// ASAIR AGR12 I2C datasheet, section 10.2 (May 2024):
// DATA0=MSB, DATA1=LSB; CHECK=DATA0 XOR DATA1.
// Signed 16-bit two's complement value is pressure in units of 0.1 kPa.
inline bool decode(const uint8_t *frame, size_t length, float &pressureKpa) {
  if (frame == nullptr || length != 3) return false;
  if (static_cast<uint8_t>(frame[0] ^ frame[1]) != frame[2]) return false;
  const uint16_t raw = static_cast<uint16_t>(
      (static_cast<uint16_t>(frame[0]) << 8) | frame[1]);
  const int32_t signedRaw = (raw & 0x8000U) != 0
      ? static_cast<int32_t>(raw) - 65536L : static_cast<int32_t>(raw);
  pressureKpa = static_cast<float>(signedRaw) / 10.0f;
  return true;
}
} // namespace agr12
