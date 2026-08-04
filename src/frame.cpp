#include "frame.h"

#include <Arduino.h>
#include <string.h>

namespace {

// Decode one UTF-8 code point from `text`, advancing the pointer.
uint32_t decodeUtf8(const char *&text) {
  const uint8_t b0 = static_cast<uint8_t>(*text++);
  if (b0 < 0x80) {
    return b0;
  }
  if ((b0 & 0xE0) == 0xC0) {
    const uint8_t b1 = static_cast<uint8_t>(*text);
    if ((b1 & 0xC0) == 0x80) {
      ++text;
      return ((b0 & 0x1F) << 6) | (b1 & 0x3F);
    }
    return 0xFFFD;
  }
  if ((b0 & 0xF0) == 0xE0) {
    const uint8_t b1 = static_cast<uint8_t>(text[0]);
    const uint8_t b2 = static_cast<uint8_t>(text[1]);
    if (((b1 & 0xC0) == 0x80) && ((b2 & 0xC0) == 0x80)) {
      text += 2;
      return ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
    }
    return 0xFFFD;
  }
  if ((b0 & 0xF8) == 0xF0) {
    const uint8_t b1 = static_cast<uint8_t>(text[0]);
    const uint8_t b2 = static_cast<uint8_t>(text[1]);
    const uint8_t b3 = static_cast<uint8_t>(text[2]);
    if (((b1 & 0xC0) == 0x80) && ((b2 & 0xC0) == 0x80) && ((b3 & 0xC0) == 0x80)) {
      text += 3;
      return ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
    }
    return 0xFFFD;
  }
  return 0xFFFD;
}

}  // namespace

void FrameBuffer::clear(uint8_t color) {
  const uint8_t packed = color | (color << 2) | (color << 4) | (color << 6);
  memset(buffer_, packed, sizeof(buffer_));
}

void FrameBuffer::putPixel(int x, int y, uint8_t color) {
  if ((x < 0) || (x >= kWidth) || (y < 0) || (y >= kHeight)) {
    return;
  }
  const size_t index = static_cast<size_t>(y) * kWidth + x;
  const uint8_t shift = 6 - 2 * (x & 3);
  buffer_[index >> 2] = static_cast<uint8_t>((buffer_[index >> 2] & ~(0x03U << shift)) |
                                             ((color & 0x03U) << shift));
}

uint8_t FrameBuffer::getPixel(int x, int y) const {
  if ((x < 0) || (x >= kWidth) || (y < 0) || (y >= kHeight)) {
    return kColorWhite;
  }
  const size_t index = static_cast<size_t>(y) * kWidth + x;
  const uint8_t shift = 6 - 2 * (x & 3);
  return static_cast<uint8_t>((buffer_[index >> 2] >> shift) & 0x03U);
}

void FrameBuffer::drawHLine(int x, int y, int len, uint8_t color) {
  for (int i = 0; i < len; ++i) {
    putPixel(x + i, y, color);
  }
}

void FrameBuffer::drawVLine(int x, int y, int len, uint8_t color) {
  for (int i = 0; i < len; ++i) {
    putPixel(x, y + i, color);
  }
}

void FrameBuffer::fillRect(int x, int y, int w, int h, uint8_t color) {
  if (w <= 0 || h <= 0) {
    return;
  }
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > kWidth) {
    w = kWidth - x;
  }
  if (y + h > kHeight) {
    h = kHeight - y;
  }
  for (int row = y; row < y + h; ++row) {
    for (int col = x; col < x + w; ++col) {
      putPixel(col, row, color);
    }
  }
}

void FrameBuffer::drawRect(int x, int y, int w, int h, uint8_t color, int thickness) {
  for (int t = 0; t < thickness; ++t) {
    drawHLine(x + t, y + t, w - 2 * t, color);
    drawHLine(x + t, y + h - 1 - t, w - 2 * t, color);
    drawVLine(x + t, y + t, h - 2 * t, color);
    drawVLine(x + w - 1 - t, y + t, h - 2 * t, color);
  }
}

void FrameBuffer::drawGlyph(int x, int y, const uint8_t *bits, uint8_t size, uint8_t color) {
  const int rowBytes = (size + 7) / 8;
  for (int row = 0; row < size; ++row) {
    const uint8_t *data = bits + row * rowBytes;
    for (int col = 0; col < size; ++col) {
      if (data[col >> 3] & (0x80U >> (col & 7))) {
        putPixel(x + col, y + row, color);
      }
    }
  }
}

int FrameBuffer::textWidth(const char *text, const FontInfo &font) const {
  int width = 0;
  while (*text) {
    decodeUtf8(text);
    width += font.size;
  }
  return width;
}

int FrameBuffer::drawText(int x, int y, const char *text, uint8_t color, const FontInfo &font) {
  int cursor = x;
  while (*text) {
    const uint32_t code = decodeUtf8(text);
    if (code == 0x20) {  // space: just advance
      cursor += font.size;
      continue;
    }
    const uint8_t *bits = fontFindGlyph(font, code);
    if (bits == nullptr) {
      Serial.print(F("[font] missing glyph U+"));
      Serial.println(code, HEX);
      cursor += font.size;
      continue;
    }
    drawGlyph(cursor, y, bits, font.size, color);
    cursor += font.size;
  }
  return cursor - x;
}
