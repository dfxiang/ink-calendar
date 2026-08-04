#pragma once

#include <stddef.h>
#include <stdint.h>

#include "font.h"

// App frame color codes (also consumed by gdem042f86.cpp):
// 0 = white, 1 = black, 2 = red, 3 = yellow.
enum FrameColor {
  kColorWhite = 0,
  kColorBlack = 1,
  kColorRed = 2,
  kColorYellow = 3,
};

class FrameBuffer {
 public:
  static constexpr int kWidth = 400;
  static constexpr int kHeight = 300;
  static constexpr size_t kBytes = (kWidth * kHeight) / 4;

  FrameBuffer() = default;

  void clear(uint8_t color = kColorWhite);

  void putPixel(int x, int y, uint8_t color);
  uint8_t getPixel(int x, int y) const;

  void fillRect(int x, int y, int w, int h, uint8_t color);
  void drawRect(int x, int y, int w, int h, uint8_t color, int thickness = 1);
  void drawHLine(int x, int y, int len, uint8_t color);
  void drawVLine(int x, int y, int len, uint8_t color);

  // Draws UTF-8 text with a bitmap font. Returns the drawn width in pixels.
  int drawText(int x, int y, const char *text, uint8_t color, const FontInfo &font);
  int textWidth(const char *text, const FontInfo &font) const;

  const uint8_t *data() const { return buffer_; }

 private:
  void drawGlyph(int x, int y, const uint8_t *bits, uint8_t size, uint8_t color);

  uint8_t buffer_[kBytes];
};
