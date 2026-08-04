#pragma once

#include <Arduino.h>

namespace GDEM042F86 {

constexpr uint16_t kWidth = 400;
constexpr uint16_t kHeight = 300;
constexpr size_t kFrameBytes = (kWidth * kHeight) / 4;

void begin(int8_t sck, int8_t miso, int8_t mosi, int8_t cs, int8_t dc, int8_t rst, int8_t busy);
bool displayFrame(const uint8_t *frame, size_t length, bool fastUpdate = false);
bool clearWhite();

}  // namespace GDEM042F86
